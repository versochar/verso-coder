#pragma once
#include "ConnectionProfile.h"
#include <QObject>
#include <QProcess>
#include <QStringList>

// Stage 16: SSH oturumu — OpenSSH CLI tabanlı (ssh/sftp), libssh2 varsa
// CMake VERSO_HAVE_LIBSSH2 tanımıyla native yola geçilebilir (API aynı).
// Test edilebilirlik için komut kurma statik metotlarda (süreçsiz) tutulur.
class SshSession : public QObject {
    Q_OBJECT
public:
    explicit SshSession(QObject* parent = nullptr);

    void setProfile(const ConnectionProfile& p) { m_profile = p; }
    const ConnectionProfile& profile() const { return m_profile; }
    bool isConnected() const { return m_connected; }
    QString lastError() const { return m_error; }

    // Bağlantı denetimi: `ssh ... true` (engelleyici, timeoutMs)
    bool testConnection(int timeoutMs = 8000);
    void disconnect();

    // Uzakta komut çalıştır (engelleyici). cwd boşsa profil kökü kullanılır.
    struct ExecResult { int exit = -1; QString out; QString err; };
    ExecResult exec(const QString& command, const QString& cwd = QString(),
                    int timeoutMs = 30000);
    // Eşzamansız çalıştırma (uzun görevler/terminal dışı): bitince finished
    QProcess* execAsync(const QString& command, const QString& cwd = QString());

    // Dosya aktarımı (sftp batch): local <-> remote
    bool upload(const QString& local, const QString& remote, int timeoutMs = 60000);
    bool download(const QString& remote, const QString& local, int timeoutMs = 60000);
    // Hızlı içerik okuma/yazma (küçük dosyalar): ssh + cat / heredoc
    ExecResult readFile(const QString& remotePath, int timeoutMs = 15000);
    ExecResult writeFile(const QString& remotePath, const QString& content,
                         int timeoutMs = 15000);

    // --- Saf komut kurma (birim testler için) ---
    static QStringList sshBaseArgs(const ConnectionProfile& p);
    static QString sshTarget(const ConnectionProfile& p); // [user@]host
    static QString remoteShellCmd(const ConnectionProfile& p, const QString& command,
                                  const QString& cwd);
    static QStringList sftpBatchArgs(const ConnectionProfile& p);
    static bool haveNative(); // libssh2 derleme desteği

signals:
    void connected();
    void disconnected();
    void error(const QString& msg);

private:
    ConnectionProfile m_profile;
    bool m_connected = false;
    QString m_error;
};
