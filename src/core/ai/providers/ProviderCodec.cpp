#include "ProviderCodec.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>

// ---------- URL'ler ----------

QString ProviderCodec::chatUrl(const ProviderSpec& spec, const QString& model) {
    QString path = spec.chatPath;
    if (path.contains("%1")) {
        const QString m = spec.resolveModelId(model);
        // Gemini ':' içeriyorsa (gpt-oss:free) URL'de sorun çıkmaz
        path.replace("%1", m.isEmpty() ? QString("default") : m);
    }
    return spec.url(path);
}

QString ProviderCodec::modelsUrl(const ProviderSpec& spec) {
    if (spec.modelsPath.isEmpty()) return {};
    return spec.url(spec.modelsPath);
}

QString ProviderCodec::embedUrl(const ProviderSpec& spec, const QString& model) {
    if (spec.embedPath.isEmpty() || !spec.supportsEmbed) return {};
    QString path = spec.embedPath;
    if (path.contains("%1")) path.replace("%1", spec.resolveModelId(model));
    return spec.url(path);
}

// ---------- OpenAI-uyumlu gövde ----------

QJsonObject ProviderCodec::openAiTool(const AiToolDef& t) {
    QJsonObject fn;
    fn["name"] = t.name;
    fn["description"] = t.description;
    QJsonObject params = t.parameters;
    if (params.isEmpty()) {
        params["type"] = "object";
        params["properties"] = QJsonObject();
    }
    fn["parameters"] = params;
    QJsonObject o;
    o["type"] = "function";
    o["function"] = fn;
    return o;
}

static QJsonArray openAiMessages(const AiChatRequest& req) {
    QJsonArray out;
    if (!req.systemPrompt.trimmed().isEmpty())
        out.append(QJsonObject{{"role", "system"}, {"content", req.systemPrompt}});
    for (const AiMessage& m : req.messages) {
        if (m.role == AiRole::System) {
            if (!m.text().isEmpty())
                out.append(QJsonObject{{"role", "system"}, {"content", m.text()}});
            continue;
        }
        if (m.role == AiRole::Tool) {
            out.append(QJsonObject{{"role", "tool"},
                                   {"tool_call_id", m.callId},
                                   {"content", m.toolOutput}});
            continue;
        }
        if (m.role == AiRole::User && !m.images.isEmpty()) {
            QJsonArray parts;
            parts.append(QJsonObject{{"type", "text"}, {"text", m.text()}});
            for (const AiImage& img : m.images)
                parts.append(QJsonObject{
                    {"type", "image_url"},
                    {"image_url", QJsonObject{{"url", QString("data:%1;base64,%2")
                                                          .arg(img.mime, img.base64())}}}});
            out.append(QJsonObject{{"role", "user"}, {"content", parts}});
            continue;
        }
        out.append(QJsonObject{{"role", m.role == AiRole::Assistant ? "assistant" : "user"},
                               {"content", m.text()}});
    }
    return out;
}

