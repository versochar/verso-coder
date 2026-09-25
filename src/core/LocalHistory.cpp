#include "LocalHistory.h"
#include <QDir>
#include <QFile>
#include <QCryptographicHash>
#include <algorithm>

LocalHistory::LocalHistory(const QString& storeDir) : m_store(storeDir) {}

QString LocalHistory::keyFor(const QString& filePath) {
    const QByteArray h =
        QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Sha1).toHex();
    return QString::fromUtf8(h.left(12));
}

QString LocalHistory::stamp() {
    return QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
}

QString LocalHistory::dirFor(const QString& filePath) const {
    return QDir(m_store).absoluteFilePath("history/" + keyFor(filePath));
}

QList<HistorySnap> LocalHistory::list(const QString& filePath) const {
    QList<HistorySnap> out;
    QDir d(dirFor(filePath));
    if (!d.exists()) return out;
    // Yeniden eskiye: önce mtime, eşitlikte ad tersi (aynı-saniye sayacı: -2 yenidir)
    QFileInfoList infos = d.entryInfoList({"*.bak"}, QDir::Files, QDir::NoSort);
    std::sort(infos.begin(), infos.end(), [](const QFileInfo& a, const QFileInfo& b) {
        if (a.lastModified() != b.lastModified())
            return a.lastModified() > b.lastModified();
        return a.fileName() > b.fileName();
    });
    for (const QFileInfo& fi : infos) {
        HistorySnap s;
        const QString f = fi.fileName();
        s.id = f.left(f.size() - 4);
        s.when = QDateTime::fromString(s.id.left(15), "yyyyMMdd-HHmmss");
        if (!s.when.isValid()) s.when = fi.lastModified();
        s.path = fi.absoluteFilePath();
        s.size = fi.size();
        out << s;
    }
    return out;
}

bool LocalHistory::snapshot(const QString& filePath, const QString& content,
                            int keepMax) {
    if (filePath.isEmpty()) return false;
    QDir().mkpath(dirFor(filePath));
    // Aynı içerik varsa atla (son anlık görüntüyle karşılaştır)
    const QList<HistorySnap> cur = list(filePath);
    if (!cur.isEmpty() && read(cur.first()) == content) return false;
    QString id = stamp();
    int n = 1;
    QString dest = QDir(dirFor(filePath)).absoluteFilePath(id + ".bak");
    // Aynı saniye çakışması: id'ye sayaç ekle (sıralama yeniyi öne koyar)
    while (QFile::exists(dest)) {
        id = QString("%1-%2").arg(stamp()).arg(++n);
        dest = QDir(dirFor(filePath)).absoluteFilePath(id + ".bak");
    }
    QFile f(dest);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    // İlk satır meta (özgün yol), sonra içerik
    f.write(("path=" + filePath + "\n").toUtf8());
    f.write(content.toUtf8());
    f.close();
    prune(filePath, keepMax);
    return true;
}

QString LocalHistory::read(const HistorySnap& s) const {
    QFile f(s.path);
    if (!f.open(QIODevice::ReadOnly)) return QString();
    QString all = QString::fromUtf8(f.readAll());
    const int nl = all.indexOf('\n');
    return (nl >= 0) ? all.mid(nl + 1) : QString();
}

bool LocalHistory::restore(const QString& filePath, const HistorySnap& s) {
    QFile cur(filePath);
    if (cur.open(QIODevice::ReadOnly)) {
        const QString now = QString::fromUtf8(cur.readAll());
        cur.close();
        snapshot(filePath, now); // günü kurtar
    }
    const QString content = read(s);
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(content.toUtf8());
    return true;
}

bool LocalHistory::clear(const QString& filePath) {
    QDir d(dirFor(filePath));
    if (!d.exists()) return true;
    bool ok = true;
    for (const QString& f : d.entryList({"*.bak"}, QDir::Files))
        ok &= d.remove(f);
    return ok;
}

int LocalHistory::prune(const QString& filePath, int keepMax) {
    QList<HistorySnap> all = list(filePath); // yeniden eskiye
    int removed = 0;
    while (all.size() > keepMax) {
        if (QFile::remove(all.takeLast().path)) ++removed;
        else break;
    }
    return removed;
}
