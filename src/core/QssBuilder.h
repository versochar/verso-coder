#pragma once
#include <QColor>
#include <QString>
#include "ThemeTokens.h"
#include "Typography.h"

// Stage 9: ThemeTokens + AccentColor + UiMetrics + Typography → tam uygulama QSS'i.
// Tek doğruluk kaynağı: QSS'ler elle değil buradan üretilir.
class QssBuilder {
public:
    struct Input {
        ThemeTokens tokens;
        QColor accentOverride;        // geçersizse tokens.accent kullanılır
        TypographySettings typo;
        double scale = 1.0;
    };

    static QString build(const Input& in);
    static QColor effectiveAccent(const Input& in); // override ya da tema accent'i
};
