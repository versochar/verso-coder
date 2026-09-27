#pragma once
#include "LlmProvider.h"
#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

// Stage 36: sağlayıcı sağlık skoru — istek/hata/gecikme istatistiği.
struct ProviderHealthEntry {
    int calls = 0;
    int errors = 0;
    int consecutiveErrors = 0;
    qint64 totalLatencyMs = 0;
    int lastStatus = 0;
    QDateTime lastOk;
    QDateTime lastFail;

    double errorRate() const { return calls > 0 ? double(errors) / double(calls) : 0.0; }
    int avgLatencyMs() const { return calls > 0 ? int(totalLatencyMs / qMax(1, calls - errors)) : 0; }
    bool healthy() const { return consecutiveErrors < 3; }
    qint64 uptimeMs() const;
    // Stage 38: sigorta — arka arkaya "yoğun" hatalarda sağlayıcı devre dışı
    int busyFails = 0;        // 429/403/503 "rate limit" sayısı
    QDateTime openUntil;      // bu zamana kadar yönlendirmeden çıkarılır
    bool tripped() const { return openUntil.isValid() && QDateTime::currentDateTime() < openUntil; }
};

class ProviderHealth {
public:
    explicit ProviderHealth(const QString& file = QString());
    static ProviderHealth& instance();

    void recordSuccess(const QString& providerId, int latencyMs = 0);
    void recordFailure(const QString& providerId, int latencyMs = 0, int httpStatus = 0);
    // Tek çağrıyı tek satırda kaydet
    void record(const QString& providerId, bool ok, int latencyMs = 0, int httpStatus = 0);

    ProviderHealthEntry entry(const QString& providerId) const;
    double errorRate(const QString& providerId) const;
    bool isHealthy(const QString& providerId) const;
    void reset(const QString& providerId = QString());
    bool load();
    bool save() const;
    QString filePath() const { return m_file; }
    QStringList tracked() const;

    // --- Failover ---
    // Seçili sağlayıcı sağlıksızsa eşdeğer yedeğe devret.
    // Eşdeğer = aynı tür (kind) ya da aynı yetenekler + anahtarı var (ya da yerel).
    // Öncelik: sağlıklı > düşük hata oranı > yerel (ücretsiz) > hızlı.
    static QString pickFailover(const QString& failedProviderId, const QList<ProviderSpec>& pool,
                                const QString& requiredModel = QString());
    // Aynı işi yapabilen aday havuzu (sıralı)
    static QStringList failoverCandidates(const QString& failedProviderId,
                                          const QList<ProviderSpec>& pool,
                                          const QString& requiredModel = QString());
    // Eşdeğerlik testi
    static bool isEquivalent(const ProviderSpec& a, const ProviderSpec& b,
                             const QString& requiredModel = QString());

    // Görüntüleme
    QString statusLine(const QString& providerId) const;
    static QString verdict(const ProviderHealthEntry& e);

    // --- Stage 38: sigorta (circuit breaker) ---
    // "Yoğun" hatalar art arda gelirse sağlayıcı soğuma süresince havuzdan
    // çıkarılır; süre dolunca veya başarılı istekte yeniden alınır.
    static int busyThreshold() { return 3; }
    static int cooldownSec() { return 120; }
    // Başarılı istek: sigortayı sıfırlar
    void recordBusy(const QString& providerId);
    void clearTripped(const QString& providerId);
    bool isTripped(const QString& providerId) const;
    int cooldownLeft(const QString& providerId) const;
    // Sağlayıcı seçiminden çıkarılacak mı?
    bool isUsable(const QString& providerId) const;
    // Havuzdan tripped olanları ayıkla (kendi dosyasına göre)
    QStringList filterUsable(const QStringList& providerIds) const;

private:
    QString m_file;
    QJsonObject m_data; // id -> {calls,errors,consec,lat,status,lastOk,lastFail}
    ProviderHealthEntry read(const QString& id) const;
    void write(const QString& id, const ProviderHealthEntry& e);
};
