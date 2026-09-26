#pragma once
#include "../AiMessage.h"
#include "../LlmProvider.h"
#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>

// Stage 35: sağlayıcıya özel gövde üretimi/çözümlemesi. Ağ YOK — saf dönüşümler,
// böylece istek/yanıt şekilleri doğrudan test edilir.
class ProviderCodec {
public:
    // --- istek gövdesi (chat) ---
    static QJsonObject chatRequest(const ProviderSpec& spec, const AiChatRequest& req,
                                   QString* resolvedModel = nullptr);
    // --- yanıt çözümleme ---
    static AiReply parseReply(const ProviderSpec& spec, const QByteArray& body,
                              const QString& model = QString());
    // --- akış olayı çözümleme (her `data:` gövdesi için) ---
    static AiChunk parseStreamEvent(const ProviderSpec& spec, const QString& data,
                                    const QString& model = QString());
    // --- modeller ---
    static QStringList parseModels(const ProviderSpec& spec, const QByteArray& body);
    // --- gömme ---
    static QJsonObject embedRequest(const ProviderSpec& spec, const QString& model,
                                    const QStringList& inputs, QString* resolvedModel = nullptr);
    static QList<QList<float>> parseEmbed(const ProviderSpec& spec, const QByteArray& body);

    // Yardımcılar
    static QString chatUrl(const ProviderSpec& spec, const QString& model);
    static QString modelsUrl(const ProviderSpec& spec);
    static QString embedUrl(const ProviderSpec& spec, const QString& model);
    static QString openAiFinishReason(const QString& raw);
    // Araç tanımını OpenAI şemasına çevir
    static QJsonObject openAiTool(const AiToolDef& t);
};
