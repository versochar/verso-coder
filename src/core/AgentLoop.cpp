#include "AgentLoop.h"
#include "AgentMemory.h"
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>

namespace {

// write_file öncesi mevcut içeriği oku (geri alma için).
QString readIfExists(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    return QString::fromUtf8(f.read(4 * 1024 * 1024));
}

} // namespace

AgentLoop::Result AgentLoop::run(AgentTools& tools, const QString& systemPrompt,
                                 const QString& userTask, int maxSteps, const Llm& llm,
                                 const Approver& approve, const Progress& progress,
                                 AgentRunContext* ctx) {
    Result res;
    if (maxSteps <= 0) maxSteps = 1;

    // Stage 34: politika araçlara uygulanır, bütçe başlatılır
    AgentBudget budget;
    if (ctx) {
        tools.setPolicy(ctx->policy);
        budget = ctx->budget;
        if (budget.maxSteps > 0) maxSteps = qMin(maxSteps, budget.maxSteps);
        else budget.maxSteps = maxSteps;
    } else {
        budget.maxSteps = maxSteps;
    }
    const qint64 t0 = QDateTime::currentMSecsSinceEpoch();
    budget.reset(t0);

    QString system = systemPrompt + tools.systemPromptAddendum();
    auto emitProgress = [&](const QString& s) { if (progress) progress(s); };

    QString conversation = "[GÖREV]\n" + userTask + "\n\n";
    if (ctx && !ctx->skillChainPrompt.isEmpty())
        conversation += ctx->skillChainPrompt + "\n";
    if (ctx && ctx->memory) {
        const QString recall =
            AgentMemory::formatRecall(AgentMemory::recall(ctx->memory->notes(), userTask), userTask);
        conversation += recall + "\n";
    }

    for (int step = 0; step < maxSteps; ++step) {
        // Bütçe denetimi
        QString why;
        if (budget.exceeded(QDateTime::currentMSecsSinceEpoch(), &why)) {
            res.budgetStopped = true;
            res.budgetReason = why;
            emitProgress("[bütçe] " + why);
            break;
        }
        budget.addStep();

        emitProgress(QString("[adım %1/%2] modelden yanıt bekleniyor...").arg(step + 1).arg(maxSteps));
        QString err;
        QString reply = llm(system, conversation, err);
        if (!err.isEmpty()) {
            res.error = err;
            emitProgress("[hata] " + err);
            break;
        }
        AgentStep st;
        st.assistant = reply;
        st.calls = AgentTools::parseCalls(reply);
        emitProgress(QString("[adım %1] yanıt alındı (%2 araç çağrısı)").arg(step + 1).arg(st.calls.size()));

        if (st.calls.isEmpty()) {
            st.reflection = AgentReflection::reflect(reply, QStringList(), step + 1, maxSteps);
            res.reflections << st.reflection;
            res.steps << st;
            res.finalText = reply;
            res.ok = true;
            emitProgress("[bitti] araç çağrısı yok, nihai yanıt üretildi.");
            break;
        }

        for (const ToolCall& c : st.calls) {
            // Araç kotası
            if (!budget.toolAllowed()) {
                st.observations << QString("[%1] araç kotası doldu, çağrı yapılmadı").arg(c.name);
                st.callResults.append({c.name, false});
                res.budgetStopped = true;
                res.budgetReason = QString("araç çağrısı kotası doldu (%1)").arg(budget.maxToolCalls);
                break;
            }
            res.toolCalls++;
            budget.addToolCall();
            emitProgress(QString("→ araç: %1(%2)").arg(c.name, QString::fromUtf8(
                QJsonDocument(c.args).toJson(QJsonDocument::Compact)).left(200)));

            // Geri alma için write_file öncesi içerik
            QString before;
            bool existed = true;
            const bool isWrite = (c.name == "write_file");
            if (isWrite) {
                const QString abs = tools.absoluteInRoot(c.args.value("path").toString());
                existed = QFile::exists(abs);
                before = readIfExists(abs);
            }

            // Onay: AgentLoop her çağrıda sorar. Araç kendi içinde de ikinci kez
            // sorabilir (m_approver bağlıysa); AiPanel yalnız AgentLoop onayını kullanır.
            bool approvedOk = true;
            if (approve && !approve(c)) approvedOk = false;
            if (!approvedOk) {
                st.observations << QString("[%1] kullanıcı reddetti").arg(c.name);
                st.callResults.append({c.name, false});
                emitProgress(QString("✗ %1 reddedildi").arg(c.name));
                continue;
            }
            ToolResult tr = tools.execute(c);
            st.observations << QString("[%1] %2").arg(c.name, tr.output);
            st.callResults.append({c.name, tr.ok});
            if (isWrite && tr.ok) {
                budget.addWrite();
                RunFile rf;
                rf.path = tools.absoluteInRoot(c.args.value("path").toString());
                rf.before = before;
                rf.created = !existed;
                res.changedFiles.append(rf);
            }
            emitProgress(QString("%1 %2").arg(tr.denied ? "✗" : (tr.ok ? "✓" : "⚠"), c.name));
        }

        // Stage 34: refleksiyon
        st.reflection = AgentReflection::reflect(reply, st.observations, step + 1, maxSteps);
        res.reflections << st.reflection;
        res.finalScore = st.reflection.score;
        res.callResults += st.callResults;
        emitProgress(QString("[refleksiyon] %1 (%2/100)").arg(
            StepReflection::verdictLabel(st.reflection.verdict)).arg(st.reflection.score));

        res.steps << st;

        conversation += "[ASİSTAN]\n" + reply + "\n[GÖZLEMLER]\n";
        for (const QString& o : st.observations)
            conversation += o.left(6000) + "\n";
        conversation += "\nGörevi bitir. Araç gerekmiyorsa nihai yanıtı normal metin olarak ver.\n";
        // Araç başarı istatistiğinden ceza satırı
        const QString penalty = AgentReflection::penaltyLine(AgentReflection::stats(res.callResults));
        if (!penalty.isEmpty()) conversation += penalty + "\n";
        if (st.reflection.hadError) conversation += "[NOT] " + st.reflection.advice + "\n";
        if (conversation.size() > 60000) conversation = conversation.right(60000);

        // Belleğe ders yaz
        if (ctx && ctx->memory && AgentReflection::shouldRemember(st.reflection))
            ctx->memory->add(AgentReflection::rememberText(ctx->goal.isEmpty() ? userTask : ctx->goal,
                                                           st.reflection),
                             st.reflection.wasDenied ? "failure" : "fact");
    }

    budget.finish(QDateTime::currentMSecsSinceEpoch());
    res.budget = budget;
    if (res.finalText.isEmpty() && !res.steps.isEmpty())
        res.finalText = res.steps.last().assistant;
    if (res.error.isEmpty() && !res.budgetStopped && !res.maxStepsReached && !res.ok &&
        !res.steps.isEmpty())
        res.error = "Ajan hedefe ulaşamadı.";
    if (res.maxStepsReached) res.error = "Maksimum adım sayısına ulaşıldı.";
    return res;
}

