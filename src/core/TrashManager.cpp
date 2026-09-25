#include "TrashManager.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

TrashManager::TrashManager(const QString& trashDir)
    : m_dir(trashDir.isEmpty() ? defaultDir() : trashDir) {
    QDir().mkpath(m_dir);
}

QString TrashManager::defaultDir() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/trash";
}

static bool copyDirRecursive(const QString& src, const QString& dst) {
    QDir().mkpath(dst);
    QDir s(src);
    for (const QString& f : s.entryList(QDir::Files | QDir::NoDotAndDotDot)) {
        if (!QFile::copy(src + "/" + f, dst + "/" + f)) return false;
    }
    for (const QString& d : s.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (!copyDirRecursive(src + "/" + d, dst + "/" + d)) return false;
    }
    return true;
}

bool TrashManager::trash(const QString& absPath) {
    QFileInfo fi(absPath);
    if (!fi.exists()) return false;
    const bool isDir = fi.isDir(); // taşımadan ÖNCE (QFileInfo önbelleği)
    QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss-zzz");
    QString id = stamp + "-" + fi.fileName();
    QString dest = m_dir + "/" + id;
    QDir().mkpath(dest);
    QString target = dest + "/" + fi.fileName();
    bool ok = QFile::rename(absPath, target); // aynı fs: dosya + dizin
    if (!ok) { // farklı fs: kopyala + sil
        if (isDir) {
            ok = copyDirRecursive(absPath, target);
            if (ok) QDir(absPath).removeRecursively();
        } else {
            ok = QFile::copy(absPath, target);
            if (ok) QFile::remove(absPath);
        }
    }
    if (!ok) { QDir(dest).removeRecursively(); return false; }
    // Meta yaz
    QSettings meta(metaPath(id), QSettings::IniFormat);
    meta.setValue("origPath", absPath);
    meta.setValue("when", QDateTime::currentDateTime());
    meta.setValue("isDir", isDir);
    return true;
}

QString TrashManager::metaPath(const QString& id) const {
    return m_dir + "/" + id + ".meta";
}

QList<TrashManager::Entry> TrashManager::list() const {
    QList<Entry> out;
    QDir d(m_dir);
    if (!d.exists()) return out;
    for (const QString& e : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time)) {
        QSettings meta(metaPath(e), QSettings::IniFormat);
        QString orig = meta.value("origPath").toString();
        if (orig.isEmpty()) continue;
        Entry en;
        en.id = e;
        en.origPath = orig;
        en.name = QFileInfo(orig).fileName();
        en.when = meta.value("when").toDateTime();
        en.isDir = meta.value("isDir").toBool();
        out << en;
    }
    return out;
}

bool TrashManager::restore(const QString& id) {
    QSettings meta(metaPath(id), QSettings::IniFormat);
    QString orig = meta.value("origPath").toString();
    if (orig.isEmpty()) return false;
    QString srcDir = m_dir + "/" + id;
    QString name = QFileInfo(orig).fileName();
    QString src = srcDir + "/" + name;
    if (!QFileInfo::exists(src)) return false;
    QString dst = orig;
    if (QFileInfo::exists(dst)) {
        int k = 1;
        QString base = orig + QString("-geri%1");
        while (QFileInfo::exists(base.arg(k))) ++k;
        dst = base.arg(k);
    }
    QDir().mkpath(QFileInfo(dst).dir().absolutePath());
    bool ok = QDir().rename(src, dst) || QFile::rename(src, dst);
    if (!ok) return false;
    QDir(srcDir).removeRecursively();
    QFile::remove(metaPath(id));
    return true;
}

bool TrashManager::clear() {
    bool ok = true;
    QDir d(m_dir);
    for (const QString& e : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QDir(m_dir + "/" + e).removeRecursively();
        QFile::remove(metaPath(e));
    }
    return ok;
}
