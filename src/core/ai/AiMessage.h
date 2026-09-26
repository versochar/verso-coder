#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

// Stage 35: sağlayıcıdan bağımsız mesaj/yanıt modeli.
// Tüm adaptörler (OpenAI-uyumlu, Anthropic, Gemini, Ollama) bu tiplere çevirir;
// UI ve ajan katmanı yalnızca bunları görür.

enum class AiRole { System, User, Assistant, Tool };

struct AiImage {
    QString mime = "image/jpeg";
    QByteArray bytes;      // ham bayt
    QString base64() const; // istek gövdesine gömülecek hâli
};

struct AiPart {
    enum Kind { Text, Image, ToolCall, ToolResult };
    Kind kind = Text;
    QString text;
    AiImage image;
    // ToolCall için
    QString callId;
    QString name;          // araç adı
    QJsonObject args;
    bool ok = true;        // ToolResult için
};

struct AiMessage {
    AiRole role = AiRole::User;
    QStringList texts;     // düz metin parçaları
    QList<AiImage> images;
    QString callId;        // Tool rolü için
    QString name;          // çağrı/araç adı
    QString toolOutput;
    bool ok = true;        // Tool rolü: çağrı başarılı mı

    QString text() const { return texts.join("\n"); }
    bool isEmpty() const { return texts.isEmpty() && images.isEmpty() && toolOutput.isEmpty(); }
    static AiMessage system(const QString& t);
    static AiMessage user(const QString& t);
    static AiMessage userText(const QString& t, const QList<AiImage>& imgs);
    static AiMessage assistant(const QString& t);
    static AiMessage toolResultMsg(const QString& callId, const QString& name,
                                    const QString& out, bool ok = true);
};

struct AiToolDef {
    QString name;
    QString description;
    QJsonObject parameters; // JSON Schema
};

struct AiToolCall {
    QString id;
    QString name;
    QJsonObject args;
};

struct AiUsage {
    int promptTokens = 0;
    int evalTokens = 0;
    int reasoningTokens = 0;
    int total() const { return promptTokens + evalTokens; }
};

struct AiReply {
    bool ok = false;
    QString text;
    QString reasoning;              // düşünme/analiz metni (varsa)
    QList<AiToolCall> toolCalls;
    AiUsage usage;
    QString model;
    QString finishReason;
    QString error;
    int httpStatus = 0;
    QString requestId;
    QString raw;
};

// Akışta gelen parça
struct AiChunk {
    QString text;
    QString reasoning;
    QList<AiToolCall> toolCalls;   // akışta biriken araç çağrıları
    AiUsage usage;
    bool done = false;
    QString finishReason;
};

struct AiChatRequest {
    QString model;
    QString systemPrompt;
    QList<AiMessage> messages;
    double temperature = 0.7;
    int numCtx = 0;        // 0 = gönderme
    int maxTokens = 0;     // 0 = gönderme
    bool stream = false;
    bool wantTools = false;
    QList<AiToolDef> tools;
    // Sağlayıcı tuhaflıkları
    QString reasoningEffort;
    QJsonObject extra;     // num_gpu/num_thread vb. (yalnız Ollama/uyumlu)
};
