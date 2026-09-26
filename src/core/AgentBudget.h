#pragma once
#include <QString>
#include <QtGlobal>

// Stage 34: ajan çalışma bütçesi. Saf mantık — sayaçlar ve kota denetimi.
// 0/-1 değerleri "sınırsız" anlamına gelir.
struct AgentBudget {
    // --- limitler ---
    int maxSteps = 5;
    int maxToolCalls = 20;
    int maxWrites = 3;    // dosya yazma
    int maxTokens = 0;    // 0 = sınırsız
    qint64 maxMs = 0;     // 0 = sınırsız

    // --- tüketim ---
    int steps = 0;
    int toolCalls = 0;
    int writes = 0;
    int tokens = 0;
    qint64 startedMs = 0;
    qint64 endedMs = 0;

    void reset(qint64 nowMs);
    void addStep() { ++steps; }
    void addToolCall() { ++toolCalls; }
    void addWrite() { ++writes; }
    void addTokens(int t) { if (t > 0) tokens += t; }
    void finish(qint64 nowMs) { endedMs = nowMs; }
    qint64 elapsedMs(qint64 nowMs = 0) const;

    // Kota denetimi
    bool stepAllowed() const;
    bool toolAllowed() const;
    bool writeAllowed() const;
    bool tokenExceeded() const;
    bool timeExceeded(qint64 nowMs) const;

    // İlk aşılan kotaları gerekçesiyle bildirir; aşılmadıysa false.
    bool exceeded(qint64 nowMs, QString* reason = nullptr) const;

    // En dolu kotaların yüzdesi (0..100) — ilerleme çubuğu.
    int usedPercent(qint64 nowMs = 0) const;
    QString summary(qint64 nowMs = 0) const;
};
