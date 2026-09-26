#pragma once
#include "AgentBudget.h"
#include "AgentReflection.h"
#include "AgentRunStore.h"
#include "AgentTools.h"
#include <QStringList>
#include <functional>

class AgentMemory;

// Stage 36: adım telemetrisi — hangi sağlayıcı/model kaç token harcadı.
struct StepMeta {
    QString providerId;
    QString model;
    qint64 promptTokens = 0;
    qint64 evalTokens = 0;
    bool nativeTools = false;
    int httpStatus = 0;
};

// Tek adımın kaydı: asistan yanıtı + yapılan çağrılar + gözlemler.
struct AgentStep {
    QString assistant;
    QList<ToolCall> calls;
    QStringList observations;
    // Stage 34: çağrı sonuçları ve öz-değerlendirme
    QList<QPair<QString, bool>> callResults;
    StepReflection reflection;
};

// Stage 34: koşu bağlamı — politika, bütçe, bellek ve beceri zinciri.
struct AgentRunContext {
    AgentPolicy policy;
    AgentBudget budget;
    AgentMemory* memory = nullptr;
    QString skillChainPrompt; // beceri zinciri özeti
    QString goal;            // bellek hatırlama için hedef
};

// Çok adımlı ajan döngüsü: plan → araç çağrıları → gözlem → devam.
// LLM ve onay geri çağrıları dışarıdan enjekte edilir (ağsız test edilebilir).
class AgentLoop {
public:
    using Llm = std::function<QString(const QString& systemPrompt, const QString& userPrompt,
                                      QString& error)>;
    // Stage 36: adım sonrası telemetri toplayıcı (sağlayıcı/model/token).
    using Meta = std::function<StepMeta(const QString& assistantText, const QList<ToolCall>&)>;
    using Approver = std::function<bool(const ToolCall&)>;
    using Progress = std::function<void(const QString&)>;

    struct Result {
        bool ok = false;
        bool maxStepsReached = false;
        QString finalText;
        QString error;
        QList<AgentStep> steps;
        int toolCalls = 0;
        // Stage 34
        AgentBudget budget;
        QList<StepReflection> reflections;
        QList<QPair<QString, bool>> callResults;
        QList<RunFile> changedFiles; // geri alınabilir dosya değişiklikleri
        QList<StepMeta> metas;       // Stage 36: adım telemetrisi
        bool budgetStopped = false;
        QString budgetReason;
        int finalScore = 0; // son refleksiyon puanı
    };

    static Result run(AgentTools& tools, const QString& systemPrompt, const QString& userTask,
                      int maxSteps, const Llm& llm, const Approver& approve = {},
                      const Progress& progress = {}, AgentRunContext* ctx = nullptr,
                      const Meta& meta = {});

    static QString formatTranscript(const QList<AgentStep>& steps, int maxChars = 16000);
    // Stage 34: koşu sonrası tek parça özet (günlüğe yazılır).
    static QString summarizeRun(const Result& r);
};
