#pragma once
#include "LlmProvider.h"
#include <QString>

// Stage 36: sağlayıcı + model bazlı fiyatlandırma.
// Ücretsiz katman (`:free`), yerel sunucular ve NIM geliştirme kredisi
// her zaman 0 USD döner — yanlışlıkla ücret göstermeyi engeller.
struct ModelPrice {
    double in = 0.0;        // USD / 1M giriş token
    double out = 0.0;       // USD / 1M çıkış token
    double cachedIn = 0.0;  // USD / 1M önbellekten okunan giriş token
    bool known = false;     // fiyat biliniyor mu
    bool freeTier = false;  // :free / yerel / geliştirme kredisi

    bool isZero() const { return in == 0.0 && out == 0.0; }
};

class ProviderPricing {
public:
    // Sağlayıcı + model → fiyat. Önce ücretsiz katman kontrolü, sonra
    // sağlayıcı tablosu, en son genel model adı sezgisi.
    static ModelPrice priceFor(const ProviderSpec& spec, const QString& model);
    static ModelPrice priceFor(const QString& providerId, const QString& model);

    // Tahmini maliyet. cachedTokens kadar giriş önbellekten okunmuş sayılır.
    static double estimateUsd(const ProviderSpec& spec, const QString& model, int promptTokens,
                              int evalTokens, int cachedTokens = 0, bool batch = false);

    // Ücretsiz mi? (yerel sağlayıcı, `:free` soneki, NIM geliştirme kredisi)
    static bool isFreeTier(const ProviderSpec& spec, const QString& model);

    // Toplu işlem (batch) indirimi katsayısı.
    static double batchFactor() { return 0.5; }

    // Kullanıcıya gösterilecek açıklama: "ücretsiz", "≈$X / 1M giriş", "fiyat bilinmiyor"
    static QString explain(const ProviderSpec& spec, const QString& model);

    // Bilinmeyen model için fiyat elle girilebilir mi? (yalnız ücretli sağlayıcı)
    static bool priceOverridable(const ProviderSpec& spec);
    // Elle girilen fiyatı uygular (bilinmeyen model için); 0 = fiyat yok say.
    static ModelPrice withOverride(const ModelPrice& base, double inPerM, double outPerM);
};
