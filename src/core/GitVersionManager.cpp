#include "GitVersionManager.h"
#include <QProcess>

GitVersionManager::GitVersionManager(QObject* parent) : QObject(parent) {}

QString GitVersionManager::currentVersion() {
    if (m_repoRoot.isEmpty()) {
        emit errorOccurred("Repo kök dizini ayarlanmadı");
        return QString();
    }
    QProcess p;
    p.setWorkingDirectory(m_repoRoot);
    p.start("git", {"describe", "--tags", "--abbrev=0"});
    if (!p.waitForFinished(5000)) {
        emit errorOccurred("Git version alınamadı (zaman aşımı)");
        // Fallback: short log
        p.start("git", {"log", "--oneline", "-1"});
        p.waitForFinished(3000);
        return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    }
    QString tag = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    if (tag.isEmpty()) {
        // No tags, use short commit
        p.start("git", {"rev-parse", "--short", "HEAD"});
        p.waitForFinished(3000);
        return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    }
    // Tag varsa, o kullan
    // Tag'den 'v' prefix'ini temizle
    if (tag.startsWith("v") || tag.startsWith("V"))
        tag = tag.mid(1);
    return tag;
}

bool GitVersionManager::createTag(const QString& tagName, const QString& message) {
    if (m_repoRoot.isEmpty()) {
        emit errorOccurred("Repo kök dizini ayarlanmadı");
        return false;
    }
    QProcess p;
    p.setWorkingDirectory(m_repoRoot);
    QStringList args = {"tag", "-a", tagName, "-m", message};
    p.start("git", args);
    if (!p.waitForFinished(10000)) {
        emit errorOccurred("Tag oluşturulamadı (zaman aşımı)");
        return false;
    }
    if (p.exitCode() != 0) {
        emit errorOccurred(QString("Tag oluşturulamadı: %1").arg(QString::fromUtf8(p.readAllStandardError())));
        return false;
    }
    // Push not done here (opt-in)
    return true;
}

QString GitVersionManager::lastCommitMessage() {
    if (m_repoRoot.isEmpty()) {
        emit errorOccurred("Repo kök dizini ayarlanmadı");
        return QString();
    }
    QProcess p;
    p.setWorkingDirectory(m_repoRoot);
    p.start("git", {"log", "--oneline", "-1"});
    if (!p.waitForFinished(3000)) {
        emit errorOccurred("Commit mesajı alınamadı");
        return QString();
    }
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}

bool GitVersionManager::resetHard() {
    if (m_repoRoot.isEmpty()) {
        emit errorOccurred("Repo kök dizini ayarlanmadı");
        return false;
    }
    QProcess p;
    p.setWorkingDirectory(m_repoRoot);
    p.start("git", {"reset", "--hard"});
    if (!p.waitForFinished(10000)) {
        emit errorOccurred("reset --hard başarısız (zaman aşımı)");
        return false;
    }
    if (p.exitCode() != 0) {
        emit errorOccurred(QString("reset --hard hatası: %1").arg(QString::fromUtf8(p.readAllStandardError())));
        return false;
    }
    return true;
}
