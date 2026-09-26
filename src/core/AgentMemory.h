#pragma once
#include <QList>
#include <QString>
#include <QStringList>

// Stage 34: ajan belleği — kalıcı notlar (proje kuralları, öğrenilen yollar,
// sık kullanılan komutlar, başarısız yaklaşımlar).
struct MemoryNote {
    QString id;
    QString text;
    QString tag;   // rule | path | command | fact | failure
    int weight = 1; // öncelik puanı (yükselir)
    int uses = 0;
    qint64 whenMs = 0;
};

class AgentMemory {
public:
    explicit AgentMemory(const QString& filePath);

    bool load();
    bool save() const;
    QString filePath() const { return m_file; }

    // Not ekler, kimliğini döner. Aynı metin varsa ağırlığı artırır (tekilleştirme).
    QString add(const QString& text, const QString& tag = "fact", int weight = 1);
    bool remove(const QString& id);
    void clear();
    int count() const { return m_notes.size(); }
    QList<MemoryNote> notes() const { return m_notes; }
    QStringList texts() const;

    // --- saf yardımcılar (test edilebilir) ---
    static QStringList tags();
    static QString normalizeTag(const QString& tag);
    static double relevance(const QString& query, const MemoryNote& n);
    static QList<MemoryNote> recall(const QList<MemoryNote>& all, const QString& query,
                                    int topK = 6, double minScore = 0.15);
    static QString formatRecall(const QList<MemoryNote>& hits, const QString& query,
                                int maxChars = 2000);
    // Basit kelime kesişimi (TR/EN ortak)
    static QStringList tokens(const QString& text);

private:
    QString m_file;
    QList<MemoryNote> m_notes;
};
