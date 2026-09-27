#include "SetupAdvisor.h"
#include "ai/LlmProvider.h"
#include "ai/ProviderHealth.h"
#include "ai/ProviderPrefs.h"
#include "ai/SecretStore.h"
#include "ai/UsageLedger.h"
#include <algorithm>

QString SetupAdvisor::keyHintFor(const QString& providerId) {
    const ProviderSpec s = ProviderRegistry::byId(providerId);
    if (s.id.isEmpty()) return {};
    if (s.hint.contains("build.nvidia.com")) return QStringLiteral("build.nvidia.com → API Keys");
    if (s.id == "unorouter") return QStringLiteral("unorouter.com/en/token");
    if (s.id == "openai") return QStringLiteral("platform.openai.com → API keys");
    if (s.id == "anthropic") return QStringLiteral("console.anthropic.com → API Keys");
    if (s.id == "gemini") return QStringLiteral("aistudio.google.com/apikey");
    if (s.id == "groq") return QStringLiteral("console.groq.com/keys");
    if (s.id == "deepseek") return QStringLiteral("platform.deepseek.com → API keys");
    if (s.id == "mistral") return QStringLiteral("console.mistral.ai → API keys");
    if (s.id == "xai") return QStringLiteral("console.x.ai → API keys");
    if (s.id == "together") return QStringLiteral("api.together.ai → API keys");
    if (s.id == "openrouter") return QStringLiteral("openrouter.ai/keys");
    if (s.id == "azure-openai") return QStringLiteral("Azure portal → kaynak → anahtarlar");
    return s.hint;
}

bool SetupAdvisor::hasKey(const QString& providerId) {
    const ProviderSpec s = ProviderRegistry::byId(providerId);
    if (!s.requiresKey()) return true; // anahtar istemiyor
    SecretStore store;
    return !store.effectiveKey(providerId).isEmpty();
}

QList<SetupAction> SetupAdvisor::analyzeProvider(const QString& providerId, bool networkUp) {
    QList<SetupAction> actions;
    const ProviderSpec s = ProviderRegistry::byId(providerId);
    if (s.id.isEmpty()) {
        actions << SetupAction{"Sağlayıcı tanınmıyor",
                              QStringLiteral("Seçili sağlayıcı kayıtlı değil: %1").arg(providerId),
                              QStringLiteral("Ayarlar → AI Sağlayıcıları'ndan seçin"),
                              QStringLiteral("view.settings"), SetupAction::Critical, providerId};
        return actions;
    }
    if (!networkUp) {
        actions << SetupAction{
            QStringLiteral("Ağ yok — AI istekleri başarısız olur"),
            QStringLiteral("%1 için bağlantı kurulamadı").arg(s.label),
            QStringLiteral("Ağ bağlantısını denetleyin; yerel Ollama ağ gerektirmez"),
            QStringLiteral("view.problems"), SetupAction::Critical, providerId};
        return actions;
    }
    if (s.requiresKey() && !hasKey(providerId)) {
        actions << SetupAction{
            QStringLiteral("%1 için API anahtarı gerekli").arg(s.label),
            QStringLiteral("Anahtar kasada yok; istekler 401 alır"),
            keyHintFor(providerId).isEmpty() ? QStringLiteral("Anahtarı girin")
                                             : QStringLiteral("Anahtar: %1").arg(keyHintFor(providerId)),
            QStringLiteral("view.settings"), SetupAction::Critical, providerId};
    }
    // Kota
    QString why;
    if (UsageLedger::instance().quotaExceeded(providerId, why)) {
        actions << SetupAction{QStringLiteral("%1 kotası doldu").arg(s.label), why,
                              QStringLiteral("Kota tavanını artırın ya da yarın tekrar deneyin"),
                              QStringLiteral("view.settings"), SetupAction::Warning, providerId};
    }
    // Sağlık / sigorta
    const ProviderHealth& health = ProviderHealth::instance();
    if (health.isTripped(providerId)) {
        const int left = health.cooldownLeft(providerId);
        actions << SetupAction{
            QStringLiteral("%1 geçici olarak devre dışı").arg(s.label),
            QStringLiteral("Arka arkaya yoğun hata: %1 sn soğuma bekleniyor").arg(left),
            QStringLiteral("Bekleyin veya başka bir sağlayıcı seçin"),
            QStringLiteral("view.settings"), SetupAction::Warning, providerId};
    } else if (!health.isUsable(providerId)) {
        actions << SetupAction{
            QStringLiteral("%1 sağlıksız görünüyor").arg(s.label),
            health.statusLine(providerId),
            QStringLiteral("Bağlantıyı test edin"),
            QStringLiteral("view.settings"), SetupAction::Warning, providerId};
    }
    // Model yok / yedek yok
    if (!s.supportsEmbed && !s.supportsTools && s.kind == ProviderKind::Ollama) {
        // yerel sunucu: araç/gömme yoksa uyarı değil, bilgi
    }
    if (s.hint.isEmpty() && !s.requiresKey() && s.kind != ProviderKind::Ollama
        && s.id != ProviderPrefs::activeProvider()) {
        actions << SetupAction{QStringLiteral("%1 anahtar istemiyor").arg(s.label),
                              QStringLiteral("Yerel/uyumlu sunucu olarak kullanılıyor"),
                              QStringLiteral("Sunucunun çalıştığından emin olun"),
                              QStringLiteral("view.problems"), SetupAction::Info, providerId};
    }
    return actions;
}

