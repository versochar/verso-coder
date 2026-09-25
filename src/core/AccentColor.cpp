#include "AccentColor.h"
#include "ThemeTokens.h"

QStringList AccentColor::presetNames() {
    return {"Mavi", "Mor", "Turkuaz", "Yeşil", "Turuncu", "Pembe", "Kırmızı"};
}

QColor AccentColor::preset(const QString& name) {
    if (name == "Mor")        return "#bd93f9";
    if (name == "Turkuaz")    return "#4ec9b0";
    if (name == "Yeşil")      return "#a6e22e";
    if (name == "Turuncu")    return "#ff9e64";
    if (name == "Pembe")      return "#ff79c6";
    if (name == "Kırmızı")    return "#f44747";
    if (name == "Mavi")       return "#007acc";
    return "#007acc"; // varsayılan: mavi
}

QColor AccentColor::hover(const QColor& c, bool dark) {
    return dark ? c.lighter(125) : c.darker(115);
}

QColor AccentColor::pressed(const QColor& c, bool dark) {
    return dark ? c.lighter(150) : c.darker(140);
}

QColor AccentColor::soft(const QColor& c, const QColor& bg, double t) {
    return ThemeTokens::mix(bg, c, t);
}
