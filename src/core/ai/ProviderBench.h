#pragma once
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

// Stage 36: sağlayıcı karşılaştırma (mikro-bench).
// Ağ YOK: puanlama/öneri saf fonksiyonlar, test edilebilir.

struct BenchTask {
    QString id;
    QString title;
    QString prompt;
    QStringList keywords; // yanıtta aranan işaretler
    int difficulty = 1; // 1 basit · 2 orta · 3 zor
    bool wantsCode = false;
};

struct BenchResult {
    QString providerId;
    QString model;
    double quality = 0.0; // 0–100
    int latencyMs = 0;
    int promptTokens = 0;
    int evalTokens = 0;
    double usd = 0.0;
    double score = 0.0; // birleşik 0–100
    QStringList notes;
    bool ok = false;
};

class ProviderBench {
public:
    // Standart görev seti (kod okuma, hata bulma, kısa açıklama, yeniden yazım)
    static QList<BenchTask> tasks();
    // Yanıtları puanla (taskId → yanıt). Geçersiz/boş yanıtlar 0 puan alır.
    static BenchResult score(const QString& providerId, const QString& model,
                             const QList<QPair<QString, QString>>& answers,
                             const QList<int>& latenciesMs);
    // Tek görev puanı (0–100)
    static double qualityOf(const QString& taskId, const QString& answer);
    // Birleşik puan: kalite %60, gecikme %25, maliyet %15
    static double composite(const BenchResult& r);
    // En iyi sağlayıcı önerisi (ücretsiz olan yakınsa o tercih edilir)
    static QString recommend(const QList<BenchResult>& results);
    static QString explain(const BenchResult& r);
    static QString verdict(const BenchResult& r);

    // Yardımcılar
    static int tokensEstimate(const QString& text);
    static double usdFor(const QString& providerId, const QString& model, int p, int e);
    static BenchTask taskById(const QString& id);
};
