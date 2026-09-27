#include "LlmProvider.h"
#include <QJsonDocument>
#include <QSettings>
#include <QStringList>

static const char* kGroup = "aiProviders";

QString ProviderSpec::url(const QString& path) const {
    QString b = baseUrl.trimmed();
    while (b.endsWith('/')) b.chop(1);
    if (path.isEmpty()) return b;
    return b + (path.startsWith('/') ? path : "/" + path);
}

QJsonObject ProviderSpec::authHeaders(const QString& apiKey) const {
    QJsonObject h;
    const QString k = apiKey.trimmed();
    if (auth == AuthStyle::None) return h;
    // Statik başlıklar anahtardan bağımsızdır (ör. Anthropic sürüm başlığı)
    if (auth == AuthStyle::AnthropicKey) h["anthropic-version"] = "2023-06-01";
    if (k.isEmpty()) return h;
    switch (auth) {
    case AuthStyle::Bearer:
        h["Authorization"] = "Bearer " + k;
        break;
    case AuthStyle::AnthropicKey:
        h["x-api-key"] = k;
        break;
    case AuthStyle::GoogleKey:
        h["x-goog-api-key"] = k;
        break;
    case AuthStyle::AzureApiKey:
        h["api-key"] = k;
        break;
    case AuthStyle::CustomHeader:
        h[apiKeyHeader.isEmpty() ? QString("Authorization") : apiKeyHeader] =
            apiKeyPrefix + k;
        break;
    case AuthStyle::None:
        break;
    }
    return h;
}

QJsonObject ProviderSpec::allHeaders(const QString& apiKey) const {
    QJsonObject h = authHeaders(apiKey);
    for (const QString& raw : extraHeaders) {
        const int c = raw.indexOf(':');
        if (c > 0)
            h[raw.left(c).trimmed()] = raw.mid(c + 1).trimmed();
    }
    return h;
}

QString ProviderSpec::resolveModelId(const QString& model) const {
    QString m = model.trimmed();
    if (m.isEmpty()) return m;
    if (quirks & QModelIdPrefix) {
        // NIM/UnoRouter "saglayici/model" ya da "saglayici:model" öneki ister
        if (!m.contains('/') && !m.contains(':') && m.section(':', 0, 0) != id)
            m = id + "/" + m;
    }
    return m;
}

bool ProviderSpec::isFreeModel(const QString& model) const {
    if (quirks & QFreeModelSuffix) return model.trimmed().endsWith(":free");
    return false;
}

QString ProviderSpec::badge() const {
    if (kind == ProviderKind::Ollama && !requiresKey()) return "yerel";
    if (isGateway()) return "ağgeçidi";
    if (quirks & QFreeModelSuffix) return "ücretsiz modeller";
    return {};
}

// --- Dahili ön ayarlar ---

