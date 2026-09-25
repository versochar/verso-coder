#include "TokenStats.h"

double TokenStats::priceIn(const QString& model) {
    const QString m = model.toLower();
    if (m.contains("gpt-4o-mini")) return 0.15;
    if (m.contains("gpt-4o")) return 2.5;
    if (m.contains("gpt-4-turbo")) return 10.0;
    if (m.contains("claude-3-5-sonnet") || m.contains("claude-3.5-sonnet")) return 3.0;
    if (m.contains("claude-3-opus")) return 15.0;
    if (m.contains("gemini-1.5-pro")) return 1.25;
    if (m.contains("deepseek")) return 0.14;
    return 0.0; // yerel / bilinmeyen
}

double TokenStats::priceOut(const QString& model) {
    const QString m = model.toLower();
    if (m.contains("gpt-4o-mini")) return 0.6;
    if (m.contains("gpt-4o")) return 10.0;
    if (m.contains("gpt-4-turbo")) return 30.0;
    if (m.contains("claude-3-5-sonnet") || m.contains("claude-3.5-sonnet")) return 15.0;
    if (m.contains("claude-3-opus")) return 75.0;
    if (m.contains("gemini-1.5-pro")) return 5.0;
    if (m.contains("deepseek")) return 0.28;
    return 0.0;
}

double TokenStats::estimateCostUsd(const QString& model) const {
    return (promptTokens / 1'000'000.0) * priceIn(model) +
           (evalTokens / 1'000'000.0) * priceOut(model);
}

QString TokenStats::summary(const QString& model) const {
    QString s = QString("token: %1 giriş + %2 çıkış = %3 (%4 çağrı)")
                    .arg(promptTokens).arg(evalTokens).arg(total()).arg(calls);
    double cost = estimateCostUsd(model);
    if (cost > 0.0) s += QString(" · ~$%1").arg(cost, 0, 'f', 4);
    return s;
}
