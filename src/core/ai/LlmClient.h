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
    static int retryDelayMs(int attempt, int httpStatus, const QByteArray& retryAfter = QByteArray());
    static bool isRetryable(int httpStatus);
    // Sağlayıcıyı değiştirmeyi gerektiren kalıcı hata mı? (yeniden denemek anlamsız)
    static bool isHardFailure(int httpStatus);
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

    bool m_failover = true;
    int m_failoverCount = 0;
    QString m_failoverFrom;
    SseParser m_sse;
    AiChatRequest m_req;      // yeniden denemede kullanılır
    AiReply m_acc;            // akış birikimi
    QList<AiToolCall> m_streamCalls;
};
