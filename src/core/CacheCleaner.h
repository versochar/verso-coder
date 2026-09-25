#pragma once
#include <QString>
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

signals:
    void cleaned(int bytesFreed);
    void errorOccurred(const QString& message);

private:
    QString m_versoRoot;
};
