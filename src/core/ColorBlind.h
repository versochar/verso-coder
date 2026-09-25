#pragma once
#include <QColor>
#include <QString>
#include <QStringList>

class ThemeTokens;

// Stage 12: renk körü dostu palet dönüşümü (Machado 2009 yaklaşıklığı).
// ThemeTokens JSON turu üzerinden uygulanır — yeni renk alanları otomatik kapsanır.
class ColorBlind {
public:
    enum class Mode { None, Deuteranopia, Protanopia, Tritanopia };

    static Mode fromName(const QString& name);
    static QString name(Mode m);
    static QStringList names();
    static QString title(const QString& name);

    static QColor adjust(const QColor& c, Mode m);
    static ThemeTokens applyTo(const ThemeTokens& tk, Mode m);
};
