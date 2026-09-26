#pragma once
#include <QString>
#include <QtGlobal>

// Oturum boyunca toplanan token kullanımı + maliyet.
// Stage 36: artık sağlayıcı farkındadır — ücretsiz katman (`:free`), yerel
// sunucular ve NIM kredisi 0 USD döner, önbellekten okunan giriş ayrı fiyatlanır.
struct TokenStats {
    qint64 promptTokens = 0;
    qint64 evalTokens = 0;
    qint64 cachedTokens = 0; // önbellekten okunan giriş (bu promptTokens'ın içinde)
    int calls = 0;

    void add(int prompt, int eval, int callCount = 1);
    void addWithCache(int prompt, int eval, int cached, int callCount = 1);
    qint64 total() const { return promptTokens + evalTokens; }
    qint64 freshPromptTokens() const {
        return qMax<qint64>(0, promptTokens - cachedTokens);
    }
    void reset() { *this = TokenStats(); }

    // --- Stage 35 geri uyum: model adına göre kaba fiyat (yerel/bilinmeyen = 0) ---
    static double priceIn(const QString& model);
    static double priceOut(const QString& model);

    // --- Stage 36: sağlayıcı + model ---
    double estimateCostUsd(const QString& providerId, const QString& model) const;
    double estimateCostUsd(const QString& model) const; // geri uyum
    QString summary(const QString& providerId, const QString& model) const;
    QString summary(const QString& model = QString()) const; // geri uyum

    // --- Maliyet koruması (koşu maliyet onayı) ---
    // Ayardaki günlük/koşu tavanı; 0 = sınırsız.
    static double costGuardUsd();
    static void setCostGuardUsd(double usd);
    bool exceedsGuard(double limitUsd = -1.0, const QString& providerId = QString(),
                      const QString& model = QString()) const;
    // Tahmini maliyet, tavanı aşarsa onay gerekir
    static bool needsConfirmation(double estimatedUsd, double limitUsd = -1.0);
    static QString confirmationText(double estimatedUsd, const QString& model);
};
