#include "PathGuard.h"
#include <QChar>
#include <QDir>
#include <QFileInfo>

PathGuard::PathGuard(const QString& root) {
    m_root = QDir::cleanPath(QDir(root).absolutePath());
    const QString c = canonicalizeBestEffort(m_root);
    m_canonRoot = c.isEmpty() ? m_root : c;
}

QStringList PathGuard::segmentsOf(const QString& cleanAbsPath) {
    QString p = QDir::cleanPath(cleanAbsPath);
    // Kök öneki ("/" veya "C:/") ayrılır
    QString prefix;
    if (p.startsWith(QLatin1Char('/'))) {
        prefix = QStringLiteral("/");
        p = p.mid(1);
    } else if (p.size() > 2 && p.at(1) == QLatin1Char(':')) {
        prefix = p.left(2);
        p = p.mid(2);
    }
    const QStringList parts = p.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    QStringList out;
    out << prefix;
    for (const QString& s : parts) out << s;
    return out;
}

QString PathGuard::canonicalizeBestEffort(const QString& absPath) {
    const QFileInfo fi(absPath);
    if (fi.exists() || fi.isSymLink()) {
        const QString c = fi.canonicalFilePath();
        return c.isEmpty() ? QDir::cleanPath(absPath) : c;
    }
    // Yoksa: var olan en yakın ataya kadar in, kanonikleştir, yaprağı ekle
    const QStringList segs = segmentsOf(absPath);
    if (segs.size() <= 1) return QDir::cleanPath(absPath);
    QStringList tail;
    for (int i = segs.size() - 1; i >= 1; --i) {
        if (segs.at(i) == QLatin1String("..")) {
            tail.prepend(segs.at(i));
            continue;
        }
        tail.prepend(segs.at(i));
        break;
    }
    QString parent = segs.mid(0, segs.size() - tail.size()).join(QLatin1Char('/'));
    if (parent.isEmpty()) parent = QStringLiteral("/");
    const QFileInfo pfi(parent);
    QString canonParent = pfi.canonicalFilePath();
    if (canonParent.isEmpty()) canonParent = QDir::cleanPath(parent);
    QString out = canonParent;
    for (const QString& t : tail) {
        if (t == QLatin1String("..")) {
            out = QDir::cleanPath(out + QStringLiteral("/.."));
        } else {
            out += QLatin1Char('/') + t;
        }
    }
    return QDir::cleanPath(out);
}

QString PathGuard::progressiveCanonical(const QString& cleanAbsPath) {
    // Her önek için canonicalFilePath çağır; ilk kaçışı döndür (yoksa son önek)
    const QStringList segs = segmentsOf(cleanAbsPath);
    if (segs.size() <= 1) return QDir::cleanPath(cleanAbsPath);
    QString acc = segs.first();
    QString last = acc;
    for (int i = 1; i < segs.size(); ++i) {
        if (segs.at(i) == QLatin1String("..")) {
            acc = QDir::cleanPath(acc + QStringLiteral("/.."));
            last = acc;
            continue;
        }
        const QString cand = acc + QLatin1Char('/') + segs.at(i);
        const QFileInfo fi(cand);
        QString canon = fi.canonicalFilePath();
        if (canon.isEmpty()) {
            // Var olmayan bileşen: kalanı düz ekle (aşağıda en yakın atayla
            // kanonikleştirilecek)
            acc = cand;
            last = cand;
        } else {
            acc = canon;
            last = canon;
        }
    }
    return last;
}

bool PathGuard::isSameOrInside(const QString& canonicalRoot, const QString& canonicalPath) {
    if (canonicalRoot.isEmpty()) return false;
    QString r = QDir::cleanPath(canonicalRoot);
    QString p = QDir::cleanPath(canonicalPath);
    while (r.endsWith(QLatin1Char('/')) && r.size() > 1) r.chop(1);
    if (p == r) return true;
    return p.startsWith(r + QLatin1Char('/'));
}

bool PathGuard::isSuspiciousInput(const QString& raw, QString* why) {
    if (raw.isEmpty()) {
        if (why) *why = QStringLiteral("yol boş");
        return true;
    }
    if (raw.contains(QChar(0))) {
        if (why) *why = QStringLiteral("yol içinde null bayt var");
        return true;
    }
    for (QChar c : raw) {
        if (c.unicode() < 0x20) {
            if (why) *why = QStringLiteral("yol içinde kontrol karakteri var");
            return true;
        }
    }
    if (raw.size() > 4096) {
        if (why) *why = QStringLiteral("yol çok uzun (%1 karakter)").arg(raw.size());
        return true;
    }
    // "..." veya "a/../.." gibi yığınlmış atlamalar
    if (raw.contains(QStringLiteral("..."))) {
        if (why) *why = QStringLiteral("şüpheli yol kalıbı: '...'");
        return true;
    }
    return false;
}

