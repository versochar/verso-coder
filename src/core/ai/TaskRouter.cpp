#include "TaskRouter.h"
#include "ProviderPrefs.h"
#include "SecretStore.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStringList>

namespace {

bool hasAny(const QString& t, const QStringList& needles) {
    for (const QString& n : needles)
        if (t.contains(n)) return true;
    return false;
}

const QStringList kVision = {"görsel", "ekran görüntüsü", "screenshot", "resim", "fotoğraf",
                             "image", "ui'ı incele", "arayüzü incele"};
const QStringList kComplex = {"refactor", "yeniden yaz", "mimari", "mimari", "design",
                               "yeniden tasarla", "test yaz", "testleri", "optimizasyon",
                               "performans", "hata ayıkla", "debug", "race condition",
                               "eşzamanl", "thread", "bellek sızıntısı", "migration",
                               "taşı", "entegrasyon", "uçtan uca", "ci/CD", "pipeline"};
const QStringList kAgent = {"kendin", "otomatik", "kendi başına", "bütün projeyi",
                            "tüm dosyaları", "bütün repo", "bağımsız yap"};
const QStringList kTrivial = {"ne demek", "nedir", "ne işe yarar", "kısaca", "özetle",
                              "yazım hatası", "format", "biçimlendir", "adlandır",
                              "sözlük anlamı"};
const QStringList kEmbed = {"göm", "embedding", "vektör", "benzerlik", "anlamsal arama"};

} // namespace

QString TaskRouter::label(TaskClass c) {
    switch (c) {
    case TaskClass::Trivial: return " önemsiz";
    case TaskClass::Simple: return "basit";
    case TaskClass::Medium: return "orta";
    case TaskClass::Complex: return "karmaşık";
    case TaskClass::Agent: return "ajan";
    case TaskClass::Vision: return "görsel";
    case TaskClass::Embed: return "gömme";
    }
    return "?";
}

int TaskRouter::complexityScore(const QString& task) {
    const QString t = task.toLower();
    int score = 0;
    score += qMin(25, task.size() / 200);                       // uzunluk
    // Her farklı karmaşık işaret puan ekler (yığılgan, üstel değil)
    int complexHits = 0;
    for (const QString& n : kComplex)
        if (t.contains(n)) ++complexHits;
    score += qMin(40, complexHits * 15);
    if (hasAny(t, kAgent)) score += 20;
    if (task.count(QLatin1Char('\n')) >= 4) score += 10;       // çok satırlı kod
    if (task.contains("```")) score += 8;                       // kod bloğu
    if (task.contains("?") && task.size() < 80) score -= 15;    // kısa soru
    if (hasAny(t, kTrivial)) score -= 20;
    return qBound(0, score, 100);
}

QStringList TaskRouter::reasons(const QString& task) {
    const QString t = task.toLower();
    QStringList out;
    if (hasAny(t, kComplex)) out << "karmaşık değişiklik";
    if (hasAny(t, kAgent)) out << "özerk görev";
    if (hasAny(t, kVision)) out << "görsel girdi";
    if (hasAny(t, kTrivial)) out << "kısa bilgi sorusu";
    if (task.count(QLatin1Char('\n')) >= 4) out << "kod bloğu içeriyor";
    if (task.size() > 400) out << "uzun istem";
    if (out.isEmpty()) out << "sıradan istek";
    return out;
}

TaskClass TaskRouter::classify(const QString& task) {
    const QString t = task.toLower();
    if (hasAny(t, kEmbed)) return TaskClass::Embed;
    if (hasAny(t, kVision)) return TaskClass::Vision;
    if (hasAny(t, kAgent)) return TaskClass::Agent;
    const int score = complexityScore(task);
    if (hasAny(t, kTrivial) && score < 25) return TaskClass::Trivial;
    if (score >= 40) return TaskClass::Complex;
    if (score >= 20) return TaskClass::Medium;
    return TaskClass::Simple;
}

QString TaskRouter::defaultEmbedModel(const QString& providerId) {
    const ProviderSpec spec = ProviderRegistry::byId(providerId);
    if (!spec.supportsEmbed) return {};
    if (spec.id == "nvidia-nim") return "nvidia/nv-embedqa-e5-v5";
    if (spec.id == "openai") return "text-embedding-3-small";
    if (spec.id == "gemini") return "text-embedding-004";
    if (spec.id == "ollama") return "nomic-embed-text";
    // Ağgeçitlerde katalog değişebilir; doğrulanmış ve ücretsiz bir model seçilir
    // (openai/text-embedding-3-small UnoRouter'da sunulmuyor).
    if (spec.id == "unorouter") return "jina-embeddings-v3:free";
    const QStringList samples = ProviderRegistry::sampleModels(spec.id);
    for (const QString& m : samples)
        if (m.contains("embed")) return m;
    return {};
}