static QJsonObject geminiBody(const ProviderSpec& spec, const AiChatRequest& req,
                              QString* resolvedModel) {
    QJsonObject root;
    if (resolvedModel) *resolvedModel = spec.resolveModelId(req.model);
    QJsonArray contents;
    if (!req.systemPrompt.trimmed().isEmpty()) {
        QJsonObject si;
        si["text"] = req.systemPrompt;
        contents.append(QJsonObject{{"role", "user"},
                                    {"parts", QJsonArray{si}}});
    }
    for (const AiMessage& m : req.messages) {
        if (m.role == AiRole::Tool) {
            QJsonObject pr;
            pr["functionResponse"] =
                QJsonObject{{"name", m.name}, {"response", QJsonObject{{"result", m.toolOutput}}}};
            contents.append(QJsonObject{{"role", "user"}, {"parts", QJsonArray{pr}}});
            continue;
        }
        QJsonArray parts;
        if (!m.text().isEmpty()) parts.append(QJsonObject{{"text", m.text()}});
        for (const AiImage& img : m.images) {
            parts.append(QJsonObject{{"inlineData",
                                      QJsonObject{{"mimeType", img.mime},
                                                  {"data", img.base64()}}}});
        }
        if (parts.isEmpty()) continue;
        contents.append(QJsonObject{{"role", m.role == AiRole::Assistant ? "model" : "user"},
                                    {"parts", parts}});
    }
    root["contents"] = contents;
    QJsonObject gen;
    gen["temperature"] = req.temperature;
    if (req.maxTokens > 0) gen["maxOutputTokens"] = req.maxTokens;
    root["generationConfig"] = gen;
    if (req.wantTools && !req.tools.isEmpty()) {
        QJsonArray fns;
        for (const AiToolDef& t : req.tools) {
            fns.append(QJsonObject{{"name", t.name},
                                   {"description", t.description},
                                   {"parameters", t.parameters.isEmpty()
                                                      ? QJsonObject{{"type", "object"},
                                                                    {"properties",
                                                                     QJsonObject()}}
                                                      : t.parameters}});
        }
        root["tools"] = QJsonArray{QJsonObject{{"functionDeclarations", fns}}};
    }
    return root;
}

static QJsonObject anthropicBody(const ProviderSpec& spec, const AiChatRequest& req,
                                 QString* resolvedModel) {
    QJsonObject root;
    if (resolvedModel) *resolvedModel = spec.resolveModelId(req.model);
    if (!req.systemPrompt.trimmed().isEmpty()) root["system"] = req.systemPrompt;
    QJsonArray msgs;
    for (const AiMessage& m : req.messages) {
        if (m.role == AiRole::System) continue; // yukarıda gitti
        if (m.role == AiRole::Tool) {
            QJsonObject block;
            block["type"] = "tool_result";
            block["tool_use_id"] = m.callId;
            block["content"] = m.toolOutput;
            msgs.append(QJsonObject{{"role", "user"}, {"content", QJsonArray{block}}});
            continue;
        }
        QJsonArray content;
        for (const QString& t : m.texts)
            content.append(QJsonObject{{"type", "text"}, {"text", t}});
        for (const AiImage& img : m.images)
            content.append(QJsonObject{
                {"type", "image"},
                {"source", QJsonObject{{"type", "base64"},
                                       {"media_type", img.mime},
                                       {"data", img.base64()}}}});
        if (content.isEmpty()) content.append(QJsonObject{{"type", "text"}, {"text", ""}});
        msgs.append(QJsonObject{{"role", m.role == AiRole::Assistant ? "assistant" : "user"},
                                {"content", content}});
    }
    root["messages"] = msgs;
    root["max_tokens"] = req.maxTokens > 0 ? req.maxTokens : 4096;
    root["temperature"] = req.temperature;
    root["stream"] = req.stream;
    if (req.wantTools && !req.tools.isEmpty()) {
        QJsonArray tools;
        for (const AiToolDef& t : req.tools)
            tools.append(QJsonObject{{"name", t.name},
                                     {"description", t.description},
                                     {"input_schema", t.parameters.isEmpty()
                                                           ? QJsonObject{{"type", "object"},
                                                                         {"properties",
                                                                          QJsonObject()}}
                                                           : t.parameters}});
        root["tools"] = tools;
    }
    return root;
}

