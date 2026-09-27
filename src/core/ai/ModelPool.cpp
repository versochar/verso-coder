#include "ModelPool.h"
#include "../ModelCapabilities.h"
#include "ProviderPricing.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSettings>
#include <algorithm>
#include <QStandardPaths>

namespace {

QHash<QString, QStringList> g_catalog; // providerId -> modeller

QString poolFile() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/model-catalog.json");
}

} // namespace

QStringList ModelPool::catalogFor(const QString& providerId) {
    return g_catalog.value(providerId);
}

void ModelPool::setCatalog(const QString& providerId, const QStringList& models) {
    if (providerId.isEmpty()) return;
    QStringList clean;
    for (const QString& m : models) {
        const QString t = m.trimmed();
        if (!t.isEmpty() && !clean.contains(t)) clean << t;
    }
    g_catalog.insert(providerId, clean);
    saveToDisk();
}

QStringList ModelPool::catalog(const QString& providerId) { return catalogFor(providerId); }

bool ModelPool::hasCatalog(const QString& providerId) { return !catalogFor(providerId).isEmpty(); }

void ModelPool::clear() { g_catalog.clear(); }

void ModelPool::saveToDisk() {
    const QString f = poolFile();
    if (f.isEmpty()) return;
    QDir().mkpath(QFileInfo(f).absolutePath());
    QFile out(f);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    QJsonObject root;
    for (auto it = g_catalog.begin(); it != g_catalog.end(); ++it)
        root[it.key()] = QJsonArray::fromStringList(it.value());
    out.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

void ModelPool::loadFromDisk() {
    QFile in(poolFile());
    if (!in.open(QIODevice::ReadOnly)) return;
    const QJsonObject root = QJsonDocument::fromJson(in.readAll()).object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (!it.value().isArray()) continue;
        QStringList models;
        for (const QJsonValue& v : it.value().toArray()) models << v.toString();
        if (!models.isEmpty()) g_catalog.insert(it.key(), models);
    }
}

ModelPool::PriceClass ModelPool::priceClassOf(const ProviderSpec& spec, const QString& model) {
    if (model.trimmed().endsWith(QLatin1String(":free"))) return PriceClass::Free;
    if (ProviderPricing::isFreeTier(spec, model)) return PriceClass::Free;
    if (ProviderPricing::priceFor(spec, model).known) return PriceClass::Paid;
    return PriceClass::Unknown;
}

bool ModelPool::samePriceClass(const ProviderSpec& spec, const QString& a, const QString& b) {
    const PriceClass pa = priceClassOf(spec, a);
    const PriceClass pb = priceClassOf(spec, b);
    if (pa == pb) return true;
    // "Bilinmiyor" her iki tarafta da esnek sayılır (yanlış ücretlendirmeyi önler)
    if (pa == PriceClass::Unknown || pb == PriceClass::Unknown) return false;
    return false;
}

bool ModelPool::plausibleFreeModel(const QString& model) {
    return model.trimmed().endsWith(QLatin1String(":free"));
}

namespace {

// Sohbet OLMAYAN model aileleri (katalogda tür alanı yok, ada bakıyoruz)
const char* kNotChat[] = {
    // gömme / sıralama / denetleme
    "embed", "rerank", "moderation", "safeguard", "guard-", "guard", "clip",
    // görsel üretimi
    "sd1", "sd2", "sd3", "sdxl", "stable-diffusion", "diffusion", "dreamshaper",
    "anything-v5", "anything", "illustration", "realistic", "pony", "pixel", "pixart",
    "midjourney", "dall-e", "flux", "cyberrealistic", "absolutereality", "albedobase",
    "animerge", "anime", "amponyxl", "nsfw", "idc", "deliberate", "delib",
    // ses / müzik
    "tts", "whisper", "speech", "audio", "music", "voice", "sonic", "bark", "riffusion",
    // video
    "sora", "veo", "luma", "animate", "videogen", "mochi",
    // diğer
    "omni-moderation", "moderation",
};

// Sohbet olduğu bilinen aileler (yüksek öncelik)
const char* kChatFamily[] = {
    "llama", "qwen", "gemma", "mistral", "mixtral", "devstral", "ministral", "magistral",
    "deepseek", "glm", "phi", "nemotron", "granite", "olmo", "smol", "internlm",
    "falcon", "gpt-oss", "gpt-", "hermes", "command-r", "firefunction", "dbrx",
    "claude", "gemini", "exaone", "crystal", "yen", "ti-", "jamba", "dolly",
    "gpt4", "coder", "code", "agent", "moonshot", "kimi", "seed", "step", "openhermes",
};

} // namespace

