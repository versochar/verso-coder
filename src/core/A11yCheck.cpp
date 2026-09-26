#include "A11yCheck.h"
#include <QtMath>

double A11yCheck::luminance(const QColor& c) {
    auto lin = [](double v) {
        v /= 255.0;
        return (v <= 0.03928) ? v / 12.92 : qPow((v + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * lin(c.red()) + 0.7152 * lin(c.green()) + 0.0722 * lin(c.blue());
}

double A11yCheck::contrastRatio(const QColor& a, const QColor& b) {
    const double l1 = luminance(a), l2 = luminance(b);
    const double hi = qMax(l1, l2), lo = qMin(l1, l2);
    return (hi + 0.05) / (lo + 0.05);
}

bool A11yCheck::textPasses(const QColor& fg, const QColor& bg, bool large) {
    return contrastRatio(fg, bg) >= (large ? 3.0 : 4.5);
}

QStringList A11yCheck::checkTheme(const ThemeTokens& tk) {
    QStringList issues;
    if (!textPasses(tk.textStrong, tk.bg, true))
        issues << "Başlık metni/bg kontrastı düşük (< 3.0)";
    if (!textPasses(tk.text, tk.bg))
        issues << "Gövde metni/bg kontrastı düşük (< 4.5)";
    if (!textPasses(tk.textDim, tk.bg))
        issues << "Silk metin/bg kontrastı düşük (< 4.5)";
    if (!textPasses(tk.accent, tk.bg, true))
        issues << "Vurgu rengi/bg kontrastı düşük (< 3.0)";
    if (!tk.isValid()) issues << "Tema jetonları geçersiz";
    if (issues.isEmpty()) issues << "geçti";
    return issues;
}

bool A11yCheck::fontSizeOk(int px) {
    return px >= 8 && px <= 24;
}
