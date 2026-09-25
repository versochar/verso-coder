#include "CacheCleaner.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <climits>

CacheCleaner::CacheCleaner(QObject* parent) : QObject(parent) {}

bool CacheCleaner::cleanClangdCache() {
    // Clangd cache'ini temizle (genellikle ~/.cache/clangd veya proje içinde .clangd)
    const QString cacheHome = QDir::homePath() + "/.cache/clangd";
    QDir cdir(cacheHome);
    if (cdir.exists()) cdir.removeRecursively();

    // Project-level .clangd cache
    if (!m_versoRoot.isEmpty()) {
        QDir pcdir(m_versoRoot + "/.clangd");
        if (pcdir.exists()) pcdir.removeRecursively();
    }

    return true;
}

bool CacheCleaner::cleanTempFiles() {
    // Geçici dosyaları temizle (*.swp, *.swo, *.bak, *.tmp).
    // Not: .git içeriğine dokunulmaz (reflog koruması).
    const QStringList patterns = {"*.swp", "*.swo", "*.bak", "*.tmp"};
    qint64 totalFreed = 0;
    
    // Geçici klasörler
    QStringList tmpDirs = {
        QDir::tempPath(),
        QCoreApplication::applicationDirPath() + "/tmp"
    };
    
    for (const QString& dirPath : tmpDirs) {
        QDir dir(dirPath);
        if (!dir.exists()) continue;

        const QStringList files = dir.entryList(patterns, QDir::Files | QDir::Hidden);
        for (const QString& file : files) {
            const QString fullPath = dir.filePath(file);
            const qint64 size = QFileInfo(fullPath).size();
            if (QFile::remove(fullPath)) totalFreed += size;
        }
    }

    emit cleaned(totalFreed > INT_MAX ? INT_MAX : static_cast<int>(totalFreed));
    return true;
}

bool CacheCleaner::cleanAll() {
    bool ok1 = cleanClangdCache();
    bool ok2 = cleanTempFiles();
    return ok1 && ok2;
}
