#pragma once
#include "PerfTools.h"
#include <QHash>
#include <QList>
#include <QString>
#include <functional>

struct RagChunk {
    QString file;
    int startLine = 1;
    QString text;
};

// Yerel mini-RAG: proje dosyalarını parçalara böler, anahtar kelime skoruyla ilgili
// parçaları bulur. Harici bağımlılık yok, tamamı yerelde çalışır.
class RagIndexer {
public:
    int indexProject(const QString& root);            // chunk sayısını döner
    // Stage 32: artımlı — değişmeyen dosyaların parçaları yeniden kullanılır.
    int indexProjectIncremental(const QString& root);
    void clear() {
        m_chunks.clear();
        m_files = 0;
        m_index.clear();
        m_vecs.clear();
    }
    void clearCache() { m_cache.clear(); }
    int chunkCount() const { return m_chunks.size(); }
    int fileCount() const { return m_files; }
    int cacheCount() const { return m_cache.size(); } // Stage 32
    bool isEmpty() const { return m_chunks.isEmpty(); }

    QList<RagChunk> query(const QString& question, int topK = 4) const;
    static QString formatContext(const QList<RagChunk>& chunks, int maxChars = 6000);
    static QStringList sourceExts();

    // --- Stage 33: anlamsal (embedding) + hibrit arama ---
    // Gömme fonksiyonu: metin listesi → vektör listesi (internet/yerel çağrı).
    void setEmbedder(std::function<QList<QList<float>>(const QStringList&)> fn) {
        m_embedder = std::move(fn);
    }
    bool hasEmbedder() const { return bool(m_embedder); }
    int vectorCount() const { return m_vecs.size(); }
    // Parçalar için eksik vektörleri üret (arka plan iş parçacığında çağır).
    int embedAll(int batch = 16);
    // Anahtar kelime sıralaması (parça indeksleri, iyi→kötü).
    QList<int> keywordOrder(const QString& question, int pool = 40) const;
    // Hibrit: anahtar kelime + anlamsal (RRF). queryVec boşsa yalnız anahtar kelime.
    QList<RagChunk> queryHybrid(const QString& question, const QList<float>& queryVec,
                                int topK = 4, double kwWeight = 0.5,
                                double semWeight = 0.5) const;

private:
    int scan(const QString& root, bool reuse);
    static QList<RagChunk> parseFile(const QString& path);
    void buildIndex();

    struct Cached {
        qint64 mtime = 0;
        qint64 size = 0;
        QList<RagChunk> chunks;
    };

    QList<RagChunk> m_chunks;
    QHash<QString, Cached> m_cache; // Stage 32: dosya → (mtime, parçalar)
    PerfTools::InvertedIndex m_index;
    int m_files = 0;
    // Stage 33: parça başına gömme vektörü (m_chunks ile hizalı)
    std::function<QList<QList<float>>(const QStringList&)> m_embedder;
    QList<QList<float>> m_vecs;
};
