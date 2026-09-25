#pragma once
#include "ConnectionProfile.h"
#include <QProcess>
#include <QString>
#include <QStringList>

// Stage 16: git çalıştırıcı soyutlaması — GitPanel yerel/uzak seçebilir.
// Komut kurma saf (test edilebilir); çalıştırma QProcess/ssh ile.
class GitRunner {
public:
    struct Result { int exit = -1; QString out; QString err; };
    virtual ~GitRunner() = default;
    virtual Result run(const QStringList& args, int timeoutMs = 8000) = 0;
    virtual QString label() const = 0;

    // Uzak git komut satırı kurma (saf)
    static QString remoteGitCmd(const ConnectionProfile& p, const QStringList& args);
};

class LocalGitRunner : public GitRunner {
public:
    explicit LocalGitRunner(const QString& dir) : m_dir(dir) {}
    Result run(const QStringList& args, int timeoutMs = 8000) override;
    QString label() const override { return "yerel"; }

private:
    QString m_dir;
};

class RemoteGitRunner : public GitRunner {
public:
    RemoteGitRunner(const ConnectionProfile& p, QObject* parent = nullptr);
    Result run(const QStringList& args, int timeoutMs = 15000) override;
    QString label() const override { return "uzak:" + m_profile.host; }

private:
    ConnectionProfile m_profile;
};