QList<SetupAction> SetupAdvisor::analyze(bool networkUp) {
    QList<SetupAction> all;
    const QString active = ProviderPrefs::activeProvider();
    all += analyzeProvider(active, networkUp);
    // Etkin sağlayıcı dışındakilerden de hazır olanları öner
    // Hiçbir BULUT sağlayıcısının anahtarı yoksa ücretsiz seçenekleri hatırlat.
    // (Yerel Ollama her zaman vardır; bu yüzden "hiçbir şey yok" durumu değil,
    // "ücretli katmana hiç erişimin yok" durumu bildirilir.)
    bool anyKeyed = false;
    for (const ProviderSpec& s : ProviderRegistry::all()) {
        if (s.id == active) continue;
        if (s.requiresKey() && hasKey(s.id)) {
            anyKeyed = true;
            break;
        }
    }
    if (!anyKeyed) {
        all << SetupAction{
            QStringLiteral("Ücretli sağlayıcı için anahtar yok — ücretsiz seçenekler"),
            QStringLiteral("Etkin sağlayıcı: %1. Yerel Ollama dışında ücretsiz katman "
                           "sunucu gerektiren sağlayıcılar var.")
                .arg(ProviderRegistry::byId(active).label),
            QStringLiteral("NVIDIA NIM (build.nvidia.com) veya UnoRouter "
                           "ücretsiz katman sunar; anahtarı girin"),
            QStringLiteral("view.settings"), SetupAction::Info, QString()};
    }
    std::stable_sort(all.begin(), all.end(), [](const SetupAction& a, const SetupAction& b) {
        return int(a.severity) > int(b.severity);
    });
    return all;
}

SetupAction SetupAdvisor::mostCritical(const QList<SetupAction>& actions) {
    for (const SetupAction& a : actions)
        if (a.severity == SetupAction::Critical) return a;
    return actions.isEmpty() ? SetupAction{} : actions.first();
}

QString SetupAdvisor::summary(const QList<SetupAction>& actions) {
    if (actions.isEmpty())
        return QStringLiteral("Her şey hazır ✓");
    int critical = 0, warning = 0;
    for (const SetupAction& a : actions) {
        if (a.severity == SetupAction::Critical) ++critical;
        else if (a.severity == SetupAction::Warning) ++warning;
    }
    QStringList parts;
    if (critical) parts << QStringLiteral("%1 kritik").arg(critical);
    if (warning) parts << QStringLiteral("%1 uyarı").arg(warning);
    const int info = int(actions.size()) - critical - warning;
    if (info) parts << QStringLiteral("%1 bilgi").arg(info);
    const SetupAction first = actions.first();
    return QStringLiteral("%1 · önce: %2").arg(parts.join(QStringLiteral(", ")), first.title);
}
