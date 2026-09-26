#include "EmbedCache.h"
#include <QCryptographicHash>

EmbedCache::EmbedCache(int maxEntries) { m_max = qMax(2, maxEntries); }

void EmbedCache::setMaxEntries(int n) {
    m_max = qMax(2, n);
    prune();
}

void EmbedCache::clear() {
    m_map.clear();
    m_order.clear();
    m_hits = 0;
    m_misses = 0;
}

double EmbedCache::hitRate() const {
    const int total = m_hits + m_misses;
    return total > 0 ? double(m_hits) / double(total) : 0.0;
}

QString EmbedCache::textDigest(const QString& text) {
    const QByteArray d = QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(d.toHex().left(24));
}

QString EmbedCache::keyFor(const QString& model, const QString& text) {
    return model.trimmed().toLower() + QLatin1Char('|') + textDigest(text);
}

void EmbedCache::touch(const QString& key) {
    m_order.removeAll(key);
    m_order.append(key); // en yeni sona
}

bool EmbedCache::contains(const QString& model, const QString& text) const {
    return m_map.contains(keyFor(model, text));
}

QList<float> EmbedCache::get(const QString& model, const QString& text) const {
    return m_map.value(keyFor(model, text));
}

void EmbedCache::put(const QString& model, const QString& text, const QList<float>& vec) {
    if (vec.isEmpty()) return;
    const QString k = keyFor(model, text);
    if (!m_map.contains(k)) {
        m_order.append(k);
    } else {
        m_order.removeAll(k);
        m_order.append(k);
    }
    m_map.insert(k, vec);
    prune();
}

int EmbedCache::prune() {
    int removed = 0;
    while (m_order.size() > m_max) {
        const QString k = m_order.takeFirst();
        if (m_map.remove(k)) ++removed;
    }
    // Sıra listesinde artık bulunmayanları da temizle
    const QStringList keys = m_map.keys();
    for (const QString& k : keys)
        if (!m_order.contains(k)) {
            m_map.remove(k);
            ++removed;
        }
    return removed;
}

QList<int> EmbedCache::missing(const QString& model, const QStringList& texts) {
    QList<int> out;
    for (int i = 0; i < texts.size(); ++i) {
        if (m_map.contains(keyFor(model, texts.at(i))))
            continue;
        out << i;
    }
    // İstatistik güncelle (arama deseni = en sıcak kullanım)
    m_hits += texts.size() - out.size();
    m_misses += out.size();
    return out;
}

void EmbedCache::putMany(const QString& model, const QStringList& texts,
                         const QList<QList<float>>& vecs) {
    for (int i = 0; i < texts.size() && i < vecs.size(); ++i)
        put(model, texts.at(i), vecs.at(i));
}
