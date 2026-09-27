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
    QList<RunFile> files; // Stage 45: bu adımda değişen dosyalar (adım adım geri alma)
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
        int injectionHits = 0; // Stage 45: işaretlenen enjeksiyon bulgusu
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
    // --- Stage 45: dolaylı istem enjeksiyonu koruması ---
    // Araç ÇIKTILARI (dosya/komut/arama sonucu) LLM bağlamına dönmeden taranır.
    // Saldırgan dosya içine "önceki talimatları yoksay" yazabilir; model bunu
    // veri değil talimat sanır. Bulgu: metin işaretlenir + sayılır.
    // Varsayılan ENGELLEMEZ (kullanıcı enjeksiyonu tartışıyor olabilir);
    // politika eşiği aşınca durdurur.
    struct InjectionHit {
        QString pattern; // hangi kalıp yakaladı
        QString excerpt; // ilk 120 karakter
    };
    static QList<InjectionHit> scanObservation(const QString& text);
    static QString markObservation(const QString& text);
    // Adım dosyalarını geri al (önce içerik, sonra oluşturulanı sil)
    static bool revertFiles(const QList<RunFile>& files, QString* error = nullptr);
    // Tüm adımları doğru sırada geri al (son adım önce)
    static bool revertSteps(const QList<AgentStep>& steps, QString* error = nullptr);
    // Stage 34: koşu sonrası tek parça özet (günlüğe yazılır).
    static QString summarizeRun(const Result& r);
};
