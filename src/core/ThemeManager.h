#pragma once
#include <QApplication>
#include <QColor>
#include <QString>
#include <QStringList>
#include <functional>
#include "ThemeTokens.h"
#include "Typography.h"

// Tema yöneticisi. Stage 9'dan itibaren iki yol desteklenir:
//  1) JSON palet (ThemeStore) → QssBuilder ile QSS üretimi (ana yol)
//  2) eski .qss dosyaları (custom.qss içe aktarma uyumluluğu)
class ThemeManager {
public:
    static ThemeManager& instance();

    void apply(const QString& themeName);
    QString current() const { return m_current; }
    QStringList availableThemes() const;
    bool importCustomTheme(const QString& srcPath); // custom.qss olarak kopyalar (eski)

    // --- Stage 9 ---
    void setAccent(const QColor& c);            // geçersiz renk → temanın kendi rengi
    QColor accent() const { return m_accent; }  // geçersiz olabilir
    void setTypography(const TypographySettings& t);
    const TypographySettings& typography() const { return m_typo; }
    ThemeTokens tokens() const;                 // etkin (accent uygulanmış) token'lar
    void onApplied(std::function<void()> cb);   // tema/accent değişince çağrılır

    // --- Stage 12 ---
    // Canlı tema düzenleyici önizlemesi: kaydetmeden token'ları uygula.
    void previewTokens(const ThemeTokens& tk);
    void clearPreview();                        // kayıtlı temaya dön
    bool hasPreview() const { return m_hasPreview; }
    // Renk körü modu: tokens() çıktısını dönüştürür ("none" = kapalı)
    void setVisionMode(const QString& mode) { m_vision = mode; }
    QString visionMode() const { return m_vision; }
    // Stage 23: seçim zemini opaklığı (QSS'e işlenir)
    void setSelectionOpacity(double o) { m_selOpacity = qBound(0.05, o, 1.0); }

private:
    ThemeManager() = default;
    static QString customPath();
    void applyTokens();      // token → QSS + font + bildirim
    void notify();

    QString m_current = "dark";
    QColor m_accent;         // geçersiz = tema rengi
    TypographySettings m_typo;
    QList<std::function<void()>> m_callbacks;
    // Stage 12
    ThemeTokens m_preview;
    bool m_hasPreview = false;
    QString m_vision = "none";
    double m_selOpacity = 1.0; // Stage 23
};