static QJsonObject ollamaBody(const AiChatRequest& req) {
    QJsonObject root;
    root["model"] = req.model;
    root["stream"] = req.stream;
    QJsonObject opts;
    opts["temperature"] = req.temperature;
    if (req.numCtx > 0) opts["num_ctx"] = req.numCtx;
    if (!req.extra.isEmpty())
        for (auto it = req.extra.begin(); it != req.extra.end(); ++it) opts[it.key()] = it.value();
    if (req.maxTokens > 0) opts["num_predict"] = req.maxTokens;
    root["options"] = opts;
    QJsonArray msgs;
    if (!req.systemPrompt.trimmed().isEmpty())
        msgs.append(QJsonObject{{"role", "system"}, {"content", req.systemPrompt}});
    for (const AiMessage& m : req.messages) {
        if (m.role == AiRole::System) {
            msgs.append(QJsonObject{{"role", "system"}, {"content", m.text()}});
            continue;
        }
        QJsonObject user{{"role", m.role == AiRole::Assistant ? "assistant" : "user"},
                         {"content", m.text()}};
        if (!m.images.isEmpty() && m.role == AiRole::User) {
            QJsonArray imgs;
            for (const AiImage& img : m.images) imgs.append(img.base64());
            user["images"] = imgs;
        }
        msgs.append(user);
    }
    root["messages"] = msgs;
    if (req.wantTools && !req.tools.isEmpty()) {
        QJsonArray tools;
        for (const AiToolDef& t : req.tools)
            tools.append(ProviderCodec::openAiTool(t));
        root["tools"] = tools;
    }
    return root;
}

QJsonObject ProviderCodec::chatRequest(const ProviderSpec& spec, const AiChatRequest& req,
                                       QString* resolvedModel) {
    QJsonObject root;
    switch (spec.kind) {
    case ProviderKind::Gemini:
        root = geminiBody(spec, req, resolvedModel);
        break;
    case ProviderKind::Anthropic:
        root = anthropicBody(spec, req, resolvedModel);
        break;
    case ProviderKind::Ollama:
        if (resolvedModel) *resolvedModel = req.model;
        root = ollamaBody(req);
        break;
    case ProviderKind::OpenAICompat:
    default:
        if (resolvedModel) *resolvedModel = spec.resolveModelId(req.model);
        root["model"] = spec.resolveModelId(req.model);
        root["messages"] = openAiMessages(req);
        root["stream"] = req.stream;
        if (req.temperature > 0) root["temperature"] = req.temperature;
        int maxTok = req.maxTokens;
        if ((spec.quirks & QRequiresMaxTokens) && maxTok <= 0) maxTok = 4096;
        if (maxTok > 0) root["max_tokens"] = maxTok;
        if (!req.reasoningEffort.isEmpty() && (spec.quirks & QReasoningEffort))
            root["reasoning_effort"] = req.reasoningEffort;
        if (req.wantTools && !req.tools.isEmpty()) {
            QJsonArray tools;
            for (const AiToolDef& t : req.tools) tools.append(ProviderCodec::openAiTool(t));
            root["tools"] = tools;
            root["tool_choice"] = "auto";
        }
        break;
    }
    return root;
}

// ---------- yanıt çözümleme ----------

QString ProviderCodec::openAiFinishReason(const QString& raw) {
    // Gemini "STOP", OpenAI "stop", Anthropic "end_turn" — hepsini karşıla
    const QString r = raw.trimmed().toLower();
    if (r == "stop" || r == "end_turn" || r == "stop_sequence") return "durdu";
    if (r == "length" || r == "max_tokens") return "uzunluk sınırı";
    if (r == "tool_calls" || r == "tool_use") return "araç çağrısı";
    if (r == "content_filter" || r == "safety") return "içerik filtresi";
    if (raw.isEmpty()) return "durdu";
    return raw;
}

static AiUsage usageFromOpenAi(const QJsonObject& o) {
    AiUsage u;
    const QJsonObject usage = o.value("usage").toObject();
    u.promptTokens = usage.value("prompt_tokens").toInt();
    u.evalTokens = usage.value("completion_tokens").toInt();
    const QJsonObject details = usage.value("completion_tokens_details").toObject();
    u.reasoningTokens = details.value("reasoning_tokens").toInt();
    return u;
}

