#pragma once
#include <QList>
#include <QString>

struct RagChunk {
    QString file;
    int startLine = 1;
    QString text;
};

// Yerel mini-RAG: proje dosyalarını parçalara böler, anahtar kelime skoruyla ilgili
// parçaları bulur. Harici bağımlılık yok, tamamı yerelde çalışır.
class RagIndexer {
public:
    int indexProject(const QString& root); // chunk sayısını döner
    void clear() { m_chunks.clear(); m_files = 0; }
    int chunkCount() const { return m_chunks.size(); }
    int fileCount() const { return m_files; }
    bool isEmpty() const { return m_chunks.isEmpty(); }

    QList<RagChunk> query(const QString& question, int topK = 4) const;
    static QString formatContext(const QList<RagChunk>& chunks, int maxChars = 6000);

private:
    QList<RagChunk> m_chunks;
    int m_files = 0;
};
