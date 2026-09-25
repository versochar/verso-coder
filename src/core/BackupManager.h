#pragma once
#include <QList>
#include <QString>

struct BackupEntry {
    QString backupPath; // .bak tam yolu
    QString original;   // özgün dosya tam yolu
    qint64 whenMs = 0;
    qint64 size = 0;
};

// Zaman damgalı yedekler + kurtarma (crash/otomatik kaydetme için).
class BackupManager {
public:
    explicit BackupManager(const QString& dir);

    QString save(const QString& originalPath, const QString& content); // yedek yolu
    QList<BackupEntry> list() const;                                     // yeniden eskiye
    QString read(const QString& backupPath) const;
    bool restore(const QString& backupPath) const;                       // özgüne geri yazar
    bool remove(const QString& backupPath) const;
    int prune(int keepNewest);
    QString dir() const { return m_dir; }

private:
    QString m_dir;
};
