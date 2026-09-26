#include "AgentBudget.h"

void AgentBudget::reset(qint64 nowMs) {
    steps = 0;
    toolCalls = 0;
    writes = 0;
    tokens = 0;
    startedMs = nowMs;
    endedMs = 0;
}

qint64 AgentBudget::elapsedMs(qint64 nowMs) const {
    const qint64 end = endedMs > 0 ? endedMs : (nowMs > 0 ? nowMs : startedMs);
    return end > startedMs ? end - startedMs : 0;
}

bool AgentBudget::stepAllowed() const { return maxSteps <= 0 || steps < maxSteps; }
bool AgentBudget::toolAllowed() const { return maxToolCalls <= 0 || toolCalls < maxToolCalls; }
bool AgentBudget::writeAllowed() const { return maxWrites <= 0 || writes < maxWrites; }
bool AgentBudget::tokenExceeded() const { return maxTokens > 0 && tokens >= maxTokens; }
bool AgentBudget::timeExceeded(qint64 nowMs) const {
    return maxMs > 0 && elapsedMs(nowMs) >= maxMs;
}

bool AgentBudget::exceeded(qint64 nowMs, QString* reason) const {
    if (!stepAllowed()) {
        if (reason) *reason = QString("adım kotası aşıldı (%1/%2)").arg(steps).arg(maxSteps);
        return true;
    }
    if (!toolAllowed()) {
        if (reason) *reason = QString("araç çağrısı kotası aşıldı (%1/%2)").arg(toolCalls).arg(maxToolCalls);
        return true;
    }
    if (!writeAllowed()) {
        if (reason) *reason = QString("yazma kotası aşıldı (%1/%2)").arg(writes).arg(maxWrites);
        return true;
    }
    if (tokenExceeded()) {
        if (reason) *reason = QString("token bütçesi aşıldı (%1/%2)").arg(tokens).arg(maxTokens);
        return true;
    }
    if (timeExceeded(nowMs)) {
        if (reason)
            *reason = QString("süre bütçesi aşıldı (%1 sn / %2 sn)")
                          .arg(elapsedMs(nowMs) / 1000)
                          .arg(maxMs / 1000);
        return true;
    }
    if (reason) reason->clear();
    return false;
}

int AgentBudget::usedPercent(qint64 nowMs) const {
    int worst = 0;
    auto ratio = [](qint64 used, qint64 limit) {
        if (limit <= 0) return 0;
        const int r = int((used * 100) / limit);
        return qBound(0, r, 100);
    };
    worst = qMax(worst, ratio(steps, maxSteps));
    worst = qMax(worst, ratio(toolCalls, maxToolCalls));
    worst = qMax(worst, ratio(writes, maxWrites));
    worst = qMax(worst, ratio(tokens, maxTokens));
    worst = qMax(worst, ratio(elapsedMs(nowMs), maxMs));
    return worst;
}

QString AgentBudget::summary(qint64 nowMs) const {
    return QString("adım %1/%2 · araç %3/%4 · yazma %5/%6 · token %7%8 · süre %9 sn")
        .arg(steps)
        .arg(maxSteps > 0 ? QString::number(maxSteps) : QString("∞"))
        .arg(toolCalls)
        .arg(maxToolCalls > 0 ? QString::number(maxToolCalls) : QString("∞"))
        .arg(writes)
        .arg(maxWrites > 0 ? QString::number(maxWrites) : QString("∞"))
        .arg(tokens)
        .arg(maxTokens > 0 ? QString("/%1").arg(maxTokens) : QString(""))
        .arg(elapsedMs(nowMs) / 1000);
}