static void collectOpenAiToolCalls(const QJsonValue& tcValue, QList<AiToolCall>& out) {
    for (const QJsonValue& v : tcValue.toArray()) {
        const QJsonObject o = v.toObject();
        AiToolCall c;
        c.id = o.value("id").toString();
        const QJsonObject fn = o.value("function").toObject();
        c.name = fn.value("name").toString();
        const QJsonValue args = fn.value("arguments");
        if (args.isString()) {
            c.args = QJsonDocument::fromJson(args.toString().toUtf8()).object();
        } else if (args.isObject()) {
            c.args = args.toObject();
        }
        if (!c.name.isEmpty()) out << c;
    }
}

AiReply ProviderCodec::parseReply(const ProviderSpec& spec, const QByteArray& body,
                                  const QString& model) {
    AiReply r;
    r.model = model;
    r.raw = QString::fromUtf8(body.left(20000));
    const QJsonDocument d = QJsonDocument::fromJson(body);
    if (!d.isObject()) {
        r.error = "Yanıt JSON değil";
        return r;
    }
    const QJsonObject o = d.object();
    if (o.contains("error")) {
        r.httpStatus = r.httpStatus ? r.httpStatus : 400;
        r.error = ProviderRegistry::mapError(spec, r.httpStatus, body);
        return r;
    }

    if (spec.kind == ProviderKind::Gemini) {
        r.ok = true;
        r.model = o.value("modelVersion").toString(model);
        const QJsonObject um = o.value("usageMetadata").toObject();
        r.usage.promptTokens = um.value("promptTokenCount").toInt();
        r.usage.evalTokens = um.value("candidatesTokenCount").toInt();
        r.usage.reasoningTokens = um.value("thoughtsTokenCount").toInt();
        const QJsonArray cands = o.value("candidates").toArray();
        if (cands.isEmpty()) {
            r.error = "Gemini aday yanıtı boş";
            r.ok = false;
            return r;
        }
        const QJsonObject cand = cands.first().toObject();
        const QJsonObject content = cand.value("content").toObject();
        for (const QJsonValue& p : content.value("parts").toArray()) {
            const QJsonObject po = p.toObject();
            if (po.contains("text")) r.text += po.value("text").toString();
            if (po.contains("thought") && po.value("thought").toBool())
                r.reasoning += po.value("text").toString();
            if (po.contains("functionCall")) {
                const QJsonObject fc = po.value("functionCall").toObject();
                AiToolCall c;
                c.name = fc.value("name").toString();
                c.args = fc.value("args").toObject();
                c.id = c.name;
                if (!c.name.isEmpty()) r.toolCalls << c;
            }
        }
        r.finishReason = openAiFinishReason(cand.value("finishReason").toString());
        return r;
    }

    if (spec.kind == ProviderKind::Anthropic) {
        r.ok = true;
        r.usage.promptTokens = o.value("usage").toObject().value("input_tokens").toInt();
        r.usage.evalTokens = o.value("usage").toObject().value("output_tokens").toInt();
        r.finishReason = o.value("stop_reason").toString();
        for (const QJsonValue& v : o.value("content").toArray()) {
            const QJsonObject b = v.toObject();
            const QString type = b.value("type").toString();
            if (type == "text") r.text += b.value("text").toString();
            else if (type == "thinking") r.reasoning += b.value("thinking").toString();
            else if (type == "tool_use") {
                AiToolCall c;
                c.id = b.value("id").toString();
                c.name = b.value("name").toString();
                c.args = b.value("input").toObject();
                if (!c.name.isEmpty()) r.toolCalls << c;
            }
        }
        return r;
    }

    // OpenAI-uyumlu + Ollama
    r.ok = true;
    r.usage = usageFromOpenAi(o);
    const QJsonArray choices = o.value("choices").toArray();
    if (choices.isEmpty()) {
        // Ollama /api/chat yanıtı: message.content
        const QString direct = o.value("message").toObject().value("content").toString();
        if (!direct.isEmpty()) {
            r.text = direct;
            r.model = o.value("model").toString(model);
            r.usage.promptTokens = o.value("prompt_eval_count").toInt();
            r.usage.evalTokens = o.value("eval_count").toInt();
            r.finishReason = "durdu";
            return r;
        }
        r.ok = false;
        r.error = "Seçenek listesi boş";
        return r;
    }
    const QJsonObject first = choices.first().toObject();
    const QJsonObject msg = first.value("message").toObject();
    r.text = msg.value("content").toString();
    if (msg.contains("reasoning_content")) r.reasoning = msg.value("reasoning_content").toString();
    collectOpenAiToolCalls(msg.value("tool_calls"), r.toolCalls);
    r.finishReason = openAiFinishReason(first.value("finish_reason").toString());
    r.model = o.value("model").toString(r.model);
    return r;
}

