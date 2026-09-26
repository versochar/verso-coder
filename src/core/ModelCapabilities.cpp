#include "ModelCapabilities.h"
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
    return c;
}

QStringList ModelCapabilities::badges() const {
    QStringList out;
    if (vision) out << "görü";
    if (tools) out << "araç";
    if (embedding) out << "gömme";
    return out;
}