QList<ProviderSpec> ProviderRegistry::builtin() {
    QList<ProviderSpec> out;

    auto add = [&out](const ProviderSpec& s) { out << s; };

    // --- Yerel / anahtarsız ---
    ProviderSpec ollama;
    ollama.id = "ollama";
    ollama.label = "Ollama (yerel)";
    ollama.kind = ProviderKind::Ollama;
    ollama.baseUrl = "http://localhost:11434";
    ollama.auth = AuthStyle::None;
    ollama.chatPath = "/api/chat";
    ollama.modelsPath = "/api/tags";
    ollama.embedPath = "/api/embed";
    ollama.supportsVision = true;
    ollama.supportsTools = true;
    ollama.supportsEmbed = true;
    ollama.hint = "Yerel model; internet gerekmez.";
    add(ollama);

    ProviderSpec lmstudio = ollama;
    lmstudio.id = "lmstudio";
    lmstudio.label = "LM Studio (yerel)";
    lmstudio.baseUrl = "http://localhost:1234/v1";
    lmstudio.auth = AuthStyle::None;
    lmstudio.chatPath = "/chat/completions";
    lmstudio.modelsPath = "/models";
    lmstudio.embedPath = "/embeddings";
    lmstudio.supportsEmbed = true;
    lmstudio.hint = "LM Studio yerel sunucusu (başlangıçta 'Start Server' gerekir).";
    add(lmstudio);

    ProviderSpec llamacpp = lmstudio;
    llamacpp.id = "llamacpp";
    llamacpp.label = "llama.cpp sunucusu";
    llamacpp.baseUrl = "http://localhost:8080/v1";
    llamacpp.hint = "llama-server / koboldcpp ile uyumlu.";
    add(llamacpp);

    ProviderSpec vllm = lmstudio;
    vllm.id = "vllm";
    vllm.label = "vLLM (yerel)";
    vllm.baseUrl = "http://localhost:8000/v1";
    vllm.hint = "vLLM OpenAI uyumlu sunucu.";
    add(vllm);

    // --- Bulut sağlayıcıları ---
    ProviderSpec openai;
    openai.id = "openai";
    openai.label = "OpenAI";
    openai.baseUrl = "https://api.openai.com/v1";
    openai.supportsVision = true;
    openai.supportsTools = true;
    openai.supportsEmbed = true;
    openai.quirks = QRequiresMaxTokens;
    add(openai);

    ProviderSpec anthropic;
    anthropic.id = "anthropic";
    anthropic.label = "Anthropic (Claude)";
    anthropic.kind = ProviderKind::Anthropic;
    anthropic.baseUrl = "https://api.anthropic.com/v1";
    anthropic.auth = AuthStyle::AnthropicKey;
    anthropic.chatPath = "/messages";
    anthropic.modelsPath = "/models";
    anthropic.supportsVision = true;
    anthropic.supportsTools = true;
    anthropic.quirks = QRequiresMaxTokens;
    anthropic.hint = "Anahtar: console.anthropic.com → API Keys";
    add(anthropic);

    ProviderSpec gemini;
    gemini.id = "gemini";
    gemini.label = "Google Gemini";
    gemini.kind = ProviderKind::Gemini;
    gemini.baseUrl = "https://generativelanguage.googleapis.com/v1beta";
    gemini.auth = AuthStyle::GoogleKey;
    gemini.chatPath = "/models/%1:generateContent";
    gemini.modelsPath = "/models";
    gemini.embedPath = "/models/%1:batchEmbedContents";
    gemini.supportsVision = true;
    gemini.supportsTools = true;
    gemini.supportsEmbed = true;
    gemini.hint = "Anahtar: aistudio.google.com/apikey";
    add(gemini);

    ProviderSpec nvidia;
    nvidia.id = "nvidia-nim";
    nvidia.label = "NVIDIA NIM";
    nvidia.baseUrl = "https://integrate.api.nvidia.com/v1";
    nvidia.supportsVision = true;
    nvidia.supportsTools = true;
    nvidia.supportsEmbed = true;
    nvidia.quirks = Quirks(QRequiresMaxTokens | QModelIdPrefix | QReasoningEffort);
    nvidia.hint = "build.nvidia.com ücretsiz kredi; model adı nvidia/ veya openai/ önekli.";
    add(nvidia);

    ProviderSpec nimLocal = nvidia;
    nimLocal.id = "nvidia-nim-local";
    nimLocal.label = "NVIDIA NIM (yerel)";
    nimLocal.baseUrl = "http://localhost:8000/v1";
    nimLocal.auth = AuthStyle::None;
    nimLocal.supportsEmbed = false;
    nimLocal.hint = "Kendi NIM konteyneriniz (docker run ... -p 8000:8000).";
    add(nimLocal);

    ProviderSpec unorouter;
    unorouter.id = "unorouter";
    unorouter.label = "UnoRouter";
    unorouter.baseUrl = "https://api.unorouter.com/v1";
    unorouter.supportsVision = true;
    unorouter.supportsTools = true;
    unorouter.supportsEmbed = true;
    unorouter.quirks = Quirks(QFreeModelSuffix | QModelIdPrefix | QNativeGateway);
    unorouter.hint = "Tek anahtarla 200+ model; birçok model :free sonekli.";
    add(unorouter);

    ProviderSpec unorouterAnthropic = unorouter;
    unorouterAnthropic.id = "unorouter-anthropic";
    unorouterAnthropic.label = "UnoRouter → Anthropic geçidi";
    unorouterAnthropic.kind = ProviderKind::Anthropic;
    unorouterAnthropic.auth = AuthStyle::AnthropicKey;
    unorouterAnthropic.chatPath = "/messages";
    unorouterAnthropic.supportsEmbed = false;
    unorouterAnthropic.hint = "Aynı UnoRouter anahtarı, native /v1/messages geçidi.";
    add(unorouterAnthropic);

    ProviderSpec unorouterGemini = unorouter;
    unorouterGemini.id = "unorouter-gemini";
    unorouterGemini.label = "UnoRouter → Gemini geçidi";
    unorouterGemini.kind = ProviderKind::Gemini;
    unorouterGemini.auth = AuthStyle::GoogleKey;
    unorouterGemini.chatPath = "/models/%1:generateContent";
    unorouterGemini.embedPath = "/models/%1:batchEmbedContents";
    unorouterGemini.hint = "Aynı UnoRouter anahtarı, native /v1beta geçidi.";
    add(unorouterGemini);

    auto openAiLike = [](const QString& id, const QString& label, const QString& base,
                         const QString& hint) {
        ProviderSpec s;
        s.id = id;
        s.label = label;
        s.baseUrl = base;
        s.supportsVision = true;
        s.supportsTools = true;
        s.supportsEmbed = true;
        s.quirks = QRequiresMaxTokens;
        s.hint = hint;
        return s;
    };

    add(openAiLike("groq", "Groq", "https://api.groq.com/openai/v1",
                   "Ücretsiz katman hızlı; anahtar: console.groq.com"));
    add(openAiLike("openrouter", "OpenRouter", "https://openrouter.ai/api/v1",
                   "Çoklu sağlayıcı tek anahtar; bazı modeller :free"));
    add(openAiLike("deepseek", "DeepSeek", "https://api.deepseek.com/v1",
                   "platform.deepseek.com"));
    add(openAiLike("mistral", "Mistral", "https://api.mistral.ai/v1",
                   "console.mistral.ai"));
    add(openAiLike("xai", "xAI (Grok)", "https://api.x.ai/v1", "console.x.ai"));
    add(openAiLike("together", "Together AI", "https://api.together.xyz/v1",
                   "api.together.ai"));
    add(openAiLike("openai-compatible", "OpenAI-uyumlu özel", "",
                   "Kendi sunucunuz; base URL ve anahtarı elle girin."));

    ProviderSpec azure;
    azure.id = "azure-openai";
    azure.label = "Azure OpenAI";
    azure.baseUrl = "https://RESOURCE.openai.azure.com/openai";
    azure.auth = AuthStyle::AzureApiKey;
    azure.chatPath = "/deployments/%1/chat/completions?api-version=2024-10-21";
    azure.modelsPath = "";
    azure.supportsVision = true;
    azure.supportsTools = true;
    azure.quirks = QRequiresMaxTokens;
    azure.hint = "base URL: https://<resource>.openai.azure.com/openai ; deployment adı = model";
    add(azure);

    return out;
}

