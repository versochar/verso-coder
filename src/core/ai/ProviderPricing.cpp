#include "ProviderPricing.h"
#include <QHash>
#include <QStringList>

namespace {

// Sağlayıcı → (model anahtarı → {giriş, çıkış}) — USD / 1M token.
// Anahtarlar küçük harf; eşleşme "başlangıçta" (en uzun anahtar önce).
struct Row {
    const char* provider;
    const char* key;
    double in;
    double out;
};

// Bilinen fiyatlar. Bilinmeyenler `known=false` döner (0 USD + uyarı).
const Row kTable[] = {
    // --- OpenAI ---
    {"openai", "gpt-4o-mini", 0.15, 0.6},
    {"openai", "gpt-4o", 2.5, 10.0},
    {"openai", "gpt-4.1-mini", 0.4, 1.6},
    {"openai", "gpt-4.1-nano", 0.1, 0.4},
    {"openai", "gpt-4.1", 2.0, 8.0},
    {"openai", "gpt-4-turbo", 10.0, 30.0},
    {"openai", "o3-mini", 1.1, 4.4},
    {"openai", "o3", 2.0, 8.0},
    {"openai", "text-embedding-3-small", 0.02, 0.0},
    {"openai", "text-embedding-3-large", 0.13, 0.0},
    // --- Anthropic ---
    {"anthropic", "claude-haiku-4", 0.8, 4.0},
    {"anthropic", "claude-opus-4", 15.0, 75.0},
    {"anthropic", "claude-sonnet-4", 3.0, 15.0},
    {"anthropic", "claude-3-5-sonnet", 3.0, 15.0},
    {"anthropic", "claude-3-opus", 15.0, 75.0},
    // --- Gemini ---
    {"gemini", "gemini-2.5-flash-lite", 0.10, 0.40},
    {"gemini", "gemini-2.5-flash", 0.30, 2.50},
    {"gemini", "gemini-2.5-pro", 1.25, 10.0},
    {"gemini", "text-embedding-004", 0.0, 0.0},
    // --- Groq ---
    {"groq", "llama-3.3-70b", 0.59, 0.79},
    {"groq", "llama-3.1-8b", 0.05, 0.08},
    {"groq", "qwen3-32b", 0.29, 0.59},
    // --- DeepSeek ---
    {"deepseek", "deepseek-chat", 0.27, 1.10},
    {"deepseek", "deepseek-reasoner", 0.55, 2.19},
    // --- Mistral ---
    {"mistral", "mistral-large", 2.0, 6.0},
    {"mistral", "mistral-small", 0.2, 0.6},
    {"mistral", "codestral", 0.3, 0.9},
    // --- xAI ---
    {"xai", "grok-4", 3.0, 15.0},
    {"xai", "grok-3-mini", 0.3, 0.5},
    // --- NIM: geliştirme kredisi ücretsiz (build.nvidia.com) ---
    {"nvidia-nim", "", 0.0, 0.0},
    // --- NIM gömme modelleri ---
    {"nvidia-nim", "nv-embedqa", 0.0, 0.0},
};

// Sağlayıcıdan bağımsız genel eşleşme (UnoRouter/OpenRouter birçok satıcıyı sunar).
const Row kGlobal[] = {
    {"", "gpt-4o-mini", 0.15, 0.6},
    {"", "gpt-4o", 2.5, 10.0},
    {"", "claude-opus-4", 15.0, 75.0},
    {"", "claude-sonnet-4", 3.0, 15.0},
    {"", "claude-haiku", 0.8, 4.0},
    {"", "gemini-2.5-flash", 0.30, 2.50},
    {"", "gemini-2.5-pro", 1.25, 10.0},
    {"", "deepseek-chat", 0.27, 1.10},
    {"", "gpt-oss-120b", 0.0, 0.0},
};

QString normModel(const QString& model) {
    QString m = model.trimmed().toLower();
    // Sağlayıcı önekini at (nvidia/x, openai/x, vendor/x) ama geçmişi koru
    const int slash = m.lastIndexOf('/');
    if (slash > 0 && slash < m.size() - 1) m = m.mid(slash + 1);
    return m;
}

const Row* lookup(const Row* table, size_t n, const QString& providerId, const QString& model) {
    const QString m = normModel(model);
    const Row* best = nullptr;
    for (size_t i = 0; i < n; ++i) {
        if (providerId.isEmpty() && table[i].provider[0]) continue;
        if (!providerId.isEmpty() && qstrcmp(table[i].provider, providerId.toLatin1()) != 0)
            continue;
        const QString key = QString::fromLatin1(table[i].key);
        if (key.isEmpty() || m.startsWith(key)) {
            // En uzun eşleşen anahtar kazanır
            if (!best || key.size() > QString::fromLatin1(best->key).size()) best = &table[i];
        }
    }
    return best;
}

} // namespace