TaskRouter::Route TaskRouter::route(const QString& task, const Prefs& p) {
    Route r;
    r.cls = classify(task);
    if (!p.enabled) {
        r.ok = true;
        r.providerId = p.forceProvider.isEmpty() ? ProviderPrefs::activeProvider() : p.forceProvider;
        r.model = p.forceModel;
        r.reason = "yönlendirme kapalı — etkin sağlayıcı";
        r.forceLocked = true;
        return r;
    }
    // 1) Manuel kilit
    if (!p.forceProvider.isEmpty()) {
        r.ok = true;
        r.providerId = p.forceProvider;
        r.model = p.forceModel;
        r.reason = "manuel kilit";
        r.forceLocked = true;
        return r;
    }
    // 2) Görev sınıfına göre
    const bool heavy = r.cls == TaskClass::Complex || r.cls == TaskClass::Agent ||
                       r.cls == TaskClass::Vision;
    QString wantProv = heavy ? p.strongProvider : p.quickProvider;
    QString wantModel = heavy ? p.strongModel : p.quickModel;

    if (wantProv.isEmpty()) wantProv = ProviderPrefs::activeProvider();
    ProviderSpec spec = ProviderPrefs::resolve(wantProv);
    if (wantModel.isEmpty())
        wantModel = ProviderPrefs::modelFor(spec.id, ProviderRegistry::sampleModels(spec.id).value(0));

    // 3) Yetenek uyumu: görsel isteniyorsa görsel destekleyen yedeğe geç
    if (r.cls == TaskClass::Vision && !spec.supportsVision) {
        for (const ProviderSpec& cand : ProviderRegistry::all()) {
            if (!cand.supportsVision) continue;
            if (cand.requiresKey() && SecretStore().effectiveKey(cand.id).isEmpty()) continue;
            spec = cand;
            wantModel.clear();
            wantModel = ProviderPrefs::modelFor(cand.id,
                                                ProviderRegistry::sampleModels(cand.id).value(0));
            r.reason = "görsel destekleyen sağlayıcıya yönlendirildi";
            break;
        }
    }
    // 4) Ücretsiz tercihi: bilinen ücretsiz bir sağlayıcı anahtarsız ise
    if (p.useFreeFirst) {
        if (spec.kind == ProviderKind::Ollama && heavy) {
            // Karmaşık işi ücretsiz yerel modele yükleme
            for (const ProviderSpec& cand : ProviderRegistry::all()) {
                if (cand.kind == ProviderKind::Ollama) continue;
                if (cand.requiresKey() && SecretStore().effectiveKey(cand.id).isEmpty()) continue;
                spec = cand;
                r.reason = "karmaşık görev için ücretli/güçlü sağlayıcı";
                break;
            }
        }
    }
    r.ok = !spec.id.isEmpty();
    r.providerId = spec.id;
    r.model = wantModel;
    if (r.reason.isEmpty())
        r.reason = QString("%1 görev → %2")
                       .arg(label(r.cls),
                            spec.label);
    return r;
}

TaskRouter::Route TaskRouter::route(const QString& task) { return route(task, prefs()); }

QString TaskRouter::embedProvider(const Prefs& p) {
    if (!p.embedProvider.isEmpty()) return p.embedProvider;
    const QString active = ProviderPrefs::activeProvider();
    const ProviderSpec spec = ProviderPrefs::resolve(active);
    if (spec.supportsEmbed) return spec.id;
    // Etkin sağlayıcı gömme desteklemiyorsa destekleyen ilkini seç
    for (const ProviderSpec& cand : ProviderRegistry::all()) {
        if (!cand.supportsEmbed) continue;
        if (cand.requiresKey() && SecretStore().effectiveKey(cand.id).isEmpty()) continue;
        return cand.id;
    }
    return active;
}

QString TaskRouter::embedModel(const Prefs& p) {
    const QString prov = embedProvider(p);
    if (!p.embedModel.isEmpty() &&
        (p.embedProvider.isEmpty() || p.embedProvider == prov))
        return p.embedModel;
    return defaultEmbedModel(prov);
}

TaskRouter::Prefs TaskRouter::defaultPrefs() { return Prefs{}; }

TaskRouter::Prefs TaskRouter::readSettings() {
    Prefs p;
    QSettings s;
    s.beginGroup(QStringLiteral("ai/routing"));
    p.enabled = s.value("enabled", true).toBool();
    p.forceProvider = s.value("forceProvider").toString();
    p.forceModel = s.value("forceModel").toString();
    p.quickProvider = s.value("quickProvider").toString();
    p.quickModel = s.value("quickModel").toString();
    p.strongProvider = s.value("strongProvider").toString();
    p.strongModel = s.value("strongModel").toString();
    p.embedProvider = s.value("embedProvider").toString();
    p.embedModel = s.value("embedModel").toString();
    p.useFreeFirst = s.value("useFreeFirst", true).toBool();
    s.endGroup();
    return p;
}

void TaskRouter::writeSettings(const Prefs& p) {
    QSettings s;
    s.beginGroup(QStringLiteral("ai/routing"));
    s.setValue("enabled", p.enabled);
    s.setValue("forceProvider", p.forceProvider);
    s.setValue("forceModel", p.forceModel);
    s.setValue("quickProvider", p.quickProvider);
    s.setValue("quickModel", p.quickModel);
    s.setValue("strongProvider", p.strongProvider);
    s.setValue("strongModel", p.strongModel);
    s.setValue("embedProvider", p.embedProvider);
    s.setValue("embedModel", p.embedModel);
    s.setValue("useFreeFirst", p.useFreeFirst);
    s.endGroup();
    s.sync();
}

TaskRouter::Prefs TaskRouter::prefs() { return readSettings(); }

void TaskRouter::setPrefs(const Prefs& p) { writeSettings(p); }

void TaskRouter::resetPrefs() { writeSettings(Prefs{}); }
