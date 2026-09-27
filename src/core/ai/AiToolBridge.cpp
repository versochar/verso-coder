#include "AiToolBridge.h"
#include <QJsonObject>
#include "../AgentTools.h"

QList<AiToolDef> AiToolBridge::fromOpenAiSchemas(const QJsonArray& schemas) {
    QList<AiToolDef> out;
    for (const QJsonValue& v : schemas) {
        const QJsonObject o = v.toObject();
        const QJsonObject fn = o.contains("function") ? o.value("function").toObject() : o;
        AiToolDef t;
        t.name = fn.value("name").toString();
        t.description = fn.value("description").toString();
        t.parameters = fn.value("parameters").toObject();
        if (!t.name.isEmpty()) out << t;
    }
    return out;
}

QList<ToolCall> AiToolBridge::toAgentCalls(const QList<AiToolCall>& native) {
    QList<ToolCall> out;
    for (const AiToolCall& c : native) {
        if (c.name.isEmpty()) continue;
        ToolCall t;
        t.name = c.name;
        t.args = c.args;
        t.raw = c.id;
        out << t;
    }
    return out;
}

QList<ToolCall> AiToolBridge::extract(const AiReply& reply) {
    if (!reply.toolCalls.isEmpty()) return toAgentCalls(reply.toolCalls);
    // Yedeğin yolu: model metin protokolüne düştü
    return AgentTools::parseCalls(reply.text);
}

QString AiToolBridge::textProtocolHint() {
    return QString(
        "\nAraç çağırmak için yanıtına şu bloğu yaz:\n"
        "<tool_call>\n{\"name\":\"read_file\",\"arguments\":{\"path\":\"src/main.cpp\"}}\n"
        "</tool_call>\n"
        "Gözlemler sana geri verilir. Her adımda en fazla birkaç çağrı yaz.\n");
}

AiMessage AiToolBridge::toolResultMessage(const AiToolCall& call, const QString& output, bool ok) {
    return AiMessage::toolResultMsg(call.id.isEmpty() ? call.name : call.id, call.name, output, ok);
}

bool AiToolBridge::supportsNativeTools(const ProviderSpec& spec) { return spec.supportsTools; }
