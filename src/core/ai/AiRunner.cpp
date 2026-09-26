#include "AiRunner.h"
#include "../ModelCapabilities.h"
#include "LlmClient.h"
#include "ProviderHealth.h"
#include "ProviderPrefs.h"
#include "ProviderPricing.h"
#include "SecretStore.h"
#include "UsageLedger.h"
#include <QElapsedTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QSet>

namespace {
// Ortak kuyruk durumu (tüm AiRunner örnekleri paylaşır)
QSet<AiRunner*> g_active;
QMutex g_activeMutex;
} // namespace

AiRunner::AiRunner(QObject* parent) : QObject(parent) {
    // Çağıran kasa bağlamayı unutsa bile anahtar bulunabilsin
    if (!m_secrets) m_secrets = new SecretStore();
}

AiRunner::~AiRunner() { leave(); }

bool AiRunner::busy() {
    QMutexLocker lk(&g_activeMutex);
    return !g_active.isEmpty();
}

int AiRunner::activeCount() {
    QMutexLocker lk(&g_activeMutex);
    return int(g_active.size());
}

void AiRunner::cancelAll() {
    QList<AiRunner*> copy;
    {
        QMutexLocker lk(&g_activeMutex);
        copy = g_active.values();
    }
    for (AiRunner* r : copy) r->cancel();
}

void AiRunner::cancel() {
    for (LlmClient* c : m_pool) c->cancel();
}

void AiRunner::enter() {
    if (m_inside) return;
    m_inside = true;
    {
        QMutexLocker lk(&g_activeMutex);
        g_active.insert(this);
    }
    emit busyChanged(true);
}

void AiRunner::leave() {
    if (!m_inside) return;
    m_inside = false;
    bool empty = false;
    {
        QMutexLocker lk(&g_activeMutex);
        g_active.remove(this);
        empty = g_active.isEmpty();
    }
    if (empty) emit busyChanged(false);
}

AiRunner::Options AiRunner::optionsFor(AiTask task, const QString& systemPrompt) {
    const AiProfiles::Profile p = AiProfiles::profile(task);
    Options o;
    o.task = task;
    o.systemPrompt = AiProfiles::withSuffix(systemPrompt, task);
    o.temperature = p.temperature;
    o.maxTokens = p.maxTokens;
    o.timeoutMs = p.timeoutMs;
    return o;
}

AiRunner::Plan AiRunner::planFor(const Options& opts) {
    Plan p;
    const AiProfiles::Profile prof = AiProfiles::profile(opts.task);

    if (opts.bypassRouting || !opts.providerId.isEmpty() || !opts.model.isEmpty()) {
        // Verilen sağlayıcı/model öncelikli (yönlendirme atlanır)
        const QString prov =
            opts.providerId.isEmpty() ? ProviderPrefs::activeProvider() : opts.providerId;
        p.providerId = prov;
        p.model = opts.model.isEmpty()
                      ? ProviderPrefs::modelFor(prov, ProviderRegistry::sampleModels(prov).value(0))
                      : opts.model;
        p.cls = prof.hint;
        p.reason = opts.providerId.isEmpty() ? QStringLiteral("varsayılan sağlayıcı")
                                             : QStringLiteral("verilen sağlayıcı");
        p.forceLocked = !opts.providerId.isEmpty();
    } else {
        // Görev ipucu yönlendirmeyi besler
        TaskRouter::Prefs tp = TaskRouter::prefs();
        if (!tp.enabled) tp.enabled = true; // AiRunner her zaman yönlendirir
        TaskRouter::Route r = TaskRouter::route(prof.hint == TaskClass::Vision
                                                    ? QStringLiteral("görsel")
                                                    : (prof.hint == TaskClass::Complex
                                                           ? QStringLiteral("mimari refactor test")
                                                           : QStringLiteral("kısa açıklama")),
                                                tp);
        p.providerId = r.providerId;
        p.model = opts.model.isEmpty() ? r.model : opts.model;
        p.cls = r.cls;
        p.reason = r.reason;
        p.forceLocked = r.forceLocked;
    }
    if (p.providerId.isEmpty()) p.providerId = ProviderPrefs::activeProvider();
    if (p.model.isEmpty())
        p.model = ProviderPrefs::modelFor(p.providerId,
                                          ProviderRegistry::sampleModels(p.providerId).value(0));
    const ProviderSpec spec = ProviderRegistry::byId(p.providerId);
    p.ok = !p.providerId.isEmpty() && !p.model.isEmpty() && !spec.id.isEmpty();
    return p;
}

AiRunner::Plan AiRunner::resolve(const Options& opts) { return planFor(opts); }

QList<AiImage> AiRunner::imagesFromBase64(const QStringList& b64, const QString& mime) {
    QList<AiImage> out;
    for (const QString& raw : cleanBase64(b64)) {
        AiImage im;
        im.mime = mime;
        im.bytes = QByteArray::fromBase64(raw.toLatin1());
        if (!im.bytes.isEmpty()) out << im;
    }
    return out;
}

