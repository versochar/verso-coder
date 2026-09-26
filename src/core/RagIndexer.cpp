#include "RagIndexer.h"
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>

QStringList RagIndexer::sourceExts() {
    return {"cpp", "h", "hpp", "c", "py", "js", "ts", "qml",
            "cmake", "txt", "md", "json", "qss", "pro", "java", "rs", "go"};
}

int RagIndexer::indexProject(const QString& root) {
    m_cache.clear(); // tam yeniden indeks
    return scan(root, false);
}

int RagIndexer::indexProjectIncremental(const QString& root) {
    return scan(root, true); // Stage 32: değişmeyen dosyalar yeniden kullanılır
}

QList<RagChunk> RagIndexer::parseFile(const QString& path) {
    QList<RagChunk> out;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return out;
    QByteArray raw = f.read(200 * 1024);
    if (raw.contains('\0')) return out;
    QStringList lines = QString::fromUtf8(raw).split('\n');
    // 40 satırlık chunk, 10 satır örtüşme
    for (int s = 0; s < lines.size(); s += 30) {
        int e = qMin(s + 40, (int)lines.size());
        QStringList part;
        for (int i = s; i < e; ++i) part << lines[i];
        QString t = part.join('\n');
        if (t.trimmed().size() > 40) out.append({path, s + 1, t});
        if (e >= lines.size()) break;
    }
    return out;
}

int RagIndexer::scan(const QString& root, bool reuse) {
    m_chunks.clear();
    m_index.clear();
    m_files = 0;
    if (root.isEmpty()) {
        m_cache.clear();
        return 0;
    }
    const QStringList exts = sourceExts();
    QDirIterator it(root, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    int scanned = 0;
    QSet<QString> visited;
    while (it.hasNext() && scanned < 600 && m_chunks.size() < 4000) {
        QString p = it.next();
        if (p.contains("/.git/") || p.contains("/build/") || p.contains("/node_modules/") ||
            p.contains("/.venv/") || p.contains("/__pycache__/"))
            continue;
        QFileInfo fi(p);
        if (fi.size() > 200 * 1024) continue; // büyük dosya atla
        const QString suffix = fi.suffix().toLower();
        if (!exts.contains(suffix) && fi.fileName() != "CMakeLists.txt") {
            ++scanned;
            continue;
        }
        ++scanned;
        visited.insert(p);
        const qint64 mtime = fi.lastModified().toMSecsSinceEpoch();
        auto cached = m_cache.find(p);
        if (reuse && cached != m_cache.end() && cached->mtime == mtime &&
            cached->size == fi.size()) {
            m_chunks.append(cached->chunks); // değişmemiş: yeniden oku yok
            ++m_files;
            continue;
        }
        QList<RagChunk> chunks = parseFile(p);
        m_cache.insert(p, {mtime, fi.size(), chunks});
        m_chunks.append(chunks);
        ++m_files;
    }
    // Artık ağaçta olmayan önbellek girdilerini temizle
    for (auto it2 = m_cache.begin(); it2 != m_cache.end();) {
        if (!visited.contains(it2.key()))
            it2 = m_cache.erase(it2);
        else
            ++it2;
    }
    buildIndex();
    return m_chunks.size();
}

void RagIndexer::buildIndex() {
    for (int i = 0; i < m_chunks.size(); ++i) {
        const RagChunk& c = m_chunks[i];
        // Dosya adı jetonları da indekslenir (dosya adı bonusu için)
        m_index.add(i, QFileInfo(c.file).fileName() + " " + c.text);
    }
}

QList<RagChunk> RagIndexer::query(const QString& question, int topK) const {
    const QStringList kws = PerfTools::keywords(question);
    if (kws.isEmpty() || m_chunks.isEmpty()) return {};

    // Stage 32: ters indeksten adaylar (tüm korpusu taramaz)
    QHash<int, int> candidate = m_index.scoreDocs(kws);
    if (candidate.isEmpty()) return {};

    struct Scored { int score = 0; int idx = 0; };
    QList<Scored> ranked;
    ranked.reserve(candidate.size());
    for (auto c = candidate.begin(); c != candidate.end(); ++c) {
        const int i = c.key();
        const RagChunk& rc = m_chunks[i];
        int sc = 0;
        for (const QString& k : kws) {
            int n = 0, pos = 0;
            while ((pos = rc.text.indexOf(k, pos, Qt::CaseInsensitive)) >= 0) {
                ++n;
                pos += k.size();
                if (n > 20) break;
            }
            sc += qMin(n, 20);
        }
        const QString fn = QFileInfo(rc.file).fileName().toLower();
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
