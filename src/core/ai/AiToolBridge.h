#pragma once
#include "AiMessage.h"
#include "LlmProvider.h"
#include <QJsonArray>
#include <QList>
#include <QString>

struct ToolCall;
class AgentTools;

// Stage 35: sağlayıcıdan bağımsız araç çağırma köprüsü.
// Sağlayıcıda native function-calling varsa onu kullanır; yoksa mevcut
// <tool_call> metin protokolüne düşer — ajan döngüsü hiç değişmez.
class AiToolBridge {
public:
    // AgentTools::toolSchemas() çıktısını (OpenAI biçimli) birleşik tanıma çevirir
    static QList<AiToolDef> fromOpenAiSchemas(const QJsonArray& schemas);
    // Birleşik araç çağrılarını ajan biçimine çevirir
    static QList<ToolCall> toAgentCalls(const QList<AiToolCall>& native);
    // Yanıttan araç çağrılarını çıkarır: native → metin protokolü sırasıyla
    static QList<ToolCall> extract(const AiReply& reply);
    // Model desteklemiyorsa sisteme eklenecek metin protokolü talimatı
    static QString textProtocolHint();
    // Araç sonuçlarını birleşik konuşmaya eklemek için hazırla
    static AiMessage toolResultMessage(const AiToolCall& call, const QString& output, bool ok);
    // Sağlayıcı bu modelde native araç çağırmayı destekliyor mu?
    static bool supportsNativeTools(const ProviderSpec& spec);
};