QStringList AiRunner::cleanBase64(const QStringList& in) {
    QStringList out;
    for (const QString& s : in) {
        QString v = s.trimmed();
        const int comma = v.indexOf(QLatin1Char(','));
        if (v.startsWith(QLatin1String("data:")) && comma > 0) v = v.mid(comma + 1);
        v.remove(QLatin1Char(' '));
        v.remove(QLatin1Char('\n'));
        v.remove(QLatin1Char('\r'));
        if (!v.isEmpty()) out << v;
    }
    return out;
}

double AiRunner::estimateUsd(const Plan& plan, int promptChars, int outputTokens) {
    const int promptTokens = qMax(1, promptChars / 4);
    return ProviderPricing::estimateUsd(ProviderRegistry::byId(plan.providerId), plan.model,
                                        promptTokens, qMax(0, outputTokens));
}

LlmClient* AiRunner::clientFor(const QString& providerId) {
    LlmClient*& c = m_pool[providerId];
    if (!c) {
        c = new LlmClient(this);
        if (m_secrets) c->setSecretStore(m_secrets);
    }
    return c;
}

AiRunner::Result AiRunner::run(const Options& opts, const QString& userPrompt,
                              const QList<AiImage>& images) {
    Result res;
    const Plan plan = planFor(opts);
    res.providerId = plan.providerId;
    res.model = plan.model;
    if (!plan.ok) {
        res.error = QStringLiteral("Uygun sağlayıcı/model bulunamadı.");
        return res;
    }
    // Kota: aşıldıysa ağa hiç gidilmez
    {
        QString why;
        if (UsageLedger::instance().quotaExceeded(plan.providerId, why)) {
            res.error = why;
            return res;
        }
    }
    ProviderSpec spec = ProviderPrefs::resolve(plan.providerId);
    // Görsel isteniyorsa yetenek yoksa uyar ama devam et (sessizce ekleri atmak kötü)
    if (!images.isEmpty() && !spec.supportsVision) {
        res.error = QStringLiteral("%1 görsel desteklemiyor.").arg(spec.label);
        return res;
    }

    LlmClient* c = clientFor(plan.providerId);
    c->setProvider(spec);
    c->setFailoverEnabled(opts.allowFailover);
    if (!c->secretStore()) c->setSecretStore(m_secrets);
    c->loadKeyForProvider();
    if (spec.requiresKey() && c->apiKey().isEmpty()) {
        res.error = QStringLiteral("%1 için API anahtarı gerekli.").arg(spec.label);
        return res;
    }

    AiChatRequest req;
    req.model = plan.model;
    req.systemPrompt = opts.systemPrompt;
    const AiProfiles::Profile prof = AiProfiles::profile(opts.task);
    req.temperature = opts.temperature >= 0.0 ? opts.temperature : -1.0;
    req.maxTokens = opts.maxTokens > 0 ? opts.maxTokens : prof.maxTokens;
    AiMessage m;
    m.role = AiRole::User;
    m.texts = QStringList{userPrompt};
    m.images = images;
    req.messages << m;

    enter();
    QElapsedTimer timer;
    timer.start();
    const AiReply rep = c->chatSync(req, opts.timeoutMs > 0 ? opts.timeoutMs : prof.timeoutMs);
    res.ms = timer.elapsed();

    if (opts.record && m_recordUsage)
        ProviderHealth::instance().record(plan.providerId, rep.ok, int(res.ms), rep.httpStatus);
    if (!rep.ok) {
        res.httpStatus = rep.httpStatus;
        res.error = rep.error;
        res.reason = plan.reason;
        if (c->lastFailoverFrom() == plan.providerId)
            res.reason = QStringLiteral("sağlayıcı değiştirildi, yine de başarısız");
        leave();
        emit finished(res);
        return res;
    }
    if (c->lastFailoverFrom() == plan.providerId) {
        // İstek eşdeğer bir sağlayıcıda tamamlandı: gerçek kimliği yaz
        res.providerId = c->provider().id;
        res.reason = QStringLiteral("otomatik failover");
    }
    if (res.reason.isEmpty()) res.reason = plan.reason;
    res.ok = true;
    res.text = rep.text;
    res.reasoning = rep.reasoning;
    res.toolCalls = rep.toolCalls;
    res.usage = rep.usage;
    res.httpStatus = rep.httpStatus;
    res.requestId = rep.requestId;
    res.usd = ProviderPricing::estimateUsd(ProviderRegistry::byId(res.providerId), res.model,
                                           rep.usage.promptTokens, rep.usage.evalTokens);
    if (opts.record && m_recordUsage)
        UsageLedger::instance().record(res.providerId, res.model, rep.usage.promptTokens,
                                       rep.usage.evalTokens);
    leave();
    emit finished(res);
    return res;
}

AiRunner::Result AiRunner::ask(AiTask task, const QString& userPrompt,
                              const QString& systemPrompt, const QList<AiImage>& images) {
    Options o = optionsFor(task, systemPrompt);
    return run(o, userPrompt, images);
}

// --- asenkron yol (arayüzü kilitlemez, iptal edilebilir) ---

