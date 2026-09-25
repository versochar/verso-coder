#pragma once
#include <QString>
#include <QStringList>

// Stage 12: tema JSON doğrulama — canlı düzenleyici ve AI üretici için
// güvenlik ağı (bozuk tema tüm QSS'i bozmasın).
class ThemeValidator {
public:
    // json bir tema paleti mi? Hata varsa error dolar.
    static bool validate(const QString& json, QString* error = nullptr);
    // Serbest metinden ilk {...} JSON bloğunu çıkarır (AI yanıtları için)
    static QString extractJson(const QString& text);
    static QStringList requiredKeys();
};
