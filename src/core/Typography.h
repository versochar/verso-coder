#pragma once
#include <QFont>
#include <QString>

// Stage 9: tipografi — UI ve editör fontu ayrı; aile/boyut/satır yüksekliği/
// harf aralığı/ligature ayarlarından gerçek QFont üretilir.
struct TypographySettings {
    QString uiFamily;             // boşsa sistem varsayılanı
    int     uiSize = 13;          // px
    QString editorFamily;         // boşsa "JetBrains Mono, Consolas, monospace"
    int     editorSize = 11;      // pt (mevcut davranışla uyumlu)
    double  lineHeight = 1.0;     // 1.0..2.0 (editör)
    double  letterSpacing = 0.0;  // yüzde, ör. 5 = +%5
    bool    ligatures = true;     // OpenType ligature (destekleyen fontlarla)
};

class Typography {
public:
    static QFont uiFont(const TypographySettings& t);
    static QFont editorFont(const TypographySettings& t);
    static int lineSpacingPx(const QFont& f, double lineHeight);
    static double clampLineHeight(double v);
    static double clampLetterSpacing(double v);
    static QStringList editorFontSuggestions();
};
