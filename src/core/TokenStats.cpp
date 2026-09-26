#include "TokenStats.h"
#include "ai/ProviderPricing.h"
#include "ai/LlmProvider.h"
#include <QSettings>

void TokenStats::add(int prompt, int eval, int callCount) {
    promptTokens += qMax(0, prompt);
    evalTokens += qMax(0, eval);
    calls += qMax(0, callCount);
}

void TokenStats::addWithCache(int prompt, int eval, int cached, int callCount) {
    const qint64 p = qMax(0, static_cast<qint64>(prompt));
    promptTokens += p;
    evalTokens += qMax(0, eval);
    // Önbellekten okunan miktar bu çağrının prompt'unu aşamaz
    cachedTokens += qBound<qint64>(0, static_cast<qint64>(cached), p);
    calls += qMax(0, callCount);
}

double TokenStats::priceIn(const QString& model) {
    return ProviderPricing::priceFor(QString(), model).in;
}

double TokenStats::priceOut(const QString& model) {
    return ProviderPricing::priceFor(QString(), model).out;
}

double TokenStats::estimateCostUsd(const QString& providerId, const QString& model) const {
    const ProviderSpec spec = ProviderRegistry::byId(providerId);
    return ProviderPricing::estimateUsd(spec, model, int(promptTokens), int(evalTokens),
                                        int(cachedTokens));
}

double TokenStats::estimateCostUsd(const QString& model) const {
    return estimateCostUsd(QString(), model);
}

QString TokenStats::summary(const QString& providerId, const QString& model) const {
    QString s = QString("token: %1 giriş + %2 çıkış = %3 (%4 çağrı)")
                    .arg(promptTokens)
                    .arg(evalTokens)
                    .arg(total())
                    .arg(calls);
    if (cachedTokens > 0) s += QString(" · %1 önbellek").arg(cachedTokens);
    const double cost = estimateCostUsd(providerId, model);
    if (cost > 0.0) s += QString(" · ~$%1").arg(cost, 0, 'f', 4);
    else if (!ProviderRegistry::byId(providerId).id.isEmpty() &&
             ProviderPricing::isFreeTier(ProviderRegistry::byId(providerId), model))
        s += QString(" · ücretsiz");
    return s;
}

QString TokenStats::summary(const QString& model) const { return summary(QString(), model); }

double TokenStats::costGuardUsd() {
    QSettings s;
    return s.value(QStringLiteral("ai/costGuardUsd"), 0.0).toDouble();
}

void TokenStats::setCostGuardUsd(double usd) {
    QSettings s;
    s.setValue(QStringLiteral("ai/costGuardUsd"), qMax(0.0, usd));
}

bool TokenStats::exceedsGuard(double limitUsd, const QString& providerId,
                              const QString& model) const {
    const double limit = limitUsd < 0.0 ? costGuardUsd() : limitUsd;
    if (limit <= 0.0) return false;
    return estimateCostUsd(providerId, model) > limit;
}

bool TokenStats::needsConfirmation(double estimatedUsd, double limitUsd) {
    const double limit = limitUsd < 0.0 ? costGuardUsd() : limitUsd;
    if (limit <= 0.0) return false;
    return estimatedUsd > limit;
}

QString TokenStats::confirmationText(double estimatedUsd, const QString& model) {
    return QString("Bu koşu ≈ $%1 tutabilir (tavan: $%2).\nDevam edilsin mi?")
        .arg(estimatedUsd, 0, 'f', 4)
        .arg(costGuardUsd(), 0, 'f', 2);
}
