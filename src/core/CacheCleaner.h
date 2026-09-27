#pragma once
#include <QString>
#include <QList>
#include <QObject>

class CacheCleaner : public QObject {
    Q_OBJECT
public:
    explicit CacheCleaner(QObject* parent = nullptr);
    ~CacheCleaner() override = default;

    void setRepoRoot(const QString& root) { m_versoRoot = root; }

    // Clangd cache'ini temizle
    bool cleanClangdCache();

    // Geçici dosyaları temizle (git tmp, *.swp, *.bak gibi)
    bool cleanTempFiles();

    // Tüm cache'leri temizle
    bool cleanAll();

    // --- Stage 38: AI önbellekleri ---
    // AI tarafının ürettiği dosyalar (model kataloğu, gömme önbelleği, kullanım
    // geçmişi, sağlık, komut denetimi) tek yerde toplanır; boyut raporuyla.
    struct AiEntry {
        QString id;
        QString label;
        QString path;
        qint64 bytes = 0;
        bool removable = false; // kullanım geçmişi silinmez (veri kaybı)
    };
    static QList<AiEntry> aiCacheEntries();
    static qint64 aiCacheBytes();
    bool cleanAiCache(bool includeHistory = false);
    // Tüm önbelleklerin (clangd + AI) toplam özeti
    QString totalSummary() const;

signals:
    void cleaned(int bytesFreed);
    void errorOccurred(const QString& message);

private:
    QString m_versoRoot;
};
