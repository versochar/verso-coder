#pragma once
#include "../AgentLoop.h"
#include "AiMessage.h"
#include "AiToolBridge.h"
#include "LlmClient.h"
#include "LlmProvider.h"
#include <QJsonArray>
#include <QObject>
#include <QString>
#include <functional>

// Stage 36: ajan döngüsü için sağlayıcıdan bağımsız LLM arayüzü.
// Sağlayıcının desteklediği native function-calling varsa kullanılır; yoksa
// mevcut `<tool_call>` metin protokolüne düşülür — AgentLoop hiç değişmeden
// çalışmaya devam eder. Sağlık, kullanım ve maliyet burada toplanır.
class AgentLlmAdapter : public QObject {
    Q_OBJECT
public:
    struct Options {
        QString providerId;
        QString model;
        bool stream = false;
        bool useNativeTools = true;
        bool recordHealth = true;
        bool recordUsage = true;
        double temperature = -1.0; // <0 → ayar kullanılır
        int maxTokens = 0;
    };

    struct Turn {
        bool ok = false;
        QString text;                // metin + araç çağrısı protokolü
        QString reasoning;           // düşünme metni
        bool usedNative = false;     // native araç çağrısı mı?
        int nativeCallCount = 0;
        QString error;
        AiUsage usage;
        int httpStatus = 0;
        QString providerId;
        QString model;
    };

    explicit AgentLlmAdapter(QObject* parent = nullptr);

    void setOptions(const Options& o) { m_opt = o; }
    Options options() const { return m_opt; }
    void setSecretStore(SecretStore* s) { m_secrets = s; m_client.setSecretStore(s); }

    // Sağlayıcı/Model bilgisi verilmezse TaskRouter + ProviderPrefs çözülür
    void resolve();
    ProviderSpec spec() const;

    // Tek tur: sistem + kullanıcı istemi → metin (araç çağrıları dahil)
    Turn ask(const QString& systemPrompt, const QString& userPrompt,
             const QJsonArray& toolSchemas = QJsonArray());
    // AgentLoop::Llm imzasına uyarlayıcı
    std::function<QString(const QString&, const QString&, QString&)> toFn(
        const QJsonArray& toolSchemas = QJsonArray());

    // Son tur telemetrisi (AgentLoop::Meta geri çağrısına bağlanır)
    StepMeta lastMeta() const { return m_lastMeta; }

    // Koşu maliyet tahmini (onay diyaloğu için)
    static double estimateUsd(const QString& providerId, const QString& model, int steps,
                              int tokensPerStep = 1200);
    static QString costPreview(const QString& providerId, const QString& model, int steps);

private:
    QString nativeToText(const AiReply& rep) const;

    LlmClient m_client;
    Options m_opt;
    StepMeta m_lastMeta;
    SecretStore* m_secrets = nullptr;
};