QList<ProviderSpec> ProviderRegistry::custom() {
    QList<ProviderSpec> out;
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    for (const QString& id : q.childKeys()) {
        const QString blob = q.value(id).toString();
        if (blob.isEmpty()) continue;
        const QJsonObject o = QJsonDocument::fromJson(blob.toUtf8()).object();
        ProviderSpec s;
        s.builtin = false;
        s.id = o.value("id").toString(id);
        s.label = o.value("label").toString(s.id);
        s.baseUrl = o.value("baseUrl").toString();
        s.kind = static_cast<ProviderKind>(o.value("kind").toInt(0));
        s.auth = o.value("auth").toInt(0) == 0 ? AuthStyle::Bearer : AuthStyle::Bearer;
        s.chatPath = o.value("chatPath").toString("/chat/completions");
        s.modelsPath = o.value("modelsPath").toString("/models");
        s.embedPath = o.value("embedPath").toString("/embeddings");
        s.supportsVision = o.value("vision").toBool();
        s.supportsTools = o.value("tools").toBool();
        s.supportsEmbed = o.value("embed").toBool();
        s.quirks = static_cast<Quirks>(o.value("quirks").toInt());
        if (!s.id.isEmpty() && !s.baseUrl.isEmpty()) out << s;
    }
    return out;
}

