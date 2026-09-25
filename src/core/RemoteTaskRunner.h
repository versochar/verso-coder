#pragma once
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

// Stage 16: uzak görev çalıştırma — komut kurma + çıktı ayrıştırma (saf, test edilebilir).
// Gerçek çalıştırma SshSession::exec üzerinden yapılır; sonuç ProblemMatcher'a verilir.
class RemoteTaskRunner {
public:
    // gcc/clang çıktısından dosya:satır:sütun yakalama (yerel derleyiciyle aynı format)
    struct TaskIssue { QString file; int line1 = 1; int col1 = 1; QString message; QString kind; };
    static QList<TaskIssue> parseBuildOutput(const QString& output, const QString& remoteRoot,
                                             const QString& localMirror = QString());
    // Uzak köke göre komut sarmalama (cd + env)
    static QString wrapCommand(const QString& command, const QString& remoteRoot,
                               const QMap<QString, QString>& env = {});
    // Çıkış kodu + sinyal yorumu
    static QString describeExit(int code);
};
