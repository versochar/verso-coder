#pragma once
#include "AgentTools.h"
#include <QStringList>
#include <functional>

// Tek adımın kaydı: asistan yanıtı + yapılan çağrılar + gözlemler.
struct AgentStep {
    QString assistant;
    QList<ToolCall> calls;
    QStringList observations;
};

// Çok adımlı ajan döngüsü: plan → araç çağrıları → gözlem → devam.
// LLM ve onay geri çağrıları dışarıdan enjekte edilir (ağsız test edilebilir).
class AgentLoop {
public:
    using Llm = std::function<QString(const QString& systemPrompt, const QString& userPrompt,
                                      QString& error)>;
    using Approver = std::function<bool(const ToolCall&)>;
    using Progress = std::function<void(const QString&)>;

    struct Result {
        bool ok = false;
        bool maxStepsReached = false;
        QString finalText;
        QString error;
        QList<AgentStep> steps;
        int toolCalls = 0;
    };

    static Result run(AgentTools& tools, const QString& systemPrompt, const QString& userTask,
                      int maxSteps, const Llm& llm, const Approver& approve = {},
                      const Progress& progress = {});

    static QString formatTranscript(const QList<AgentStep>& steps, int maxChars = 16000);
};