// ---------- akış olayı ----------

AiChunk ProviderCodec::parseStreamEvent(const ProviderSpec& spec, const QString& data,
                                        const QString& model) {
    AiChunk c;
    const QString d = data.trimmed();
    if (d.isEmpty() || d == "[DONE]") {
        c.done = true;
        return c;
    }
    const QJsonObject o = QJsonDocument::fromJson(d.toUtf8()).object();

    if (spec.kind == ProviderKind::Gemini) {
        const QJsonArray cands = o.value("candidates").toArray();
        if (!cands.isEmpty()) {
            const QJsonObject cand = cands.first().toObject();
            for (const QJsonValue& p : cand.value("content").toObject().value("parts").toArray()) {
                const QJsonObject po = p.toObject();
                if (po.contains("text")) {
                    if (po.value("thought").toBool()) c.reasoning += po.value("text").toString();
                    else c.text += po.value("text").toString();
                }
                if (po.contains("functionCall")) {
                    const QJsonObject fc = po.value("functionCall").toObject();
                    AiToolCall call;
                    call.name = fc.value("name").toString();
                    call.args = fc.value("args").toObject();
                    call.id = call.name;
                    if (!call.name.isEmpty()) c.toolCalls << call;
                }
            }
            if (cand.value("finishReason").toString().size()) {
                c.done = true;
                c.finishReason = openAiFinishReason(cand.value("finishReason").toString());
            }
        }
        const QJsonObject um = o.value("usageMetadata").toObject();
        if (!um.isEmpty()) {
            c.usage.promptTokens = um.value("promptTokenCount").toInt();
            c.usage.evalTokens = um.value("candidatesTokenCount").toInt();
            c.usage.reasoningTokens = um.value("thoughtsTokenCount").toInt();
        }
        return c;
    }

    if (spec.kind == ProviderKind::Anthropic) {
        const QString type = o.value("type").toString();
        if (type == "content_block_delta") {
            const QJsonObject delta = o.value("delta").toObject();
            if (delta.contains("text")) c.text += delta.value("text").toString();
            if (delta.contains("thinking")) c.reasoning += delta.value("thinking").toString();
            if (delta.contains("input_json_delta"))
                c.text += delta.value("input_json_delta").toString();
        } else if (type == "message_start") {
            c.usage.promptTokens =
                o.value("message").toObject().value("usage").toObject().value("input_tokens").toInt();
        } else if (type == "message_delta") {
            c.usage.evalTokens =
                o.value("usage").toObject().value("output_tokens").toInt();
            if (!o.value("delta").toObject().value("stop_reason").toString().isEmpty()) {
                c.done = true;
                c.finishReason = o.value("delta").toObject().value("stop_reason").toString();
            }
        } else if (type == "message_stop") {
            c.done = true;
        }
        return c;
    }

    // OpenAI-uyumlu
    const QJsonArray choices = o.value("choices").toArray();
    if (!choices.isEmpty()) {
        const QJsonObject first = choices.first().toObject();
        const QJsonObject delta = first.value("delta").toObject();
        if (delta.contains("content")) c.text += delta.value("content").toString();
        if (delta.contains("reasoning_content")) c.reasoning += delta.value("reasoning_content").toString();
        collectOpenAiToolCalls(delta.value("tool_calls"), c.toolCalls);
        const QString fr = first.value("finish_reason").toString();
        if (!fr.isEmpty()) {
            c.done = true;
            c.finishReason = openAiFinishReason(fr);
        }
    }
    if (o.contains("usage")) c.usage = usageFromOpenAi(o);
    return c;
}

