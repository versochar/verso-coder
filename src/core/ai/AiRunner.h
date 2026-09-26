#pragma once
#include "AiMessage.h"
#include "AiProfiles.h"
#include "LlmProvider.h"
#include "TaskRouter.h"
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QElapsedTimer>
#include <QStringList>
#include <functional>

class LlmClient;
class SecretStore;

// Stage 37: tek AI cephesi. Söylebet, hayalet tamamlama, arena, tema üretimi,
// belge yorumu, test, commit mesajı, özet ve görsel akışlarının hepsi buradan
// geçer; böylece sağlayıcı seçimi, yönlendirme, kota, sağlık, kullanım kaydı,
// maliyet ve failover uygulamanın tamamında aynı davranır.
class AiRunner : public QObject {
    Q_OBJECT
public:
    struct Options {
        AiTask task = AiTask::Chat;
        QString model;       // boş → yönlendirme/kayıtlı tercih
        QString providerId;  // boş → yönlendirme/kayıtlı tercih
        QString systemPrompt;
        double temperature = -1.0; // <0 → profil/ayar
        int maxTokens = 0;         // 0 → profil
        int timeoutMs = 0;         // 0 → profil
        bool allowFailover = true;
        bool record = true;        // kullanım + sağlık kaydı
        bool bypassRouting = false; // yönlendirme yok, verilen sağlayıcıyı kullan
    };

    struct Result {
        bool ok = false;
        QString text;
        QString reasoning;
        QList<AiToolCall> toolCalls;
        AiUsage usage;
        int httpStatus = 0;
        QString error;
        QString providerId;
        QString model;
        double usd = 0.0;
        qint64 ms = 0;
        QString requestId;
        QString reason; // yönlendirme açıklaması / failover notu
    };

    // Saf karar: hangi sağlayıcı/model? (ağ YOK — test edilebilir)
    struct Plan {
        bool ok = false;
        QString providerId;
        QString model;
        TaskClass cls = TaskClass::Simple;
        QString reason;
        bool forceLocked = false;
    };

    explicit AiRunner(QObject* parent = nullptr);
    ~AiRunner() override;

    void setSecretStore(SecretStore* s) { m_secrets = s; }
    void setRecordUsage(bool on) { m_recordUsage = on; }

    // Seçenekleri göreve göre doldurur (sıcaklık/token/sistem tonu)
    static Options optionsFor(AiTask task, const QString& systemPrompt = QString());

    // Yönlendirme kararı (ağ yok)
    static Plan planFor(const Options& opts);
    // Profil + plan birleşimi
    static Plan resolve(const Options& opts);

    // Senkron çalıştır
    Result run(const Options& opts, const QString& userPrompt,
               const QList<AiImage>& images = QList<AiImage>());
    // Kısa yol
    Result ask(AiTask task, const QString& userPrompt, const QString& systemPrompt = QString(),
               const QList<AiImage>& images = QList<AiImage>());

    // Asenkron (arayüzü kilitlemez, iptal edilebilir). Sonuç finished sinyalinde.
    void runAsync(const Options& opts, const QString& userPrompt,
                  const QList<AiImage>& images = QList<AiImage>());
    // İptal (aktif istek varsa) — GUI iş parçacığından güvenle çağrılır
    bool cancelActive();
    bool hasActive() const { return m_active != nullptr; }

    // Etkin sağlayıcının model listesini getirir (arayüzü kilitlemez)
    void fetchModels(std::function<void(const QStringList&)> done);

    // Görsel yardımcı
    static QList<AiImage> imagesFromBase64(const QStringList& b64,
                                           const QString& mime = QStringLiteral("image/jpeg"));
    // Base64 girdisini temizler (data: öneki, boşluk, satır sonu)
    static QStringList cleanBase64(const QStringList& in);

    // Görsel kararı (ağ YOK): sağlayıcı bu modelde görsel kabul eder mi?
    // Görsel varsa ama desteklenmiyorsa istek "sessizce ekleri atmak" yerine
    // durdurulur — kullanıcı yanlış cevap almak istemez.
    static bool canRunWithImages(const QString& providerId, const QString& model,
                                 int imageCount);
    static QString imageBlockReason(const QString& providerId, int imageCount);

    // --- Ortak kuyruk (11): tek eşzamanlı AI isteği + iptal ---
    static bool busy();
    void cancel();
    static void cancelAll();
    static int activeCount();
    // Tahmini maliyet (onay diyaloğu için)
    static double estimateUsd(const Plan& plan, int promptChars, int outputTokens);

signals:
    void finished(const Result& r);
    void busyChanged(bool busy);

private:
    LlmClient* clientFor(const QString& providerId);
    void enter();
    void leave();

    void handleAsyncResult(const LlmClient* client, const AiReply& rep, const QElapsedTimer& timer,
                           const Plan& plan);

    LlmClient* m_active = nullptr;
    QHash<QString, LlmClient*> m_pool;
    SecretStore* m_secrets = nullptr;
    bool m_recordUsage = true;
    bool m_inside = false;
};
