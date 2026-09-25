#include "RagIndexer.h"
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QRegularExpression>

int RagIndexer::indexProject(const QString& root) {
    clear();
    if (root.isEmpty()) return 0;
    QDirIterator it(root, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    int scanned = 0;
    while (it.hasNext() && scanned < 600 && m_chunks.size() < 4000) {
        QString p = it.next();
        if (p.contains("/.git/") || p.contains("/build/") || p.contains("/node_modules/") ||
            p.contains("/.venv/") || p.contains("/__pycache__/"))
            continue;
        QFileInfo fi(p);
        if (fi.size() > 200 * 1024) continue; // büyük dosya atla
        // Sadece metin kaynaklar (uzantı filtresi + binary kontrolü)
        static const QStringList exts = {"cpp", "h", "hpp", "c", "py", "js", "ts", "qml",
                                         "cmake", "txt", "md", "json", "qss", "pro", "java", "rs", "go"};
        if (!exts.contains(fi.suffix().toLower()) && fi.fileName() != "CMakeLists.txt") {
            ++scanned;
            continue;
        }
        QFile f(p);
        if (!f.open(QIODevice::ReadOnly)) { ++scanned; continue; }
        QByteArray raw = f.read(200 * 1024);
        if (raw.contains('\0')) { ++scanned; continue; }
        QStringList lines = QString::fromUtf8(raw).split('\n');
        // 40 satırlık chunk, 10 satır overlap
        for (int s = 0; s < lines.size(); s += 30) {
            int e = qMin(s + 40, (int)lines.size());
            QStringList part;
            for (int i = s; i < e; ++i) part << lines[i];
            QString t = part.join('\n');
            if (t.trimmed().size() > 40)
                m_chunks.append({p, s + 1, t});
            if (e >= lines.size()) break;
        }
        ++m_files;
        ++scanned;
    }
    return m_chunks.size();
}

static QStringList keywords(const QString& q) {
    QStringList out;
    for (const QString& w : q.toLower().split(QRegularExpression("\\W+"), Qt::SkipEmptyParts))
        if (w.size() >= 3) out << w;
    return out;
}

QList<RagChunk> RagIndexer::query(const QString& question, int topK) const {
    struct Scored { int score = 0; int idx = 0; };
    QStringList kws = keywords(question);
    if (kws.isEmpty() || m_chunks.isEmpty()) return {};
    QList<Scored> ranked;
    for (int i = 0; i < m_chunks.size(); ++i) {
        QString t = m_chunks[i].text.toLower();
        int sc = 0;
        for (const QString& k : kws) {
            int c = 0, pos = 0;
            while ((pos = t.indexOf(k, pos)) >= 0) { ++c; pos += k.size(); if (c > 20) break; }
            sc += qMin(c, 20);
        }
        // Dosya adı eşleşme bonusu
        QString fn = QFileInfo(m_chunks[i].file).fileName().toLower();
        for (const QString& k : kws)
            if (fn.contains(k)) sc += 5;
        if (sc > 0) ranked.append({sc, i});
    }
    std::sort(ranked.begin(), ranked.end(),
              [](const Scored& a, const Scored& b) { return a.score > b.score; });
    QList<RagChunk> out;
    for (int i = 0; i < qMin(topK, (int)ranked.size()); ++i)
        out << m_chunks[ranked[i].idx];
    return out;
}

QString RagIndexer::formatContext(const QList<RagChunk>& chunks, int maxChars) {
    QString out;
    for (const auto& c : chunks) {
        QString head = QString("\n[Dosya: %1:%2]\n").arg(c.file).arg(c.startLine);
        QString body = c.text.left(2000);
        if (out.size() + head.size() + body.size() > maxChars) break;
        out += head + body + "\n";
    }
    return out;
}
