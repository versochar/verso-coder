#include "GitRunner.h"
#include "SshSession.h"

QString GitRunner::remoteGitCmd(const ConnectionProfile& p, const QStringList& args) {
    QString cmd = "git";
    if (!p.remoteRoot.trimmed().isEmpty())
        cmd += " -C '" + QString(p.remoteRoot.trimmed()).replace("'", "'\"'\"'") + "'";
    for (const QString& a : args) {
        if (a.contains(' ') || a.contains('\'') || a.contains('"'))
            cmd += " '" + QString(a).replace("'", "'\"'\"'") + "'";
        else
            cmd += " " + a;
    }
    return SshSession::remoteShellCmd(p, cmd, QString());
}

GitRunner::Result LocalGitRunner::run(const QStringList& args, int timeoutMs) {
    Result r;
    QProcess p;
    p.setWorkingDirectory(m_dir);
    p.start("git", args);
    if (!p.waitForFinished(timeoutMs)) { p.kill(); r.exit = -1; r.err = "Zaman aşımı"; return r; }
    r.exit = p.exitCode();
    r.out = QString::fromUtf8(p.readAllStandardOutput());
    r.err = QString::fromUtf8(p.readAllStandardError());
    return r;
}

RemoteGitRunner::RemoteGitRunner(const ConnectionProfile& p, QObject* parent) : m_profile(p) {
    Q_UNUSED(parent);
}

GitRunner::Result RemoteGitRunner::run(const QStringList& args, int timeoutMs) {
    Result r;
    SshSession s;
    s.setProfile(m_profile);
    auto e = s.exec(remoteGitCmd(m_profile, args), QString(), timeoutMs);
    r.exit = e.exit;
    r.out = e.out;
    r.err = e.err;
    return r;
}
