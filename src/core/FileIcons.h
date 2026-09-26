#pragma once
#include <QCache>
#include <QColor>
#include <QFileIconProvider>
#include <QIcon>

// Uzantıya göre renkli rozet ikonlar (koyu temaya uygun).
class FileIconProvider : public QFileIconProvider {
public:
    QIcon icon(IconType type) const override;
    QIcon icon(const QFileInfo& info) const override;

    static QColor colorFor(const QString& suffix);

private:
    mutable QCache<QString, QIcon> m_cache{256}; // Stage 32: ikon önbelleği
};
