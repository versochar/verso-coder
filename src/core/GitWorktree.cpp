#include "GitWorktree.h"
#include "GitRunner.h"

QList<WorktreeInfo> GitWorktree::list(GitRunner& g, QString* error) {
    QList<WorktreeInfo> out;
    GitRunner::Result r = g.run({"worktree", "list", "--porcelain"}, 15000);
    if (r.exit != 0) {
        if (error) *error = r.err.trimmed().left(200);
        return out;
    }
    WorktreeInfo cur;
    bool open = false;
    auto flush = [&]() {
        if (open && !cur.path.isEmpty()) out << cur;
        cur = WorktreeInfo();
        open = false;
    };
    for (const QString& ln : r.out.split('\n')) {
        if (ln.trimmed().isEmpty()) {
            flush();
            continue;
        }
        if (ln.startsWith("worktree ")) {
            flush();
            cur.path = ln.mid(9).trimmed();
            open = true;
        } else if (ln.startsWith("HEAD ")) {
            cur.commit = ln.mid(5, 12).trimmed();
        } else if (ln.startsWith("branch ")) {
            cur.branch = ln.mid(7).trimmed();
        } else if (ln == "bare") {
            cur.bare = true;
        } else if (ln == "detached") {
            cur.detached = true;
        } else if (ln.startsWith("locked")) {
            cur.locked = true;
        } else if (ln.startsWith("prunable")) {
            cur.prunable = true;
        }
    }
    flush();
    for (auto& w : out)
        if (w.branch.isEmpty()) w.branch = w.detached ? "(ayrık)" : "-";
    return out;
}

bool GitWorktree::add(GitRunner& g, const QString& path, const QString& rev,
                      const QString& newBranch, QString* error) {
    if (path.trimmed().isEmpty()) {
        if (error) *error = "dizin boş";
        return false;
    }
    QStringList a = {"worktree", "add"};
    if (!newBranch.trimmed().isEmpty()) a << "-b" << newBranch.trimmed();
    a << path.trimmed();
    if (!rev.trimmed().isEmpty()) a << rev.trimmed();
    GitRunner::Result r = g.run(a, 30000);
    if (r.exit != 0 && error) *error = (r.err + " " + r.out).trimmed().left(300);
    return r.exit == 0;
}

bool GitWorktree::remove(GitRunner& g, const QString& path, bool force,
                         QString* error) {
    QStringList a = {"worktree", "remove"};
    if (force) a << "--force";
    a << path;
    GitRunner::Result r = g.run(a, 30000);
    if (r.exit != 0 && error) *error = (r.err + " " + r.out).trimmed().left(300);
    return r.exit == 0;
}

bool GitWorktree::prune(GitRunner& g, QString* error) {
    GitRunner::Result r = g.run({"worktree", "prune"}, 15000);
    if (r.exit != 0 && error) *error = (r.err + " " + r.out).trimmed().left(300);
    return r.exit == 0;
}
