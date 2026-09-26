#include "BackupManager.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

BackupManager::BackupManager(const QString& dir) : m_dir(dir) {
    QDir().mkpath(m_dir);
}

static QString hashOf(const QString& s) {
    return QString::fromLatin1(QCryptographicHash::hash(s.toUtf8(), QCryptographicHash::Sha1).toHex().left(14));
}

QString BackupManager::save(const QString& originalPath, const QString& content) {
    // Stage 33 (kararlılık): aynı milisaniyedeki ardışık yedeklerde damgayı tekilleştir,
    // böylece sıralama ve kota budama deterministik olur (eskiler doğru seçilir).
    static qint64 g_lastBackupMs = 0;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now <= g_lastBackupMs) now = g_lastBackupMs + 1;
    g_lastBackupMs = now;
    const QString stem = QFileInfo(originalPath).fileName();
    const QString base = QString("%1_%2_%3").arg(now).arg(hashOf(originalPath)).arg(stem);
    const QString bak = m_dir + "/" + base + ".bak";
    const QString meta = m_dir + "/" + base + ".meta.json";

    QFile bf(bak);
    if (!bf.open(QIODevice::WriteOnly | QIODevice::Truncate)) return {};
    bf.write(content.toUtf8());
    bf.close();

    QJsonObject o;
    o["original"] = originalPath;
    o["when"] = double(now);
    o["size"] = double(content.toUtf8().size());
    QFile mf(meta);
    if (mf.open(QIODevice::WriteOnly | QIODevice::Truncate))
        mf.write(QJsonDocument(o).toJson(QJsonDocument::Compact));
    return bak;
}

QList<BackupEntry> BackupManager::list() const {
    QList<BackupEntry> out;
    QDir d(m_dir);
    const QStringList metas = d.entryList({"*.meta.json"}, QDir::Files);
    for (const QString& m : metas) {
        QFile f(d.absoluteFilePath(m));
        if (!f.open(QIODevice::ReadOnly)) continue;
        QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
        BackupEntry e;
        e.original = o.value("original").toString();
        e.whenMs = qint64(o.value("when").toDouble());
        e.size = qint64(o.value("size").toDouble());
        const QString base = m.left(m.size() - QString(".meta.json").size());
        e.backupPath = d.absoluteFilePath(base + ".bak");
        if (!QFile::exists(e.backupPath)) continue;
        out << e;
    }
    std::sort(out.begin(), out.end(), [](const BackupEntry& a, const BackupEntry& b) {
        if (a.whenMs != b.whenMs) return a.whenMs > b.whenMs; // yeniden eskiye
        return a.backupPath > b.backupPath;                   // deterministik eşitlik çözümü
    });
    return out;
}

QString BackupManager::read(const QString& backupPath) const {
    QFile f(backupPath);
    if (!f.open(QIODevice::ReadOnly)) return {};
    return QString::fromUtf8(f.readAll());
}

bool BackupManager::restore(const QString& backupPath) const {
    const QString meta = backupPath.left(backupPath.size() - 4) + ".meta.json";
    QFile mf(meta);
    if (!mf.open(QIODevice::ReadOnly)) return false;
    const QString original = QJsonDocument::fromJson(mf.readAll()).object().value("original").toString();
    if (original.isEmpty()) return false;
    QDir().mkpath(QFileInfo(original).absolutePath());
    QFile of(original);
    if (!of.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    of.write(read(backupPath).toUtf8());
    return true;
}

bool BackupManager::remove(const QString& backupPath) const {
    QFile::remove(backupPath);
    const QString meta = backupPath.left(backupPath.size() - 4) + ".meta.json";
    return QFile::remove(meta);
}

int BackupManager::pruneByQuota(qint64 maxBytes, int maxAgeDays) {
    QList<BackupEntry> all = list(); // yeniden eskiye
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    int removed = 0;
    qint64 total = 0;
    for (const BackupEntry& e : all) total += e.size;
    for (int i = all.size() - 1; i >= 0; --i) { // en eskiden başla
        const BackupEntry& e = all[i];
        const bool tooOld = maxAgeDays > 0 && (now - e.whenMs) > qint64(maxAgeDays) * 86400000LL;
        if (tooOld || total > maxBytes) {
            if (remove(e.backupPath)) {
                ++removed;
                total -= e.size;
            }
        }
    }
    return removed;
}

int BackupManager::prune(int keepNewest) {
    QList<BackupEntry> all = list();
    int removed = 0;
    for (int i = keepNewest; i < all.size(); ++i)
        if (remove(all[i].backupPath)) ++removed;
    return removed;
}
