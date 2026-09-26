#include "ModelCapabilities.h"
#include "ai/ProviderPricing.h"
#include <QJsonArray>
#include <QJsonValue>
#include <initializer_list>

// Ada göre kaba yetenek sezgisi (sunucu /api/show vermediyse).
ModelCapabilities ModelCapabilities::fromName(const QString& model) {
    ModelCapabilities c;
    const QString m = model.toLower();
    auto any = [&m](std::initializer_list<const char*> needles) {
        for (const char* n : needles)
            if (m.contains(QString::fromLatin1(n))) return true;
        return false;
    };
    if (any({"embed", "nomic-embed", "bge-", "all-minilm", "mxbai-embed", "snowflake-arctic"}))
        c.embedding = true;
    if (any({"llava", "bakllava", "moondream", "minicpm-v", "qwen2.5vl", "qwen2-vl",
             "gemma3", "llama3.2-vision", "pixtral", "granite3.2-vision"}))
        c.vision = true;
    if (any({"llama3.1", "llama3.2", "llama3.3", "qwen2.5", "qwen3", "mistral",
             "mixtral", "firefunction", "command-r", "hermes3", "granite3", "devstral"}))
        c.tools = true;
    // Stage 36: düşünme kanalı + ücretsiz katman
    if (any({"deepseek-reasoner", "gpt-oss", "nemotron", "qwen3", "magistral", "r1",
             "reasoning", "thinking", "phi4-reason"}))
        c.reasoning = true;
    if (m.endsWith(":free")) c.freeTier = true;
    if (c.embedding) c.completion = false;
    c.family = m.section(':', 0, 0);
    return c;
}

ModelCapabilities ModelCapabilities::fromShow(const QJsonObject& show) {
    ModelCapabilities c;
    bool sawCaps = false;
    const QJsonArray caps = show.value("capabilities").toArray();
    if (!caps.isEmpty()) {
        sawCaps = true;
        c.completion = false;
        for (const QJsonValue& v : caps) {
            const QString s = v.toString().toLower();
            if (s == "completion") c.completion = true;
            else if (s == "tools") c.tools = true;
            else if (s == "vision") c.vision = true;
            else if (s == "embedding") c.embedding = true;
            else if (s == "thinking" || s == "reasoning") c.reasoning = true;
        }
    }
    const QJsonObject details = show.value("details").toObject();
    if (c.family.isEmpty()) c.family = details.value("family").toString();

    const QJsonObject info = show.value("model_info").toObject();
    for (auto it = info.begin(); it != info.end(); ++it) {
        if (it.key().endsWith(".context_length")) {
            c.contextLength = it.value().toInt();
            break;
        }
    }
    if (c.family.isEmpty()) {
        for (auto it = info.begin(); it != info.end(); ++it) {
            if (it.key().endsWith(".family")) {
                c.family = it.value().toString();
                break;
            }
        }
    }
    // capabilities yoksa aile adından sez
    if (!sawCaps && !c.family.isEmpty()) {
        ModelCapabilities byName = fromName(c.family);
        byName.contextLength = c.contextLength;
        byName.family = c.family;
        byName.reasoning = c.reasoning || byName.reasoning;
        return byName;
    }
    return c;
}

ModelCapabilities ModelCapabilities::detect(const QString& model, const QJsonObject& show) {
    ModelCapabilities byName = fromName(model);
    if (show.isEmpty()) return byName;
    ModelCapabilities c = fromShow(show);
    // Eksik alanları ad sezgisiyle tamamla
    if (c.family.isEmpty()) c.family = byName.family;
    if (c.contextLength == 0) c.contextLength = byName.contextLength;
    if (!c.tools && !c.embedding && byName.tools) {
        // capabilities alanı hiç yoksa (eski Ollama) ad sezgisi geçerli
        if (!show.contains("capabilities")) c.tools = byName.tools;
    }
    if (!c.vision && byName.vision && !show.contains("capabilities")) c.vision = true;
    if (!c.reasoning && byName.reasoning && !show.contains("capabilities")) c.reasoning = true;
    if (byName.freeTier) c.freeTier = true;
    return c;
}

ModelCapabilities ModelCapabilities::fromSpec(const ProviderSpec& spec, const QString& model) {
    ModelCapabilities c = fromName(model);
    // Sağlayıcı düzeyi yetenekler model sezgisi üstüne OR-lanır (bulut sağlayıcıda
    // model adından anlaşılmayan yetenekler genelde vardır).
    if (spec.supportsTools) c.tools = true;
    if (spec.supportsVision && !c.embedding) c.vision = true;
    if (!spec.supportsEmbed) c.embedding = false;   // sağlayıcı yapamıyorsa yetenek yok
    else if (c.embedding) c.embedding = true;
    if (spec.quirks & QReasoningEffort) c.reasoning = true;
    c.freeTier = ProviderPricing::isFreeTier(spec, model) || c.freeTier;
    c.contextLimit = 0;
    if (spec.id == "ollama" || spec.kind == ProviderKind::Ollama) {
        c.contextLimit = 131072; // kullanıcı num_ctx ile ayarlar
    } else if (spec.id == "gemini") {
        c.contextLimit = 1048576;
    } else if (spec.id == "anthropic") {
        c.contextLimit = 200000;
    } else if (spec.id == "nvidia-nim" || spec.id == "unorouter") {
        c.contextLimit = 131072;
    } else if (spec.id.startsWith("openai") || spec.id == "azure-openai") {
        c.contextLimit = 128000;
    } else {
        c.contextLimit = 32768; // güvenli varsayılan
    }
    if (c.family.isEmpty()) c.family = spec.id;
    return c;
}

ModelCapabilities ModelCapabilities::merge(const ModelCapabilities& a,
                                            const ModelCapabilities& b) {
    ModelCapabilities c;
    c.completion = a.completion || b.completion;
    c.tools = a.tools || b.tools;
    c.vision = a.vision || b.vision;
    c.embedding = a.embedding || b.embedding;
    c.reasoning = a.reasoning || b.reasoning;
    c.freeTier = a.freeTier || b.freeTier;
    c.family = b.family.isEmpty() ? a.family : b.family;
    c.contextLength = b.contextLength > 0 ? b.contextLength : a.contextLength;
    c.contextLimit = b.contextLimit > 0 ? b.contextLimit : a.contextLimit;
    if (c.embedding) c.completion = false;
    return c;
}

QStringList ModelCapabilities::badges() const {
    QStringList out;
    if (vision) out << "görü";
    if (tools) out << "araç";
    if (embedding) out << "gömme";
    if (reasoning) out << "düşünme";
    if (freeTier) out << "ücretsiz";
    return out;
}

bool ModelCapabilities::isEmbeddingModel(const QString& model) {
    const QString m = model.toLower();
    return m.contains("embed") || m.contains("nv-embed") || m.contains("e5-v") ||
           m.contains("minilm") || m.contains("bge-") || m.contains("arctic");
}
