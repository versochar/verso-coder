#include "PerfTools.h"
#include <QRegularExpression>
#include <QSet>

namespace PerfTools {

bool isLikelyBinary(const QByteArray& prefix) {
    if (prefix.isEmpty()) return false;
    int ctrl = 0;
    for (char c : prefix) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (u == 0) return true; // NUL kesin ikili
        if (u < 9 || (u > 13 && u < 32)) ++ctrl;
    }
    return ctrl * 100 > prefix.size() * 30; // %30+ kontrol → ikili
}

bool skipByExtAndSize(qint64 sizeBytes, const QString& suffix,
                      const QStringList& allowedExts, qint64 maxBytes) {
    if (sizeBytes > maxBytes) return true;
    if (allowedExts.isEmpty()) return false;
    const QString s = suffix.toLower();
    for (const QString& e : allowedExts)
        if (e == s) return false;
    return true;
}

int largeFileMode(qint64 sizeBytes, int previewMb, int readonlyMb) {
    if (previewMb <= 0) previewMb = 2;
    if (readonlyMb <= 0) readonlyMb = 8;
    const qint64 ro = qint64(readonlyMb) * 1024 * 1024;
    const qint64 pv = qint64(previewMb) * 1024 * 1024;
    if (sizeBytes >= ro) return 2;
    if (sizeBytes >= pv) return 1;
    return 0;
}

QString LowerCache::lower(const QString& key) {
    auto it = m_map.find(key);
    if (it != m_map.end()) {
        m_order.removeAll(key);
        m_order.append(key);
        return it.value();
    }
    const QString v = key.toLower();
    m_map.insert(key, v);
    m_order.append(key);
    while (m_order.size() > m_capacity) {
        const QString old = m_order.takeFirst();
        m_map.remove(old);
    }
    return v;
}

void InvertedIndex::add(int docId, const QString& text) {
    const QStringList words =
        text.toLower().split(QRegularExpression("[^\\p{L}\\p{N}_]+"), Qt::SkipEmptyParts);
    QSet<QString> seen;
    for (const QString& w : words) {
        if (w.size() < 2) continue;
        if (seen.contains(w)) continue;
        seen.insert(w);
        m_post[w].append(docId);
    }
    ++m_docs;
}

QHash<int, int> InvertedIndex::scoreDocs(const QStringList& kws) const {
    QHash<int, int> score;
    // Stage 32: tam-kelime yerine alt-dize uyumu — RAG'ın eski alt-dize
    // davranışını korur ("fonksiyon" → "fonksiyonu" belgesini de bulur).
    for (const QString& k : kws) {
        for (auto it = m_post.constBegin(); it != m_post.constEnd(); ++it) {
            if (!it.key().contains(k)) continue;
            for (int id : it.value()) ++score[id];
        }
    }
    return score;
}

QStringList keywords(const QString& query, int minLen) {
    QStringList out;
    QSet<QString> seen;
    const QStringList ws = query.toLower().split(QRegularExpression("[^\\p{L}\\p{N}_]+"),
                                                 Qt::SkipEmptyParts);
    for (const QString& w : ws) {
        if (w.size() < qMax(1, minLen)) continue;
        if (seen.contains(w)) continue;
        seen.insert(w);
        out << w;
    }
    return out;
}

} // namespace PerfTools