bool AiRunner::cancelActive() {
    if (!m_active) return false;
    m_active->cancel();
    m_active = nullptr;
    return true;
}

void AiRunner::handleAsyncResult(const LlmClient* client, const AiReply& rep,
                                 const QElapsedTimer& timer, const Plan& plan) {
    Result res;
    res.providerId = client->provider().id;
    res.model = plan.model;
    res.ms = timer.elapsed();
    res.httpStatus = rep.httpStatus;
    res.requestId = rep.requestId;
    if (m_recordUsage)
        ProviderHealth::instance().record(res.providerId, rep.ok, int(res.ms), rep.httpStatus);
    if (rep.ok) {
        if (client->lastFailoverFrom() == plan.providerId) {
            res.providerId = client->provider().id;
            res.reason = QStringLiteral("otomatik failover");
        }
        if (res.reason.isEmpty()) res.reason = plan.reason;
        res.ok = true;
        res.text = rep.text;
        res.reasoning = rep.reasoning;
        res.toolCalls = rep.toolCalls;
        res.usage = rep.usage;
        res.usd = ProviderPricing::estimateUsd(ProviderRegistry::byId(res.providerId), res.model,
                                               rep.usage.promptTokens, rep.usage.evalTokens);
        if (m_recordUsage)
            UsageLedger::instance().record(res.providerId, res.model, rep.usage.promptTokens,
                                           rep.usage.evalTokens);
    } else {
        res.reason = plan.reason;
        res.error = rep.error;
    }
    if (m_active == client) m_active = nullptr;
    leave();
    emit finished(res);
}

void AiRunner::runAsync(const Options& opts, const QString& userPrompt,
                        const QList<AiImage>& images) {
    Result err;
    const Plan plan = planFor(opts);
    err.providerId = plan.providerId;
    err.model = plan.model;
    if (!plan.ok) {
        err.error = QStringLiteral("Uygun sağlayıcı/model bulunamadı.");
        emit finished(err);
        return;
    }
    {
        QString why;
        if (UsageLedger::instance().quotaExceeded(plan.providerId, why)) {
            err.error = why;
            emit finished(err);
            return;
        }
    }
    LlmClient* c = clientFor(plan.providerId);
    c->setProvider(ProviderPrefs::resolve(plan.providerId));
    c->setFailoverEnabled(opts.allowFailover);
    if (!c->secretStore()) c->setSecretStore(m_secrets);
    c->loadKeyForProvider();
    if (c->provider().requiresKey() && c->apiKey().isEmpty()) {
        err.error = QStringLiteral("%1 için API anahtarı gerekli.").arg(c->provider().label);
        emit finished(err);
        return;
    }
    if (!images.isEmpty() && !c->provider().supportsVision) {
        err.error = QStringLiteral("%1 görsel desteklemiyor.").arg(c->provider().label);
        emit finished(err);
        return;
    }

    const AiProfiles::Profile prof = AiProfiles::profile(opts.task);
    AiChatRequest req;
    req.model = plan.model;
    req.systemPrompt = opts.systemPrompt;
    req.temperature = opts.temperature >= 0.0 ? opts.temperature : -1.0;
    req.maxTokens = opts.maxTokens > 0 ? opts.maxTokens : prof.maxTokens;
    AiMessage m;
    m.role = AiRole::User;
    m.texts = QStringList{userPrompt};
    m.images = images;
    req.messages << m;

    enter();
    m_active = c;
    QElapsedTimer timer;
    timer.start();
    connect(c, &LlmClient::finished, this,
            [this, c, plan, timer](const AiReply& rep) {
                handleAsyncResult(c, rep, timer, plan);
            },
            Qt::SingleShotConnection);
    c->chat(req);
}

void AiRunner::fetchModels(std::function<void(const QStringList&)> done) {
    const QString prov = ProviderPrefs::activeProvider();
    LlmClient* c = clientFor(prov);
    c->setProvider(ProviderPrefs::resolve(prov));
    c->loadKeyForProvider();
    connect(c, &LlmClient::modelsReady, this, [done](const QStringList& ms) { if (done) done(ms); },
            Qt::SingleShotConnection);
    c->fetchModels();
}

bool AiRunner::canRunWithImages(const QString& providerId, const QString& model,
                                int imageCount) {
    if (imageCount <= 0) return true;
    const ProviderSpec spec = ProviderPrefs::resolve(providerId);
    if (!spec.supportsVision) return false;
    // Gömme modelleri görsel kabul etmez
    const ModelCapabilities caps = ModelCapabilities::fromSpec(spec, model);
    return !caps.embedding;
}

QString AiRunner::imageBlockReason(const QString& providerId, int imageCount) {
    if (canRunWithImages(providerId, QString(), imageCount)) return {};
    const ProviderSpec spec = ProviderPrefs::resolve(providerId);
    return QStringLiteral("%1 görsel desteklemiyor (%2 ek). Görsel eklerini kaldır "
                          "ya da görsel destekleyen bir model/sağlayıcı seç.")
        .arg(spec.label)
        .arg(imageCount);
}
