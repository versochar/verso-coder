#pragma once
#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>
#include <QStringList>

// Stage 9: SVG ikon seti + tema-uyumlu renklendirme.
// resources/icons/<ad>.svg dosyaları okunur, içindeki "currentColor"
// hedef renge çevrilir ve QSvgRenderer ile çizilir (DPI/ölçek uyumlu).
class IconTheme {
public:
    static QString iconsDir();
    static bool hasIcon(const QString& name);
    static QStringList availableIcons();

    static QPixmap pixmap(const QString& name, const QColor& color, int size);
    static QIcon icon(const QString& name, const QColor& color, int size = 18);

    static void clearCache();

private:
    static QByteArray recoloredSvg(const QString& name, const QColor& color, QString* err);
    static QString cacheKey(const QString& name, const QColor& c, int size);
};
