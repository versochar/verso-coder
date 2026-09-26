#pragma once
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QStringList>

// Ollama REST istemcisi: /api/tags, /api/chat (non-stream + streaming).
// GPU offload: CUDA/ROCm/Vulkan otomatik; num_gpu + backend tercihi options'a gömülür.
class OllamaClient : public QObject {
    Q_OBJECT
public:
    explicit OllamaClient(QObject* parent = nullptr);

    void setHost(const QString& host);
    void setKeepAlive(int minutes) { m_keepAliveMin = minutes; } // Stage 33
    void fetchModels(); // finished -> modelsReady / error
    void chat(const QString& model, const QString& systemPrompt,
              const QString& userText, const QJsonObject& options);
    void chatStream(const QString& model, const QString& systemPrompt,
                    const QString& userText, const QJsonObject& options);
    void cancelStream();
    bool isStreaming() const { return m_stream != nullptr; }
    // Stage 15: model indirme (/api/pull, akışlı ilerleme)
    void pull(const QString& model);
    void cancelPull();
    bool isPulling() const { return m_pull != nullptr; }

    // Ajan döngüsü için bloklayıcı (senkron) sohbet. Kendi ağ isteğini kullanır,
    // chatReply sinyalini yaymaz. Hata durumunda error doldurulur.
    QString chatSync(const QString& model, const QString& systemPrompt,
                     const QString& userText, const QJsonObject& options,
                     QString& error, int timeoutMs = 180000);

    // Yerel sunucu yönetimi (uygulama içinden "Ollama'yı Başlat")
    static QString findServerBinary(); // PATH + ~/.local/bin + /usr/local/bin
    static bool isServerUp(const QString& host, int timeoutMs = 2000);
    // Kapalıysa detached başlatıp açılmasını bekler (GUI thread'de çağırma!)
    static bool ensureServer(const QString& host, int timeoutMs = 25000);

    // --- Stage 33: model yeteneği + görü (vision) ---
    void showModel(const QString& model); // finished -> modelShow
    QJsonObject showSync(const QString& model, QString& error, int timeoutMs = 8000);
    void chatWithImages(const QString& model, const QString& systemPrompt,
                        const QString& userText, const QJsonObject& options,
                        const QStringList& imagesBase64);
    void chatStreamWithImages(const QString& model, const QString& systemPrompt,
                              const QString& userText, const QJsonObject& options,
                              const QStringList& imagesBase64);

signals:
    void modelsReady(const QStringList& models);
    void modelShow(const QString& model, const QJsonObject& show); // Stage 33
    void chatReply(const QString& text);
    void chatToken(const QString& chunk);   // streaming parça
    void chatFinished(const QString& full); // streaming tamamı
    void tokensUsed(int promptTokens, int evalTokens); // Ollama token sayaçları
    void error(const QString& msg);
    // Stage 15: pull ilerlemesi (completed/total bayt; total 0 olabilir)
    void pullProgress(const QString& model, qint64 completed, qint64 total,
                      const QString& status);
    void pullFinished(const QString& model);

private:
    void parseStreamChunk(const QByteArray& data);
    void emitTokenCounts(const QJsonObject& obj);
    // Stage 31: yanıt gelmezse iptal eden bekçi (sahiplik çağıranda)
    static void armTimeout(QNetworkReply* r, int ms);

    QNetworkAccessManager m_net;
    QString m_host = "http://localhost:11434";
    int m_keepAliveMin = 5; // Stage 33: Ollama keep_alive
    QNetworkReply* m_stream = nullptr;
    QNetworkReply* m_pull = nullptr; // Stage 15
    QByteArray m_pending;
    QString m_full;
};
