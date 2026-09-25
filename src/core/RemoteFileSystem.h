#pragma once
#include <QDateTime>
#include <QString>
#include <QStringList>

// Stage 16: uzak dosya sistemi — saf yol/URI/çıktı ayrıştırma (süreçsiz, test edilebilir).
// Gerçek I/O SshSession üzerinden MainWindow/RemoteExplorer'da yapılır.
struct RemoteEntry {
    QString name;
    QString path;      // tam uzak yol
    bool isDir = false;
    qint64 size = 0;
    QDateTime mtime;
};

class RemoteFileSystem {
public:
    // ssh://[user@]host[:port]/yol ayrıştır (host,user,port,path)
    struct ParsedUri { QString user; QString host; int port = 22; QString path; bool ok = false; };
    static ParsedUri parseUri(const QString& uri);
    static bool isRemotePath(const QString& path); // "ssh://" öneki

    // Yol yardımcıları (POSIX uzak taraf)
    static QString join(const QString& a, const QString& b);
    static QString parent(const QString& path);
    static QString fileName(const QString& path);
    static QString normalize(const QString& path); // //, /./, /../ temizler
    static QString quote(const QString& path);     // shell tek-tırnak

    // `ls -la --time-style=full-iso` çıktısını ayrıştır
    static QList<RemoteEntry> parseLs(const QString& dir, const QString& output);
    // `stat -c '%Y %s %n'` çıktısı: mtime epoch + boyut
    static bool parseStat(const QString& output, QDateTime& mtime, qint64& size);
    // Dizin listeleme komutu (uzak shell)
    static QString lsCommand(const QString& dir);
    static QString statCommand(const QString& path);
    static QString mkdirCommand(const QString& dir);
};
