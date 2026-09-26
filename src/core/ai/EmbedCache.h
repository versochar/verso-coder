#pragma once
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

// Stage 36: gömme vektör önbelleği — aynı metin için tekrar ağ çağrısı yapılmaz.
// Anahtar = model + metin özeti; değer = vektör. LRU tahliyesi ile sınırlı bellek.
class EmbedCache {
public:
    explicit EmbedCache(int maxEntries = 512);

    int maxEntries() const { return m_max; }
    void setMaxEntries(int n);
    int size() const { return m_map.size(); }
    int hits() const { return m_hits; }
    int misses() const { return m_misses; }
    double hitRate() const;
    void clear();
    int prune();

    static QString keyFor(const QString& model, const QString& text);
    // Metin özeti: uzun metinde tamamını saklamak bellek yiyor
    static QString textDigest(const QString& text);

    bool contains(const QString& model, const QString& text) const;
    QList<float> get(const QString& model, const QString& text) const;
    void put(const QString& model, const QString& text, const QList<float>& vec);

    // Toplu: eksik metinlerin indeksleri
    QList<int> missing(const QString& model, const QStringList& texts);
    // Toplu yazma (vektörler metinlerle sırayla eşlenir)
    void putMany(const QString& model, const QStringList& texts,
                 const QList<QList<float>>& vecs);
    // İstatistik: missing() her çağrıda isabet/ıskalama sayacını günceller

private:
    void touch(const QString& key);

    int m_max = 512;
    int m_hits = 0;
    int m_misses = 0;
    QHash<QString, QList<float>> m_map;
    QStringList m_order; // en eski başta
};