bool ProviderPricing::isFreeTier(const ProviderSpec& spec, const QString& model) {
    if (spec.kind == ProviderKind::Ollama) return true;         // yerel
    if (spec.id == "nvidia-nim" || spec.id == "nvidia-nim-local") return true; // geliştirme kredisi
    if (model.trimmed().endsWith(":free")) return true;
    if (spec.quirks & QFreeModelSuffix) {
        // Bu sağlayıcıda ücretsiz modeller var; ":free" yoksa ücretli varsay
        return spec.isFreeModel(model);
    }
    if (!spec.requiresKey() && spec.kind != ProviderKind::Ollama) return true; // anahtarsız yerel uyumlu
    return false;
}

ModelPrice ProviderPricing::priceFor(const ProviderSpec& spec, const QString& model) {
    ModelPrice p;
    if (isFreeTier(spec, model)) {
        p.freeTier = true;
        p.known = true;
        return p; // hepsi 0
    }
    const Row* r = lookup(kTable, sizeof(kTable) / sizeof(Row), spec.id, model);
    if (r) {
        p.in = r->in;
        p.out = r->out;
        p.cachedIn = r->in * 0.1; // önbellek girişleri ~%10
        p.known = true;
        return p;
    }
    r = lookup(kGlobal, sizeof(kGlobal) / sizeof(Row), QString(), model);
    if (r) {
        p.in = r->in;
        p.out = r->out;
        p.cachedIn = r->in * 0.1;
        p.known = true;
    }
    return p;
}

ModelPrice ProviderPricing::priceFor(const QString& providerId, const QString& model) {
    return priceFor(ProviderRegistry::byId(providerId), model);
}

double ProviderPricing::estimateUsd(const ProviderSpec& spec, const QString& model,
                                    int promptTokens, int evalTokens, int cachedTokens,
                                    bool batch) {
    const ModelPrice p = priceFor(spec, model);
    if (p.isZero()) return 0.0;
    const double factor = batch ? batchFactor() : 1.0;
    const int fresh = qMax(0, promptTokens - qMax(0, cachedTokens));
    const double usd = (fresh / 1'000'000.0) * p.in * factor +
                       (qMax(0, cachedTokens) / 1'000'000.0) * p.cachedIn * factor +
                       (qMax(0, evalTokens) / 1'000'000.0) * p.out * factor;
    return usd;
}

QString ProviderPricing::explain(const ProviderSpec& spec, const QString& model) {
    const ModelPrice p = priceFor(spec, model);
    if (p.freeTier) {
        if (spec.kind == ProviderKind::Ollama) return "yerel model — ücretsiz";
        if (spec.id.startsWith("nvidia-nim")) return "NIM geliştirme kredisi — ücretsiz";
        if (model.endsWith(":free")) return "ücretsiz model (':free')";
        return "ücretsiz";
    }
    if (!p.known) return "fiyat bilinmiyor (≈0 varsayıldı)";
    return QString("≈$%1 / 1M giriş · $%2 / 1M çıkış").arg(p.in, 0, 'f', 4).arg(p.out, 0, 'f', 2);
}

bool ProviderPricing::priceOverridable(const ProviderSpec& spec) {
    // Yalnız ücretli, anahtar gerektiren sağlayıcılarda elle fiyat girilir
    return spec.requiresKey() && spec.kind != ProviderKind::Ollama;
}

ModelPrice ProviderPricing::withOverride(const ModelPrice& base, double inPerM, double outPerM) {
    ModelPrice p = base;
    if (inPerM > 0.0) {
        p.in = inPerM;
        p.known = true;
        if (p.cachedIn == 0.0) p.cachedIn = inPerM * 0.1;
    }
    if (outPerM > 0.0) {
        p.out = outPerM;
        p.known = true;
    }
    return p;
}
