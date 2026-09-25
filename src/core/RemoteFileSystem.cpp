#include "RemoteFileSystem.h"
#include <QRegularExpression>

bool RemoteFileSystem::isRemotePath(const QString& path) {
    return path.startsWith("ssh://");
}

RemoteFileSystem::ParsedUri RemoteFileSystem::parseUri(const QString& uri) {
    ParsedUri p;
    if (!uri.startsWith("ssh://")) return p;
    QString rest = uri.mid(6);
    // [user@]host[:port]/path
    int slash = rest.indexOf('/');
    QString hostPart = (slash < 0) ? rest : rest.left(slash);
    p.path = (slash < 0) ? "/" : rest.mid(slash);
    int at = hostPart.lastIndexOf('@');
    if (at >= 0) { p.user = hostPart.left(at); hostPart = hostPart.mid(at + 1); }
    int colon = hostPart.lastIndexOf(':');
    if (colon >= 0) {
        bool ok = false;
        const int port = hostPart.mid(colon + 1).toInt(&ok);
        if (ok && port > 0) { p.port = port; hostPart = hostPart.left(colon); }
    }
    p.host = hostPart;
    p.ok = !p.host.isEmpty();
    return p;
}

QString RemoteFileSystem::join(const QString& a, const QString& b) {
    if (a.isEmpty()) return b;
    if (b.isEmpty()) return a;
    return normalize(a + "/" + b);
}

QString RemoteFileSystem::parent(const QString& path) {
    if (path.isEmpty() || path == "/") return "/";
    QString p = normalize(path);
    const int i = p.lastIndexOf('/');
    if (i <= 0) return "/";
    return p.left(i);
}

QString RemoteFileSystem::fileName(const QString& path) {
    const int i = path.lastIndexOf('/');
    return (i < 0) ? path : path.mid(i + 1);
}

QString RemoteFileSystem::normalize(const QString& path) {
    const bool abs = path.startsWith('/');
    QStringList out;
    for (const QString& seg : path.split('/')) {
        if (seg.isEmpty() || seg == ".") continue;
        if (seg == "..") { if (!out.isEmpty()) out.removeLast(); continue; }
        out << seg;
    }
    QString r = out.join('/');
    if (abs) r.prepend('/');
    return r.isEmpty() ? (abs ? "/" : ".") : r;
}

QString RemoteFileSystem::quote(const QString& path) {
    return "'" + QString(path).replace("'", "'\"'\"'") + "'";
}

QList<RemoteEntry> RemoteFileSystem::parseLs(const QString& dir, const QString& output) {
    QList<RemoteEntry> out;
    // full-iso: "-rw-r--r-- 1 ali ali 123 2026-01-02 15:04:05.000000000 +0300 ad"
    static QRegularExpression re(
        R"(^([bcdlps-])[rwxstST-]{9}\s+\d+\s+\S+\s+\S+\s+(\d+)\s+(\d{4}-\d{2}-\d{2})\s+(\d{2}:\d{2}:\d{2})(?:\.\d+)?\s+[+-]\d{4}\s+(.+)$)");
    static QRegularExpression ansi("\x1b\\[[0-9;]*[A-Za-z]");
    for (QString line : output.split('\n')) {
        line.remove(ansi); // uzak ls --color kalıntısı savunması
        auto m = re.match(line.trimmed());
        if (!m.hasMatch()) continue;
        const QString name = m.captured(5);
        if (name == "." || name == "..") continue;
        RemoteEntry e;
        e.name = name;
        e.path = join(dir, name);
        e.isDir = (m.captured(1) == "d");
        e.size = m.captured(2).toLongLong();
        e.mtime = QDateTime::fromString(m.captured(3) + " " + m.captured(4),
                                        "yyyy-MM-dd HH:mm:ss");
        e.mtime.setTimeSpec(Qt::UTC);
        out << e;
    }
    return out;
}

bool RemoteFileSystem::parseStat(const QString& output, QDateTime& mtime, qint64& size) {
    const QStringList parts = output.trimmed().split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 2) return false;
    bool ok1 = false, ok2 = false;
    const qint64 epoch = parts[0].toLongLong(&ok1);
    size = parts[1].toLongLong(&ok2);
    if (!ok1 || !ok2) return false;
    mtime = QDateTime::fromSecsSinceEpoch(epoch, Qt::UTC);
    return true;
}

QString RemoteFileSystem::lsCommand(const QString& dir) {
    // `command` öneki: uzak kabuktaki ls takma adı/fonksiyonu (eza vb.) atlanır
    return "command ls -la --color=never --time-style=full-iso " + quote(dir);
}

QString RemoteFileSystem::statCommand(const QString& path) {
    return "stat -c '%Y %s %F' " + quote(path);
}

QString RemoteFileSystem::mkdirCommand(const QString& dir) {
    return "mkdir -p " + quote(dir);
}
