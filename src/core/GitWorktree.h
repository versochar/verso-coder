#pragma once
#include <QList>
#include <QString>

class GitRunner;

// Git çalışma ağaçları (worktree): listele/ekle/kaldır/budama.
// GitRunner soyutlamasıyla çalışır (yerel + test edilebilir).
struct WorktreeInfo {
    QString path;
    QString commit;
    QString branch; // refs/heads/... ya da "(ayrık)"
    bool bare = false;
    bool detached = false;
    bool locked = false;
    bool prunable = false;
};

class GitWorktree {
public:
    static QList<WorktreeInfo> list(GitRunner& g, QString* error = nullptr);
    static bool add(GitRunner& g, const QString& path, const QString& rev,
                    const QString& newBranch, QString* error = nullptr);
    static bool remove(GitRunner& g, const QString& path, bool force,
                       QString* error = nullptr);
    static bool prune(GitRunner& g, QString* error = nullptr);
};
