#include "SshSession.h"
#include <QTemporaryFile>

SshSession::SshSession(QObject* parent) : QObject(parent) {}

QString SshSession::sshTarget(const ConnectionProfile& p) {
    return p.user.isEmpty() ? p.host.trimmed()
                            : p.user.trimmed() + "@" + p.host.trimmed();
}

QStringList SshSession::sshBaseArgs(const ConnectionProfile& p) {
    QStringList a;
    if (p.port != 22 && p.port > 0) a << "-p" << QString::number(p.port);
    if (!p.keyPath.trimmed().isEmpty()) a << "-i" << p.keyPath.trimmed();
    if (!p.jumpHost.trimmed().isEmpty()) a << "-J" << p.jumpHost.trimmed();
    // Toplu kip + host key: değişiklikte kesinlikle dur; ilk anahtar profile bağlı
    a << "-o" << "BatchMode=yes"
      << "-o" << "ConnectTimeout=10"
      << "-o" << "ServerAliveInterval=30"
      << "-o" << "ServerAliveCountMax=3"
      << "-o" << (p.trustNewHosts ? "StrictHostKeyChecking=accept-new"
                                  : "StrictHostKeyChecking=yes");
    return a;
}

QString SshSession::remoteShellCmd(const ConnectionProfile& p, const QString& command,
                                   const QString& cwd) {
    QString root = cwd.isEmpty() ? p.remoteRoot.trimmed() : cwd.trimmed();
    // Tek tırnak kaçışı: ' -> '"'"'
    auto sh = [](const QString& s) {
        return "'" + QString(s).replace("'", "'\"'\"'") + "'";
    };
    if (root.isEmpty()) return command;
    return "cd " + sh(root) + " && " + command;
}

QStringList SshSession::sftpBatchArgs(const ConnectionProfile& p) {
    QStringList a;
    if (p.port != 22 && p.port > 0) a << "-P" << QString::number(p.port);
    if (!p.keyPath.trimmed().isEmpty()) a << "-i" << p.keyPath.trimmed();
    if (!p.jumpHost.trimmed().isEmpty()) a << "-J" << p.jumpHost.trimmed();
    a << "-o" << "BatchMode=yes" << "-o" << "ConnectTimeout=10";
    a << "-b" << "-"; // stdin'den batch
    a << sshTarget(p);
    return a;
}

bool SshSession::haveNative() {
#ifdef VERSO_HAVE_LIBSSH2
    return true;
#else
    return false;
#endif
}

bool SshSession::testConnection(int timeoutMs) {
    if (!m_profile.isValid()) {
        m_error = "Profil geçersiz (host yok).";
        return false;
    }
    QProcess p;
    QStringList args = sshBaseArgs(m_profile);
    args << sshTarget(m_profile) << "true";
    p.start("ssh", args);
    const bool ok = p.waitForFinished(timeoutMs) && p.exitCode() == 0;
    if (!ok) {
        m_error = QString::fromUtf8(p.readAllStandardError()).trimmed();
        if (m_error.isEmpty()) m_error = "ssh bağlanamadı (kod " + QString::number(p.exitCode()) + ")";
        emit error(m_error);
        m_connected = false;
        return false;
    }
    m_connected = true;
    m_error.clear();
    emit connected();
    return true;
}

void SshSession::disconnect() {
    m_connected = false;
    emit disconnected();
}

SshSession::ExecResult SshSession::exec(const QString& command, const QString& cwd,
                                        int timeoutMs) {
    ExecResult r;
    QProcess p;
    QStringList args = sshBaseArgs(m_profile);
    args << sshTarget(m_profile) << remoteShellCmd(m_profile, command, cwd);
    p.start("ssh", args);
    if (!p.waitForFinished(timeoutMs)) {
        p.kill();
        r.exit = -1;
        r.err = "Zaman aşımı (" + QString::number(timeoutMs) + " ms)";
        return r;
    }
    r.exit = p.exitCode();
    r.out = QString::fromUtf8(p.readAllStandardOutput());
    r.err = QString::fromUtf8(p.readAllStandardError());
    return r;
}

QProcess* SshSession::execAsync(const QString& command, const QString& cwd) {
    auto* p = new QProcess(this);
    QStringList args = sshBaseArgs(m_profile);
    args << sshTarget(m_profile) << remoteShellCmd(m_profile, command, cwd);
    p->start("ssh", args);
    return p;
}

// Stage 44: sftp toplu-iş alıntısı. Yol içinde " ya da \ varsa çıplak
// çift tırnak komutu kırıyordu; kaçışlı sürüm kullanılır.
QString SshSession::sftpQuote(const QString& path) {
    QString q = path;
    q.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    q.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return QStringLiteral("\"") + q + QStringLiteral("\"");
}

bool SshSession::upload(const QString& local, const QString& remote, int timeoutMs) {
    QProcess p;
    QStringList args = sftpBatchArgs(m_profile);
    p.start("sftp", args);
    if (!p.waitForStarted(8000)) return false;
    const QString batch =
        "put " + sftpQuote(local) + " " + sftpQuote(remote) + "\nbye\n";
    p.write(batch.toUtf8());
    p.closeWriteChannel();
    if (!p.waitForFinished(timeoutMs)) { p.kill(); return false; }
    return p.exitCode() == 0;
}

bool SshSession::download(const QString& remote, const QString& local, int timeoutMs) {
    QProcess p;
    QStringList args = sftpBatchArgs(m_profile);
    p.start("sftp", args);
    if (!p.waitForStarted(8000)) return false;
    const QString batch =
        "get " + sftpQuote(remote) + " " + sftpQuote(local) + "\nbye\n";
    p.write(batch.toUtf8());
    p.closeWriteChannel();
    if (!p.waitForFinished(timeoutMs)) { p.kill(); return false; }
    return p.exitCode() == 0;
}

SshSession::ExecResult SshSession::readFile(const QString& remotePath, int timeoutMs) {
    const QString q = "'" + QString(remotePath).replace("'", "'\"'\"'") + "'";
    return exec("cat " + q, QString(), timeoutMs);
}

SshSession::ExecResult SshSession::writeFile(const QString& remotePath,
                                             const QString& content, int timeoutMs) {
    // Küçük dosyalar için base64 hattı (özel karakter güvenli)
    QProcess p;
    QStringList args = sshBaseArgs(m_profile);
    const QString q = "'" + QString(remotePath).replace("'", "'\"'\"'") + "'";
    args << sshTarget(m_profile)
         << remoteShellCmd(m_profile, "base64 -d > " + q, QString());
    p.start("ssh", args);
    ExecResult r;
    if (!p.waitForStarted(8000)) { r.exit = -1; r.err = "ssh başlatılamadı"; return r; }
    p.write(content.toUtf8().toBase64());
    p.closeWriteChannel();
    if (!p.waitForFinished(timeoutMs)) { p.kill(); r.exit = -1; r.err = "Zaman aşımı"; return r; }
    r.exit = p.exitCode();
    r.out = QString::fromUtf8(p.readAllStandardOutput());
    r.err = QString::fromUtf8(p.readAllStandardError());
    return r;
}