QList<ProviderSpec> ProviderRegistry::all() {
    QList<ProviderSpec> out = builtin();
    for (const ProviderSpec& s : custom()) {
        bool replaced = false;
        for (ProviderSpec& b : out)
            if (b.id == s.id) { b = s; replaced = true; break; }
        if (!replaced) out << s;
    }
    return out;
}

ProviderSpec ProviderRegistry::byId(const QString& id) {
    const QString want = id.trimmed().toLower();
    for (const ProviderSpec& s : all())
        if (s.id == want) return s;
    return {};
}

bool ProviderRegistry::exists(const QString& id) { return byId(id).id == id.trimmed().toLower(); }

QStringList ProviderRegistry::ids() {
    QStringList out;
    for (const ProviderSpec& s : all()) out << s.id;
    return out;
}

QStringList ProviderRegistry::labels() {
    QStringList out;
    for (const ProviderSpec& s : all()) out << s.label;
    return out;
}

QList<ProviderSpec> ProviderRegistry::byKind(ProviderKind k) {
    QList<ProviderSpec> out;
    for (const ProviderSpec& s : all())
        if (s.kind == k) out << s;
    return out;
}

ProviderSpec ProviderRegistry::defaultProvider() { return byId("ollama"); }

ProviderSpec ProviderRegistry::makeCustom(const QString& id, const QString& label,
                                          const QString& baseUrl, ProviderKind kind) {
    ProviderSpec s;
    s.builtin = false;
    s.id = id.trimmed().toLower();
    s.label = label.trimmed().isEmpty() ? s.id : label.trimmed();
    s.baseUrl = baseUrl.trimmed();
    while (s.baseUrl.endsWith('/')) s.baseUrl.chop(1);
    s.kind = kind;
    s.supportsVision = true;
    s.supportsTools = true;
    s.supportsEmbed = true;
    s.quirks = QRequiresMaxTokens;
    return s;
}

bool ProviderRegistry::save(const ProviderSpec& s) {
    if (s.id.trimmed().isEmpty() || s.baseUrl.trimmed().isEmpty()) return false;
    const QJsonObject o{{"id", s.id},
                         {"label", s.label},
                         {"baseUrl", s.baseUrl},
                         {"kind", int(s.kind)},
                         {"auth", int(s.auth)},
                         {"chatPath", s.chatPath},
                         {"modelsPath", s.modelsPath},
                         {"embedPath", s.embedPath},
                         {"vision", s.supportsVision},
                         {"tools", s.supportsTools},
                         {"embed", s.supportsEmbed},
                         {"quirks", int(s.quirks)}};
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    q.setValue(s.id, QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)));
    return true;
}

bool ProviderRegistry::remove(const QString& id) {
    const ProviderSpec b = byId(id);            // dahili olan silinemez
    if (!b.id.isEmpty() && b.builtin) return false;
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    if (!q.contains(id)) return false;
    q.remove(id);
    return true;
}