QString PathGuard::escapeSegment(const QString& path) const {
    if (m_canonRoot.isEmpty()) return QStringLiteral("(kök tanımsız)");
    const QString abs = QDir::isAbsolutePath(path)
                            ? QDir::cleanPath(path)
                            : QDir::cleanPath(m_root + QLatin1Char('/') + path);

    // Kademeli denetim KÖKÜN GÖRELİ kısmı üzerinde yürür. Kökün kendisi zaten
    // kanonikleştirilmiştir: mutlak yoldan ("/" + kök segmentleri) başlayan
    // eski yöntem, kökün atalarını da denetleyip onları "kaçış" sanıyordu
    // (örn. kök /tmp/... iken "tmp" bile reddediliyordu).
    QString rel = abs;
    bool relativeToRoot = false;
    if (abs == m_root) {
        return {};
    }
    if (abs.startsWith(m_root + QLatin1Char('/'))) {
        rel = abs.mid(m_root.size() + 1);
        relativeToRoot = true;
    }

    QString acc = relativeToRoot ? m_canonRoot : QStringLiteral("/");
    const QStringList segs = relativeToRoot
                                 ? rel.split(QLatin1Char('/'), Qt::SkipEmptyParts)
                                 : segmentsOf(abs);
    const int from = relativeToRoot ? 0 : 1;
    for (int i = from; i < segs.size(); ++i) {
        const QString& s = segs.at(i);
        if (s == QLatin1String("..")) {
            acc = QDir::cleanPath(acc + QStringLiteral("/.."));
            if (!isSameOrInside(m_canonRoot, acc)) return QStringLiteral("..");
            continue;
        }
        const QString cand = acc + QLatin1Char('/') + s;
        const QFileInfo fi(cand);
        if (fi.isSymLink() || fi.exists()) {
            const QString c = fi.canonicalFilePath();
            if (!c.isEmpty()) {
                // Sembolik bağlantı kök dışını gösteriyorsa reddet
                if (!isSameOrInside(m_canonRoot, c)) return s;
                acc = c;
                continue;
            }
        }
        acc = cand; // henüz var olmayan bileşen
    }
    return {};
}

bool PathGuard::hasSymlinkEscape(const QString& path) const {
    const QString seg = escapeSegment(path);
    return !seg.isEmpty() && seg != QDir::cleanPath(
                                  QDir::isAbsolutePath(path)
                                      ? path
                                      : m_root + QLatin1Char('/') + path);
}

bool PathGuard::isInside(const QString& path, QString* why) const {
    if (m_root.isEmpty()) {
        if (why) *why = QStringLiteral("proje kökü tanımsız");
        return false;
    }
    if (isSuspiciousInput(path, why)) return false;

    const QString abs = QDir::isAbsolutePath(path)
                            ? QDir::cleanPath(path)
                            : QDir::cleanPath(m_root + QLatin1Char('/') + path);
    // Kademeli: ".." kökü aşmamalı
    const QString seg = escapeSegment(abs);
    if (!seg.isEmpty()) {
        if (why) {
            *why = (seg == QLatin1String(".."))
                       ? QStringLiteral("yol kök dışına çıkıyor (parent erişimi)")
                       : QStringLiteral("sembolik bağlantı kök dışına işaret ediyor: '%1'").arg(seg);
        }
        return false;
    }
    const QString canon = canonicalizeBestEffort(abs);
    if (!isSameOrInside(m_canonRoot, canon)) {
        if (why) *why = QStringLiteral("yol kanonik haliyle kök dışında");
        return false;
    }
    if (why) why->clear();
    return true;
}

bool PathGuard::resolve(const QString& input, QString& outAbs, QString* why) const {
    if (!isInside(input, why)) {
        outAbs.clear();
        return false;
    }
    const QString abs = QDir::isAbsolutePath(input)
                            ? QDir::cleanPath(input)
                            : QDir::cleanPath(m_root + QLatin1Char('/') + input);
    outAbs = canonicalizeBestEffort(abs);
    if (why) why->clear();
    return true;
}