QString AgentLoop::formatTranscript(const QList<AgentStep>& steps, int maxChars) {
    QString out;
    for (int i = 0; i < steps.size(); ++i) {
        out += QString("\n— Adım %1 —\n").arg(i + 1);
        if (!steps[i].calls.isEmpty()) {
            for (const ToolCall& c : steps[i].calls) out += QString("• %1\n").arg(c.name);
        }
        for (const QString& o : steps[i].observations) out += "  " + o.left(400) + "\n";
    }
    if (out.size() > maxChars) out = out.left(maxChars) + "\n… (kısaltıldı)";
    return out;
}

QString AgentLoop::summarizeRun(const Result& r) {
    QStringList good, bad;
    for (const StepReflection& s : r.reflections) {
        if (s.verdict == "good") good << QString("adım %1").arg(s.step);
        else bad << QString("adım %1: %2").arg(s.step).arg(StepReflection::verdictLabel(s.verdict));
    }
    QString out = QString("Toplam %1 adım · %2 araç · %3 token · %4 sn")
                      .arg(r.steps.size())
                      .arg(r.toolCalls)
                      .arg(r.budget.tokens)
                      .arg(r.budget.elapsedMs() / 1000);
    if (!good.isEmpty()) out += "\nİyi: " + good.join(", ");
    if (!bad.isEmpty()) out += "\nSorunlu: " + bad.join(", ");
    if (r.budgetStopped) out += "\nBütçe: " + r.budgetReason;
    const QList<AgentReflection::ToolStat> st = AgentReflection::stats(r.callResults);
    if (!st.isEmpty()) {
        QStringList rates;
        for (const auto& s : st) rates << QString("%1 %2%%").arg(s.name).arg(int(s.rate() * 100));
        out += "\nAraç başarısı: " + rates.join(", ");
    }
    return out;
}