bool ModelPool::isChatCapable(const QString& model) {
    const QString m = model.toLower();
    if (m.isEmpty()) return false;
    if (ModelCapabilities::isEmbeddingModel(m)) return false;
    for (const char* n : kNotChat)
        if (m.contains(QLatin1String(n))) return false;
    return true;
}

int ModelPool::chatScore(const QString& model) {
    const QString m = model.toLower();
    if (!isChatCapable(m)) return 0;
    int score = 10; // sohbet olabilir ama aile tanınmıyor
    for (const char* f : kChatFamily)
        if (m.contains(QLatin1String(f))) score += 40;
    // ":free" tercih edilir (maliyet)
    if (m.endsWith(QLatin1String(":free"))) score += 15;
    // devasa/deneysel modeller sona
    if (m.contains(QLatin1String("preview")) || m.contains(QLatin1String("experimental")))
        score -= 5;
    return score;
}

bool ModelPool::fitsPurpose(const QString& model, Need need) {
    const QString m = model.toLower();
    switch (need) {
    case Need::Embed:
        return ModelCapabilities::isEmbeddingModel(m) && !isChatCapable(m);
    case Need::Vision:
        // Görsel isteniyorsa sohbet dışı her şey elenir
        return isChatCapable(model);
    case Need::Chat:
    default:
        return isChatCapable(model);
    }
}

bool ModelPool::isKnownModel(const QString& providerId, const QString& model) {
    const QStringList cat = catalogFor(providerId);
    if (cat.isEmpty()) return false; // katalog yok: bilinmiyor
    if (cat.contains(model)) return true;
    // sağlayıcı öneki olmayan/önekli eşleşmeler
    const int slash = model.lastIndexOf(QLatin1Char('/'));
    if (slash > 0) return cat.contains(model.mid(slash + 1));
    return cat.contains(QLatin1Char('/') + model);
}

QList<QString> ModelPool::candidates(const ProviderSpec& spec, const QString& preferred,
                                     int maxCount, bool allowPaid, Need need) {
    QList<QString> out;
    if (!preferred.isEmpty()) out << preferred;
    if (maxCount <= 0) return out;
    const QStringList cat = catalogFor(spec.id);
    if (cat.isEmpty()) return out; // katalog yoksa alternatif üretilemez

    const PriceClass want = priceClassOf(spec, preferred);
    // 1) Önce aynı fiyat sınıfından adaylar (`:free` → `:free`)
    // 2) Sonra "bilinmiyor" sınıfı (katalog eskiyse)
    // 3) En son, yalnız izin varsa ücretli sınıf
    for (int pass = 0; pass < 3 && out.size() < maxCount; ++pass) {
        // Her geçişte adayları "sohbet olma olasılığı"na göre sırala: katalog
        // sırası keyfidir, ağgeçitler görsel/ses modellerini de karıştırır.
        QList<QPair<int, QString>> ranked;
        for (const QString& m : cat) {
            if (out.contains(m)) continue;
            if (!fitsPurpose(m, need)) continue;
            const PriceClass pc = priceClassOf(spec, m);
            bool take = false;
            if (pass == 0)
                take = (pc == want) || (want == PriceClass::Unknown && pc == PriceClass::Unknown);
            else if (pass == 1)
                take = (pc == PriceClass::Unknown) && want != PriceClass::Unknown;
            else
                take = allowPaid && pc == PriceClass::Paid;
            if (!take) continue;
            const int score = (need == Need::Embed) ? 1000 - int(cat.size()) : chatScore(m);
            ranked.append(qMakePair(score, m));
        }
        std::sort(ranked.begin(), ranked.end(),
                  [](const QPair<int, QString>& a, const QPair<int, QString>& b) {
                      return a.first != b.first ? a.first > b.first : a.second < b.second;
                  });
        for (const auto& r : ranked) {
            if (out.size() >= maxCount) break;
            out << r.second;
        }
    }
    return out;
}

QString ModelPool::nextCandidate(const ProviderSpec& spec, const QString& preferred,
                                 const QStringList& alreadyTried, bool allowPaid, Need need) {
    // Tercih edilen model zaten denendi (o yüzden yedek olamaz): listede
    // aday olsa bile döndürülmemeli.
    QStringList tried = alreadyTried;
    if (!preferred.isEmpty()) tried << preferred;
    const QList<QString> all = candidates(spec, preferred, 12, allowPaid, need);
    for (const QString& m : all)
        if (!tried.contains(m)) return m;
    return {};
}