QStringList ProviderRegistry::sampleModels(const QString& id) {
    if (id == "nvidia-nim" || id == "nvidia-nim-local")
        return {"nvidia/llama-3.1-70b-instruct", "nvidia/nemotron-4-340b-instruct",
                "openai/gpt-oss-120b", "nvidia/nv-embedqa-e5-v5"};
    if (id.startsWith("unorouter"))
        return {"gpt-oss-120b:free", "deepseek/deepseek-chat", "anthropic/claude-sonnet-4",
                "google/gemini-2.5-pro", "openai/gpt-4o-mini"};
    if (id == "anthropic")
        return {"claude-sonnet-4-5", "claude-opus-4-1", "claude-haiku-4-5"};
    if (id == "gemini" || id == "unorouter-gemini")
        return {"gemini-2.5-pro", "gemini-2.5-flash", "gemini-2.0-flash"};
    if (id == "openai")
        return {"gpt-4o", "gpt-4o-mini", "o3-mini", "text-embedding-3-small"};
    if (id == "groq") return {"llama-3.3-70b-versatile", "qwen3-32b", "moonshotai/kimi-k2"};
    if (id == "openrouter") return {"openai/gpt-4o-mini", "anthropic/claude-3.5-sonnet"};
    if (id == "deepseek") return {"deepseek-chat", "deepseek-reasoner"};
    return {};
}

QString ProviderRegistry::mapError(const ProviderSpec& spec, int status, const QByteArray& body) {
    // Sağlayıcının kendi hata gövdesinden kısa mesaj çıkar
    QString serverMsg;
    const QJsonObject o = QJsonDocument::fromJson(body).object();
    const QJsonObject err = o.value("error").toObject();
    if (!err.isEmpty()) serverMsg = err.value("message").toString();
    if (serverMsg.isEmpty()) serverMsg = o.value("message").toString();
    if (serverMsg.isEmpty()) {
        const QString raw = QString::fromUtf8(body.left(300)).trimmed();
        if (!raw.startsWith('{') && !raw.startsWith('[')) serverMsg = raw;
    }
    serverMsg = serverMsg.left(240).trimmed();

    auto with = [&](const QString& s) {
        return s + (serverMsg.isEmpty() ? QString() : " — " + serverMsg);
    };

    // Gerçek sağlayıcılarda hız sınırı 403/5xx ile gelir ve "rate limit /
    // per minute limit / try again" der. Anahtar hatası gibi göstermek yanıltıcı.
    {
        const QByteArray low = body.toLower();
        static const char* rl[] = {"rate limit", "rate_limit", "per minute", "per-minute",
                                   "try again",  "too many requests", "quota", "hız sınırı"};
        for (const char* n : rl) {
            if (!low.contains(n)) continue;
            if (status == 403 || status == 429 || (status >= 500 && status < 600))
                return with(QString("Hız sınırı (%1) — biraz bekleyip tekrar deneyin")
                                .arg(status));
        }
    }

    switch (status) {
    case 0: return with("Ağ hatası / zaman aşımı");
    case 400: return with("İstek reddedildi (400) — model adı veya gövde hatalı olabilir");
    case 401:
    case 403:
        if (spec.id == "nvidia-nim")
            return with("Yetki hatası (403) — build.nvidia.com'dan alınan NIM anahtarını "
                        "ve doğru endpoint'i kontrol et");
        return with("API anahtarı hatalı veya yetkisiz (" + QString::number(status) + ")");
    case 404:
        if (spec.id == "nvidia-nim" || spec.id == "nvidia-nim-local")
            return with("Model bulunamadı (404) — NIM'de model adı 'nvidia/' veya 'openai/' "
                        "önekiyle yazılır");
        if (spec.id == "azure-openai")
            return with("Deployment bulunamadı (404) — model alanına deployment adını yaz");
        return with("Model veya uç nokta bulunamadı (404)");
    case 413: return with("Gönderim çok büyük (413) — bağlamı kısalt");
    case 422:
        if (spec.quirks & QRequiresMaxTokens)
            return with("422 — bu sağlayıcı max_tokens istiyor; otomatik tamamlanamadı");
        return with("İstek doğrulanamadı (422)");
    case 429: return with("Kota/hız sınırı (429) — biraz bekleyip tekrar dene");
    case 500: return with("Sağlayıcı sunucu hatası (500)");
    case 502:
    case 503: return with("Servis geçici olarak kullanılamıyor (" + QString::number(status) + ")");
    case 504: return with("Zaman aşımı (504) — istek çok büyük olabilir");
    default: break;
    }
    return with(QString("HTTP %1").arg(status));
}
