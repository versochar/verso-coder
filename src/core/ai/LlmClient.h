#pragma once
#include "AiMessage.h"
#include "LlmProvider.h"
#include "providers/ProviderCodec.h"
#include "providers/SseParser.h"
#include <QNetworkAccessManager>
#include <QObject>
#include <QStringList>

class SecretStore;

// Stage 35: sağlayıcıdan bağımsız ağ istemcisi.
// Akış (SSE) + akışsız, gömme, model listeleme, üstel geri çekilme, zaman aşımı.
class LlmClient : public QObject {
    Q_OBJECT
public:
    explicit LlmClient(QObject* parent = nullptr);
    ~LlmClient() override;

    void setProvider(const ProviderSpec& spec);
    ProviderSpec provider() const { return m_spec; }
    void setApiKey(const QString& key) { m_apiKey = key; }
    QString apiKey() const;
    void setSecretStore(SecretStore* s) { m_secrets = s; }
    SecretStore* secretStore() const { return m_secrets; }
    // Stage 37: model düzeyinde yedekleme. Seçili model hız sınırına takılırsa
    // (429/403/503 "rate limit" / "model busy") aynı sağlayıcıdaki başka bir
    // modele geçilir. Fiyat sınıfı korunur: :free → ücretliye sessizce geçilmez.
    void setModelFailoverEnabled(bool on) { m_modelFailover = on; }
    bool modelFailoverEnabled() const { return m_modelFailover; }
    // Ücretli modele geçmeye izin ver (varsayılan: hayır)
    void setModelFailoverAllowPaid(bool on) { m_modelFailoverPaid = on; }
    // Ayarlardan yükle (ai/modelFailover, ai/modelFailoverAllowPaid)
    void loadFailoverSettings();
    // Değiştirilen model (boşsa değişiklik olmadı)
    QString lastModelFailoverFrom() const { return m_modelFailoverFrom; }
    // Geçilen model (boşsa değişiklik olmadı)
    QString lastModelFailoverTo() const { return m_modelFailoverTo; }
    static int maxModelFailovers() { return 2; }
    // "Yoğun" sayılan hata mı? (hız sınırı veya model meşgul)
    static bool isModelBusy(int httpStatus, const QByteArray& body);
    // Yedeğe geçilecek model (boşsa yok)
    static QString pickModelFallback(const ProviderSpec& spec, const QString& current,
                                     const QStringList& tried, bool allowPaid = false);

    // Stage 36: sağlayıcı kalıcı olarak reddedilirse (401/403/404/5xx) eşdeğer
    // bir sağlayıcıya otomatik devret.
    void setFailoverEnabled(bool on) { m_failover = on; }
    bool failoverEnabled() const { return m_failover; }
    // Devredilen sağlayıcı (boşsa devir olmadı)
    QString lastFailoverFrom() const { return m_failoverFrom; }
    // Sağlayıcıya göre anahtarı kasadan/env'den çeker (elle verilmişse onu kullanır)
    void loadKeyForProvider();

    // Ağ
    void fetchModels();
    void chat(const AiChatRequest& request);
    void chatStream(const AiChatRequest& request);
    void cancel();
    bool busy() const { return m_busy; }

    // Senkron (ajan döngüsü ve testler)
    AiReply chatSync(const AiChatRequest& request, int timeoutMs = 180000);
    QList<QList<float>> embedSync(const QString& model, const QStringList& inputs, QString& error,
                                  int timeoutMs = 90000);
    // Ayarlar → "Bağlantıyı Test Et"
    bool testConnection(QString& error, int timeoutMs = 12000);

    // --- saf yardımcılar (test edilebilir) ---
    static QJsonObject requestHeaders(const ProviderSpec& spec, const QString& apiKey);
    // Yeniden deneme gecikmesi: 429/5xx için üstel, Retry-After baskın
    static int retryDelayMs(int attempt, int httpStatus, const QByteArray& retryAfter = QByteArray(),
                          const QByteArray& body = QByteArray());
    static bool isRetryable(int httpStatus, const QByteArray& body = QByteArray());
    // Sağlayıcıyı değiştirmeyi gerektiren kalıcı hata mı? (yeniden denemek anlamsız)
    static bool isHardFailure(int httpStatus, const QByteArray& body = QByteArray());
    // Gerçek sağlayıcılarda hız sınırı 429 değil 403 olarak da gelir ve gövdede
    // "per minute limit / try again in a minute" yazar. Bu GEÇİCİDİR: istek sağlamdır,
    // kaynağın kendisi yoğundur. Yanlışlıkla failover tetiklenmesin diye ayrışır.
    static bool isRateLimited(int httpStatus, const QByteArray& body);
    // Hız sınırında beklenecek süre (Retry-After yoksa daha uzun beklenir:
    // sağlayıcının "bir dakika" demesi 0,8 sn'lik üstel geri çekilmeyle kapanmaz)
    static int rateLimitDelayMs(int attempt);
    // Retry-After bu eşiği aşarsa yeniden deneme anlamsız (ör. ücretsiz katman
    // "1 istek / 30 dakika" → 1628 sn). O durumda beklemek yerine mesajı
    // kullanıcıya gösterip isteği bitiririz.
    static int maxRetryAfterSec() { return 120; }
    // Retry-After başlığı verilmişse ve bu eşiği aşıyorsa true
    static bool retryAfterTooLong(const QByteArray& retryAfter);
    static int maxRetries() { return 3; }

signals:
    void modelsReady(const QStringList& models);
    void replyReady(const AiReply& reply);
    void chunkReady(const AiChunk& chunk);
    void finished(const AiReply& reply);
    void error(const QString& message);
    void tokensUsed(int prompt, int eval, int reasoning);
    void statusChanged(const QString& status);

private:
    void sendChat(const AiChatRequest& req, bool stream);
    // Yeniden deneme döngüsünün kullandığı iç yol: sayaçları SIFIRLAMAZ.
    // (sendChat her çağrıda sıfırladığı için sonsuz döngüye giriyordu.)
    void dispatch(const AiChatRequest& req, bool stream);
    QNetworkReply* post(const QJsonObject& body, const QString& url);
    void handleChatFinished();
    void flushStream();
    void failWith(const QString& msg, int status = 0);
    void scheduleRetry();

    ProviderSpec m_spec;
    QString m_apiKey;
    SecretStore* m_secrets = nullptr;

    QNetworkAccessManager m_net;
    QNetworkReply* m_reply = nullptr;
    bool m_busy = false;
    bool m_wantStream = false;
    int m_attempt = 0;
    int m_gen = 0; // Stage 50-debug: iptal sonrası zamanlanmış işler çalışmaz

    bool m_failover = true;
    int m_failoverCount = 0;
    QString m_failoverFrom;
    bool m_modelFailover = true;
    bool m_modelFailoverPaid = false;
    int m_modelFailoverCount = 0;
    QStringList m_modelsTried;
    QString m_modelFailoverFrom;
    QString m_modelFailoverTo;
    SseParser m_sse;
    AiChatRequest m_req;      // yeniden denemede kullanılır
    AiReply m_acc;            // akış birikimi
    QList<AiToolCall> m_streamCalls;
};
