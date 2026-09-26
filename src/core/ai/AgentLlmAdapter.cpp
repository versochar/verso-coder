#include "AgentLlmAdapter.h"
#include "ProviderHealth.h"
#include "ProviderPrefs.h"
#include "ProviderPricing.h"
#include "SecretStore.h"
#include "TaskRouter.h"
#include "UsageLedger.h"
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QSettings>

AgentLlmAdapter::AgentLlmAdapter(QObject* parent) : QObject(parent) {}

void AgentLlmAdapter::resolve() {
    if (m_opt.providerId.isEmpty()) {
        const TaskRouter::Prefs p = TaskRouter::prefs();
        m_opt.providerId = p.forceProvider.isEmpty() ? ProviderPrefs::activeProvider()
                                                      : p.forceProvider;
    }
    if (m_opt.model.isEmpty())
        m_opt.model = ProviderPrefs::modelFor(
            m_opt.providerId, ProviderRegistry::sampleModels(m_opt.providerId).value(0));
    m_client.setProvider(ProviderPrefs::resolve(m_opt.providerId));
    m_client.loadKeyForProvider();
}

ProviderSpec AgentLlmAdapter::spec() const { return ProviderPrefs::resolve(m_opt.providerId); }

QString AgentLlmAdapter::nativeToText(const AiReply& rep) const {
    if (rep.toolCalls.isEmpty()) return rep.text;
    QString out = rep.text;
    for (const AiToolCall& c : rep.toolCalls) {
        out += QStringLiteral("\n<tool_call>\n") +
               QString::fromUtf8(
                   QJsonDocument(QJsonObject{{QStringLiteral("name"), c.name},
                                             {QStringLiteral("arguments"), c.args}})
                       .toJson(QJsonDocument::Compact)) +
               QStringLiteral("\n</tool_call>");
    }
    return out;
}

AgentLlmAdapter::Turn AgentLlmAdapter::ask(const QString& systemPrompt, const QString& userPrompt,
                                          const QJsonArray& toolSchemas) {
    Turn t;
    if (m_opt.providerId.isEmpty()) resolve();
    t.providerId = m_opt.providerId;
    t.model = m_opt.model;
    m_lastMeta = StepMeta();

    const ProviderSpec s = spec();
    // Kota denetimi: aşıldıysa ağa hiç gidilmez
    QString quotaWhy;
    if (UsageLedger::instance().quotaExceeded(s.id, quotaWhy)) {
        t.error = quotaWhy;
        return t;
    }
    if (s.requiresKey() && m_client.apiKey().isEmpty()) {
        t.error = QString("%1 için API anahtarı eksik (Ayarlar → AI Sağlayıcıları).")
                      .arg(s.label);
        return t;
    }

    AiChatRequest req;
    req.model = m_opt.model;
    req.systemPrompt = systemPrompt;
    req.messages << AiMessage::user(userPrompt);
    if (m_opt.temperature >= 0.0) req.temperature = m_opt.temperature;
    req.maxTokens = m_opt.maxTokens;
    const bool native = m_opt.useNativeTools && s.supportsTools && !toolSchemas.isEmpty();
    if (native) {
        req.wantTools = true;
        req.tools = AiToolBridge::fromOpenAiSchemas(toolSchemas);
    }

    QElapsedTimer timer;
    timer.start();
    const AiReply rep = m_client.chatSync(req);
    const int ms = int(timer.elapsed());

    t.usage = rep.usage;
    t.httpStatus = rep.httpStatus;
    t.usedNative = !rep.toolCalls.isEmpty();
    t.nativeCallCount = rep.toolCalls.size();
    t.reasoning = rep.reasoning;

    if (m_opt.recordHealth) {
        if (rep.ok)
            ProviderHealth::instance().recordSuccess(s.id, ms);
        else
            ProviderHealth::instance().recordFailure(s.id, ms, rep.httpStatus);
    }
    if (!rep.ok) {
        t.error = rep.error;
        return t;
    }
    if (m_opt.recordUsage)
        UsageLedger::instance().record(s.id, m_opt.model, rep.usage.promptTokens,
                                       rep.usage.evalTokens);
    t.ok = true;
    t.text = nativeToText(rep);
    m_lastMeta.providerId = t.providerId;
    m_lastMeta.model = t.model;
    m_lastMeta.promptTokens = rep.usage.promptTokens;
    m_lastMeta.evalTokens = rep.usage.evalTokens;
    m_lastMeta.nativeTools = t.usedNative;
    m_lastMeta.httpStatus = t.httpStatus;
    return t;
}

std::function<QString(const QString&, const QString&, QString&)> AgentLlmAdapter::toFn(
    const QJsonArray& toolSchemas) {
    return [this, toolSchemas](const QString& sys, const QString& user, QString& err) -> QString {
        const Turn t = ask(sys, user, toolSchemas);
        if (!t.ok) {
            err = t.error;
            return {};
        }
        return t.text;
    };
}

double AgentLlmAdapter::estimateUsd(const QString& providerId, const QString& model, int steps,
                                    int tokensPerStep) {
    if (steps <= 0) return 0.0;
    // Bir adımda: istem ~2/3, yanıt ~1/3 varsayımı
    const int p = int(tokensPerStep * 0.66);
    const int e = int(tokensPerStep * 0.34);
    return ProviderPricing::estimateUsd(ProviderRegistry::byId(providerId), model, p * steps,
                                        e * steps) ;
}

QString AgentLlmAdapter::costPreview(const QString& providerId, const QString& model, int steps) {
    const ProviderSpec s = ProviderRegistry::byId(providerId);
    const double usd = estimateUsd(providerId, model, steps);
    if (usd <= 0.0) {
        if (ProviderPricing::isFreeTier(s, model))
            return QString("≈ ücretsiz (%1 · %2 adım)").arg(s.label).arg(steps);
        return QString("≈ ücretsiz");
    }
    return QString("≈ $%1 (%2 · %3 adım)").arg(usd, 0, 'f', 4).arg(s.label).arg(steps);
}
