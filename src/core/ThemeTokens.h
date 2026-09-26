#pragma once
#include <QColor>
#include <QJsonObject>
#include <QString>

// Stage 9: token tabanlı tema paleti. Tüm UI (QSS + editör + gutter) buradan okur.
struct ThemeTokens {
    QString name;
    bool dark = true;

    // yüzeyler / metin
    QColor bg, surface, surfaceAlt, border;
    QColor text, textStrong, textDim;
    // markalar / durum
    QColor accent, success, warning, error;
    QColor selection, lineHighlight;
    // editör bölümleri
    QColor gutterBg, gutterText, gutterActive;
    QColor indentGuide, bracket, cursor, scrollbar;
    // sözdizimi
    QColor synKeyword, synString, synComment, synNumber, synFunc, synType;

    static ThemeTokens defaults(bool dark);
    static ThemeTokens fromJson(const QString& json, QString* error = nullptr);
    QJsonObject toJson() const;
    QString toJsonString() const;
    bool isValid() const;

    // renk yardımcıları
    static QColor mix(const QColor& a, const QColor& b, double t); // t=0 → a
    static QColor withAlphaF(const QColor& c, double a);
    // QSS'ye alfa korumalı renk: "#aarrggbb" (QColor::name() alfayı düşürür!)
    static QString css(const QColor& c) { return c.name(QColor::HexArgb); }
};
