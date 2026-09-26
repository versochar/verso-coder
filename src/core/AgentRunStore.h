#pragma once
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

// Stage 34: ajan koşu günlüğü + geri alma.
// Her koşu <dir>/<id>.json olarak saklanır; değiştirilen dosyaların eski
// içerikleri saklanır, böylece koşu tek komutla geri alınabilir.
struct AgentRunStep {
    QString assistant;
    QStringList toolNames;
    QStringList observations;
};

// Geri alınabilir tek dosya değişikliği.
struct RunFile {
    QString path;
    QString before;   // değişiklik öncesi içerik
    bool created = false; // ajan yeni dosya oluşturduysa true (geri alma = sil)
};

struct AgentRun {
    QString id;
    QString task;
    qint64 whenMs = 0;
    qint64 elapsedMs = 0;
    int steps = 0;
    int toolCalls = 0;
    int tokens = 0;
    bool ok = false;
    QString finalText;
    QList<AgentRunStep> stepLog;
    QList<RunFile> changedFiles;

    int filesChanged() const { return changedFiles.size(); }
    QString statusLabel() const;
    QString oneLine() const;
};

class AgentRunStore {
public:
    explicit AgentRunStore(const QString& dir);

    QString add(const AgentRun& run);   // kimlik üretip kaydeder, kimliği döner
    QList<AgentRun> list(int limit = 50) const; // yeni→eski
    AgentRun load(const QString& id) const;
    bool remove(const QString& id);
    int count() const;
    QString dir() const { return m_dir; }

    // Koşunun dosya değişikliklerini geri al. Değiştirilmiş dosyalara dokunmaz.
    static QString revert(const AgentRun& run, int* reverted = nullptr,
                          QStringList* skipped = nullptr);
    static QString diffSummary(const AgentRun& run, int maxChars = 4000);

private:
    QString m_dir;
};
