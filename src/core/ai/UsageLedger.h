#pragma once
#include "LlmProvider.h"
#include <QDate>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

// Stage 36: kalıcı token kullanım geçmişi + sağlayıcı başına günlük kota.
// API anahtarları buraya YAZILMAZ; yalnız sayaçlar ve maliyet tutulur.
class UsageLedger {
public:
    explicit UsageLedger(const QString& file = QString());

    QString path() const { return m_file; }
    bool load();
    bool save() const;

    // Bir çağrıyı kaydet (güncel sağlayıcı + model ile fiyat hesaplanır)
    void record(const QString& providerId, const QString& model, int prompt, int eval,
                int cached = 0);
    // Fiyatı önceden hesaplanmış kayıt (akış özeti vb.)
    void recordWithCost(const QString& providerId, const QString& model, int prompt, int eval,
                        double usd);

    struct Day {
        int calls = 0;
        qint64 prompt = 0;
        qint64 eval = 0;
        double usd = 0.0;
        qint64 total() const { return prompt + eval; }
    };
    // Tek gün + sağlayıcı toplamı
    Day day(const QString& isoDate, const QString& providerId = QString()) const;
    Day today(const QString& providerId = QString()) const;
    // Son n gün (en yeni önce)
    QList<QPair<QString, Day>> lastDays(int n = 7) const;
    QList<QPair<QString, Day>> lastDays(int n, const QString& providerId) const;
    int dayCount() const;
    QStringList providers() const;

    // --- Kota bütçesi ---
    struct Quota {
        int maxCalls = 0;   // günlük istek tavanı (0 = sınırsız)
        qint64 maxTokens = 0; // günlük token tavanı (0 = sınırsız)
        bool limited() const { return maxCalls > 0 || maxTokens > 0; }
    };
    void setQuota(const QString& providerId, const Quota& q);
    Quota quota(const QString& providerId) const;
    // Kota aşıldı mı? `why` doluysa aşılmıştır.
    bool quotaExceeded(const QString& providerId, QString& why) const;
    // Kalan bütçe (sınırsızsa -1)
    int remainingCalls(const QString& providerId) const;
    qint64 remainingTokens(const QString& providerId) const;
    // Kota sıfırlama
    void clearToday(const QString& providerId);

    // Bakım: N günden eskisini sil
    int prune(int keepDays = 90);
    void reset();

    static QString dayKey(const QDate& d = QDate::currentDate());
    static UsageLedger& instance(); // ~/.verso/ai-usage.json

private:
    QString m_file;
    QJsonObject m_days;  // "2026-09-27" -> { "openai": {calls,prompt,eval,usd} }
    QJsonObject m_quota; // "openai" -> {maxCalls,maxTokens}
};
