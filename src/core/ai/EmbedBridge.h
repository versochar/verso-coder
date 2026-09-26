#pragma once
#include "AiMessage.h"
#include "LlmProvider.h"
#include "EmbedCache.h"
#include "LlmClient.h"

class SecretStore;
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

// Stage 36: sağlayıcıdan bağımsız gömme köprüsü.
// Ollama yolu `EmbeddingClient` ile aynı sonucu verir; NIM/OpenAI/UnoRouter gibi
// bulut sağlayıcılarının gömme modellerini de aynı arayüzle kullanır.
// Aynı metin için tekrar ağ çağrısı yapılmaz (EmbedCache).
class EmbedBridge : public QObject {
    Q_OBJECT
public:
    struct Result {
        QList<QList<float>> vectors;
        QString providerId;
        QString model;
        QString error;
        int embedded = 0;    // ağdan alınan
        int cached = 0;      // önbellekten gelen
        bool ok() const { return error.isEmpty() && !vectors.isEmpty(); }
    };

    explicit EmbedBridge(QObject* parent = nullptr);
    ~EmbedBridge() override;

    void setProvider(const QString& providerId);
    void setModel(const QString& model);
    QString provider() const { return m_provider; }
    QString model() const { return m_model; }
    void setSecretStore(SecretStore* s) { m_secrets = s; }
    EmbedCache& cache() { return m_cache; }

    // Gömme sağlayıcısı ve modelini otomatik çöz (sohbet sağlayıcısı gömme
    // desteklemiyorsa destekleyen ilkini seçer)
    static QString resolveProvider();
    static QString resolveModel(const QString& providerId = QString());
    static bool available(const QString& providerId);

    // Senkron gömme. Önce önbellek, eksikler toplu hâlde istenir.
    Result embed(const QStringList& texts, int timeoutMs = 60000);
    Result embedOne(const QString& text, int timeoutMs = 30000);

    // Toplu boyut (sağlayıcıya göre)
    static int batchSize(const ProviderSpec& spec);

private:
    LlmClient& client();

    mutable LlmClient m_client;   // sağlayıcı değişince yeniden yapılandırılır
    QString m_provider;
    QString m_model;
    SecretStore* m_secrets = nullptr;
    EmbedCache m_cache{512};
};
