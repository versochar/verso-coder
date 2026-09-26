#pragma once
#include <QColor>
#include <QString>
#include <QStringList>

#include "ThemeTokens.h"

// Stage 22: erişilebilirlik denetimi — WCAG kontrast oranı + tema kontrolleri.
// Saf mantık (test edilebilir); sonuçlar DiagnosticsDialog'da gösterilir.
class A11yCheck {
public:
    // WCAG 2.x göreli parlaklık + kontrast oranı (1.0 – 21.0)
    static double luminance(const QColor& c);
    static double contrastRatio(const QColor& a, const QColor& b);
    // Metin için eşikler: normal metin 4.5, büyük/kalın metin 3.0
    static bool textPasses(const QColor& fg, const QColor& bg, bool large = false);
    // Tema jetonlarını denetle → "geçti" / sorun satırları
    static QStringList checkTheme(const ThemeTokens& tk);
    // Font ölçeği makul mü? (8–24 px)
    static bool fontSizeOk(int px);
};
