#include "AgentLoop.h"
#include <QJsonDocument>

AgentLoop::Result AgentLoop::run(AgentTools& tools, const QString& systemPrompt,
                                 const QString& userTask, int maxSteps, const Llm& llm,
                                 const Approver& approve, const Progress& progress) {
    Result res;
    if (maxSteps <= 0) maxSteps = 1;

    const QString system = systemPrompt + tools.systemPromptAddendum();
    auto emitProgress = [&](const QString& s) { if (progress) progress(s); };

    QString conversation = "[GÖREV]\n" + userTask + "\n\n";
    for (int step = 0; step < maxSteps; ++step) {
        emitProgress(QString("[adım %1/%2] modelden yanıt bekleniyor...").arg(step + 1).arg(maxSteps));
        QString err;
        QString reply = llm(system, conversation, err);
        if (!err.isEmpty()) {
            res.error = err;
            emitProgress("[hata] " + err);
            return res;
        }
        AgentStep st;
        st.assistant = reply;
        st.calls = AgentTools::parseCalls(reply);
        emitProgress(QString("[adım %1] yanıt alındı (%2 araç çağrısı)").arg(step + 1).arg(st.calls.size()));

        if (st.calls.isEmpty()) {
            res.steps << st;
            res.finalText = reply;
            res.ok = true;
            emitProgress("[bitti] araç çağrısı yok, nihai yanıt üretildi.");
            return res;
        }

        for (const ToolCall& c : st.calls) {
            res.toolCalls++;
            emitProgress(QString("→ araç: %1(%2)").arg(c.name, QString::fromUtf8(
                QJsonDocument(c.args).toJson(QJsonDocument::Compact)).left(200)));
            if (approve && !approve(c)) {
                st.observations << QString("[%1] kullanıcı reddetti").arg(c.name);
                emitProgress(QString("✗ %1 reddedildi").arg(c.name));
                continue;
            }
            ToolResult tr = tools.execute(c);
            st.observations << QString("[%1] %2").arg(c.name, tr.output);
            emitProgress(QString("%1 %2").arg(tr.denied ? "✗" : (tr.ok ? "✓" : "⚠"), c.name));
        }
        res.steps << st;

        conversation += "[ASİSTAN]\n" + reply + "\n[GÖZLEMLER]\n";
        for (const QString& o : st.observations)
            conversation += o.left(6000) + "\n";
        conversation += "\nGörevi bitir. Araç gerekmiyorsa nihai yanıtı normal metin olarak ver.\n";
        if (conversation.size() > 60000)
            conversation = conversation.right(60000);
    }

    res.maxStepsReached = true;
    res.error = "Maksimum adım sayısına ulaşıldı.";
    if (!res.steps.isEmpty()) res.finalText = res.steps.last().assistant;
    emitProgress("[uyarı] maksimum adım sayısına ulaşıldı.");
    return res;
}

QString AgentLoop::formatTranscript(const QList<AgentStep>& steps, int maxChars) {
    QString out;
    for (int i = 0; i < steps.size(); ++i) {
        out += QString("\n— Adım %1 —\n").arg(i + 1);
        if (!steps[i].calls.isEmpty()) {
            for (const ToolCall& c : steps[i].calls)
                out += QString("• %1\n").arg(c.name);
        }
        for (const QString& o : steps[i].observations)
            out += "  " + o.left(400) + "\n";
    }
    if (out.size() > maxChars) out = out.left(maxChars) + "\n… (kısaltıldı)";
    return out;
}
