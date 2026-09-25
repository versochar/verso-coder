#include "RemoteTaskRunner.h"
#include <QRegularExpression>

QList<RemoteTaskRunner::TaskIssue> RemoteTaskRunner::parseBuildOutput(
    const QString& output, const QString& remoteRoot, const QString& localMirror) {
    QList<TaskIssue> out;
    static QRegularExpression re(
        R"(^(.+\.(?:cpp|c|cc|h|hpp|py|rs|go|js|ts))[:\(](\d+)(?::|\s*,\s*|\()(\d+)?[^:]*:\s*(error|warning|hata|uyarı)?\s*:?\s*(.*)$)");
    for (const QString& raw : output.split('\n')) {
        const QString line = raw.trimmed();
        if (line.isEmpty()) continue;
        auto m = re.match(line);
        if (!m.hasMatch()) continue;
        TaskIssue t;
        t.file = m.captured(1);
        // Uzak mutlak yolu yerel aynaya çevir (varsa)
        if (!localMirror.isEmpty() && !remoteRoot.isEmpty()
            && t.file.startsWith(remoteRoot))
            t.file = localMirror + t.file.mid(remoteRoot.size());
        t.line1 = qMax(1, m.captured(2).toInt());
        t.col1 = m.captured(3).isEmpty() ? 1 : qMax(1, m.captured(3).toInt());
        const QString kind = m.captured(4).toLower();
        t.kind = (kind.contains("warn") || kind.contains("uyar")) ? "warning" : "error";
        t.message = m.captured(5).trimmed().left(300);
        out << t;
    }
    return out;
}

QString RemoteTaskRunner::wrapCommand(const QString& command, const QString& remoteRoot,
                                      const QMap<QString, QString>& env) {
    QString prefix;
    for (auto it = env.begin(); it != env.end(); ++it)
        prefix += it.key() + "='" + QString(it.value()).replace("'", "'\"'\"'") + "' ";
    auto sh = [](const QString& s) {
        return "'" + QString(s).replace("'", "'\"'\"'") + "'";
    };
    if (remoteRoot.trimmed().isEmpty()) return prefix + command;
    return prefix + "cd " + sh(remoteRoot.trimmed()) + " && " + command;
}

QString RemoteTaskRunner::describeExit(int code) {
    if (code == 0) return "başarılı";
    if (code < 0) return "zaman aşımı / öldürüldü";
    if (code > 128) return QString("sinyal ile öldü (SIG%1)").arg(code - 128);
    return QString("kod %1 ile çıktı").arg(code);
}