// ---------- modeller ----------

QStringList ProviderCodec::parseModels(const ProviderSpec& spec, const QByteArray& body) {
    QStringList out;
    const QJsonObject root = QJsonDocument::fromJson(body).object();
    if (spec.kind == ProviderKind::Ollama) {
        for (const QJsonValue& v : root.value("models").toArray())
            out << v.toObject().value("name").toString();
        return out;
    }
    if (spec.kind == ProviderKind::Gemini) {
        for (const QJsonValue& v : root.value("models").toArray()) {
            const QString name = v.toObject().value("name").toString();
            if (!name.isEmpty()) out << name.section('/', -1);
        }
        return out;
    }
    // OpenAI-uyumlu: {"data":[{"id":...}]}
    const QJsonArray data = root.value("data").toArray();
    if (!data.isEmpty()) {
        for (const QJsonValue& v : data) {
            const QString id = v.toObject().value("id").toString();
            if (!id.isEmpty()) out << id;
        }
        return out;
    }
    // Anthropic: {"data":[{"id":...}]} — aynı şekilde
    for (const QJsonValue& v : root.value("models").toArray()) {
        const QString id = v.toObject().value("id").toString();
        if (!id.isEmpty()) out << id;
    }
    return out;
}

// ---------- gömme ----------

QJsonObject ProviderCodec::embedRequest(const ProviderSpec& spec, const QString& model,
                                        const QStringList& inputs, QString* resolvedModel) {
    QJsonObject root;
    if (spec.kind == ProviderKind::Gemini) {
        if (resolvedModel) *resolvedModel = "models/" + spec.resolveModelId(model);
        QJsonArray reqs;
        for (const QString& s : inputs)
            reqs.append(QJsonObject{{"model", "models/" + spec.resolveModelId(model)},
                                    {"content", QJsonObject{{"parts",
                                                             QJsonArray{QJsonObject{{"text", s}}}}}}});
        root["requests"] = reqs;
        return root;
    }
    if (resolvedModel) *resolvedModel = spec.resolveModelId(model);
    root["model"] = spec.resolveModelId(model);
    root["input"] = QJsonArray::fromStringList(inputs);
    // Not: Ollama /api/embed `input` dizisini de kabul eder; eski tek "prompt"
    // alanı yalnız bir girdide anlamlıydı, toplu istekleri sessizce düşürüyordu.
    return root;
}

QList<QList<float>> ProviderCodec::parseEmbed(const ProviderSpec& spec, const QByteArray& body) {
    QList<QList<float>> out;
    const QJsonObject root = QJsonDocument::fromJson(body).object();
    if (spec.kind == ProviderKind::Gemini) {
        for (const QJsonValue& v : root.value("embeddings").toArray()) {
            QList<float> vec;
            for (const QJsonValue& f : v.toObject().value("values").toArray())
                vec << float(f.toDouble());
            if (!vec.isEmpty()) out << vec;
        }
        return out;
    }
    for (const QJsonValue& v : root.value("data").toArray()) {
        QList<float> vec;
        for (const QJsonValue& f : v.toObject().value("embedding").toArray())
            vec << float(f.toDouble());
        if (!vec.isEmpty()) out << vec;
    }
    if (out.isEmpty() && root.contains("embedding")) {
        QList<float> vec;
        for (const QJsonValue& f : root.value("embedding").toArray()) vec << float(f.toDouble());
        if (!vec.isEmpty()) out << vec;
    }
    if (out.isEmpty() && root.contains("embeddings")) {
        for (const QJsonValue& v : root.value("embeddings").toArray()) {
            QList<float> vec;
            for (const QJsonValue& f : v.toArray()) vec << float(f.toDouble());
            if (!vec.isEmpty()) out << vec;
        }
    }
    return out;
}
