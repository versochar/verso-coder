#include "CacheCleaner.h"
#include "ai/ModelPool.h"
#include <QFileInfo>
#include <QStandardPaths>
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

// --- Stage 38: AI önbellekleri ---
namespace {
QString appDataFile(const QString& name) {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/')
           + name;
}
qint64 fileBytes(const QString& path) {
    QFileInfo fi(path);
    return fi.exists() ? fi.size() : 0;
}
} // namespace

QList<CacheCleaner::AiEntry> CacheCleaner::aiCacheEntries() {
    QList<AiEntry> out;
    out << AiEntry{QStringLiteral("catalog"), QStringLiteral("Model kataloğu"),
                   appDataFile(QStringLiteral("model-catalog.json")),
                   fileBytes(appDataFile(QStringLiteral("model-catalog.json"))), true};
    out << AiEntry{QStringLiteral("health"), QStringLiteral("Sağlık geçmişi"),
                   appDataFile(QStringLiteral("ai-health.json")),
                   fileBytes(appDataFile(QStringLiteral("ai-health.json"))), true};
    out << AiEntry{QStringLiteral("audit"), QStringLiteral("Komut denetimi"),
                   appDataFile(QStringLiteral("command-audit.json")),
                   fileBytes(appDataFile(QStringLiteral("command-audit.json"))), true};
    out << AiEntry{QStringLiteral("usage"), QStringLiteral("Token kullanım geçmişi"),
                   appDataFile(QStringLiteral("ai-usage.json")),
                   fileBytes(appDataFile(QStringLiteral("ai-usage.json"))), false};
    out << AiEntry{QStringLiteral("keys"), QStringLiteral("API anahtarı kasası"),
                   appDataFile(QStringLiteral("ai-keys.json")),
                   fileBytes(appDataFile(QStringLiteral("ai-keys.json"))), false};
    return out;
}

qint64 CacheCleaner::aiCacheBytes() {
    qint64 total = 0;
    for (const AiEntry& e : aiCacheEntries()) total += e.bytes;
    return total;
}

bool CacheCleaner::cleanAiCache(bool includeHistory) {
    bool any = false;
    for (const AiEntry& e : aiCacheEntries()) {
        if (!e.removable && !includeHistory) continue;
        // Anahtar kasası ASLA silinmez (güvenlik)
        if (e.id == QLatin1String("keys")) continue;
        if (!QFile::exists(e.path)) continue;
        const qint64 before = e.bytes;
        if (QFile::remove(e.path)) {
            any = true;
            emit cleaned(before);
        }
    }
    ModelPool::clear();
    return any;
}

QString CacheCleaner::totalSummary() const {
    const qint64 ai = aiCacheBytes();
    return QStringLiteral("AI önbelleği: %1 KB (%2 dosya)")
        .arg(ai / 1024)
        .arg(aiCacheEntries().size());
}
