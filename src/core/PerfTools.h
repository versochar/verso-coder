#pragma once
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

// Stage 32: hız yardımcıları — saf mantık (test edilebilir).
namespace PerfTools {

// İlk baytlardan ikili (binary) sez: NUL ya da çok sayıda kontrol karakteri.
bool isLikelyBinary(const QByteArray& prefix);

// Uzantı + boyut süzgeci: arama/RAG taramasında atlanmalı mı?
bool skipByExtAndSize(qint64 sizeBytes, const QString& suffix,
                      const QStringList& allowedExts, qint64 maxBytes);

// Büyük dosya kipi: 0 = normal, 1 = önizleme (kısmi), 2 = salt-okunur
int largeFileMode(qint64 sizeBytes, int previewMb, int readonlyMb);

// Basit LRU metin önbelleği (harf büyüklüğü önceden hesaplanır).
class LowerCache {
public:
    // Metnin küçük harfli hâlini cache'ler (en çok capacity öğe, LRU).
    QString lower(const QString& key);
    void clear() { m_map.clear(); m_order.clear(); }
    int size() const { return m_map.size(); }

private:
    int m_capacity = 512;
    QHash<QString, QString> m_map;
    QList<QString> m_order; // en eski başta
};

// Ters indeks (anahtar kelime → belge sırası). Arama/RAG sorgusunu hızlandırır.
class InvertedIndex {
public:
    void clear() { m_post.clear(); }
    void add(int docId, const QString& text);
    // Sorgu anahtarlarını içeren belge kimlikleri (alt-dize uyumlu; aday daraltma).
    QHash<int, int> scoreDocs(const QStringList& keywords) const;
    int termCount() const { return m_post.size(); }
    int docCount() const { return m_docs; }

private:
    QHash<QString, QList<int>> m_post;
    int m_docs = 0;
};

// Sorguyu anahtar kelimelere ayır (>= minLen, tekrarsız).
QStringList keywords(const QString& query, int minLen = 3);

} // namespace PerfTools
