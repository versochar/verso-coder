#include "EmbedBridge.h"
#include "LlmClient.h"
#include "ProviderPrefs.h"
#include "SecretStore.h"
#include "TaskRouter.h"
#include <QSet>

EmbedBridge::EmbedBridge(QObject* parent) {
    // Kasa bağlanmasa da anahtar bulunabilsin (aksi halde istek 401 alır ve
    // "anahtar eksik" yerine sunucu hatası görünür)
    if (!m_secrets) m_secrets = new SecretStore();
    m_provider = resolveProvider();
    m_model = resolveModel(m_provider);
}

EmbedBridge::~EmbedBridge() = default;

LlmClient& EmbedBridge::client() { return m_client; }

QString EmbedBridge::resolveProvider() {
    return TaskRouter::embedProvider(TaskRouter::prefs());
}

bool EmbedBridge::available(const QString& providerId) {
    const ProviderSpec spec = ProviderRegistry::byId(providerId);
    if (!spec.supportsEmbed) return false;
    return !resolveModel(providerId).isEmpty();
}

QString EmbedBridge::resolveModel(const QString& providerId) {
    const QString prov = providerId.isEmpty() ? resolveProvider() : providerId;
    const TaskRouter::Prefs p = TaskRouter::prefs();
    if (p.embedProvider == prov && !p.embedModel.isEmpty()) return p.embedModel;
    const QString saved = ProviderPrefs::modelFor(prov);
    if (!saved.isEmpty()) return saved;
    return TaskRouter::defaultEmbedModel(prov);
}

int EmbedBridge::batchSize(const ProviderSpec& spec) {
    // Gemini batchEmbedContents çok girdi kabul eder; OpenAI-uyumlu 2048'ye kadar
    if (spec.kind == ProviderKind::Gemini) return 100;
    if (spec.id == "ollama") return 16; // yerel bellek sınırı
    return 64;
}

void EmbedBridge::setProvider(const QString& providerId) {
    const QString p = providerId.trimmed();
    if (p.isEmpty() || p == m_provider) return;
    m_provider = p;
    m_model = resolveModel(p);
}

void EmbedBridge::setModel(const QString& model) {
    const QString m = model.trimmed();
    if (!m.isEmpty()) m_model = m;
}

EmbedBridge::Result EmbedBridge::embedOne(const QString& text, int timeoutMs) {
    return embed({text}, timeoutMs);
}

EmbedBridge::Result EmbedBridge::embed(const QStringList& texts, int timeoutMs) {
    Result r;
    r.providerId = m_provider;
    r.model = m_model;
    if (texts.isEmpty()) return r;

    const ProviderSpec spec = ProviderPrefs::resolve(m_provider);
    if (!spec.supportsEmbed) {
        r.error = QString("%1 gömme desteklemiyor.").arg(spec.label);
        return r;
    }
    if (m_model.isEmpty()) {
        r.error = "Gömme modeli seçilmemiş.";
        return r;
    }
    // 1) Önbellekten doldur
    QList<QList<float>> out;
    out.resize(texts.size());
    QList<int> missingIdx;
    QSet<int> missSet;
    for (int i = 0; i < texts.size(); ++i) {
        const QList<float> v = m_cache.get(m_model, texts.at(i));
        if (v.isEmpty()) {
            missingIdx << i;
            missSet.insert(i);
        } else {
            out[i] = v;
            ++r.cached;
        }
    }
    // 2) Eksikleri toplu iste
    if (!missingIdx.isEmpty()) {
        LlmClient* c = &client();
        c->setProvider(spec);
        if (!c->secretStore()) c->setSecretStore(m_secrets);
        c->loadKeyForProvider();
        const int bs = batchSize(spec);
        for (int start = 0; start < missingIdx.size(); start += bs) {
            const int end = qMin(missingIdx.size(), start + bs);
            QStringList batch;
            batch.reserve(end - start);
            for (int k = start; k < end; ++k) batch << texts.at(missingIdx.at(k));
            QString err;
            const auto vecs = c->embedSync(m_model, batch, err, timeoutMs);
            if (err.isEmpty() && !vecs.isEmpty()) {
                for (int k = 0; k < vecs.size(); ++k) {
                    const int idx = missingIdx.at(start + k);
                    out[idx] = vecs.at(k);
                    m_cache.put(m_model, texts.at(idx), vecs.at(k));
                    ++r.embedded;
                }
            } else if (!err.isEmpty() && r.error.isEmpty()) {
                r.error = err;
            }
        }
    }
    r.vectors = out;
    return r;
}
