#include "ConversationSummarizer.h"
#include "../core/ContextBudget.h"

bool ConversationSummarizer::needed(const QList<ConvTurn>& turns,
                                    const QString& existingSummary, int maxTokens,
                                    int keepRecent) {
    if (maxTokens <= 0) return false;
    const QList<ConvTurn> older = olderTurns(turns, keepRecent);
    if (older.isEmpty()) return false;
    int est = ContextBudget::estimate(existingSummary);
    for (const ConvTurn& t : older) est += ContextBudget::estimate(t.text);
    return est > maxTokens;
}

QList<ConvTurn> ConversationSummarizer::olderTurns(const QList<ConvTurn>& turns, int keepRecent) {
    if (keepRecent <= 0 || turns.size() <= keepRecent) return {};
    return turns.mid(0, turns.size() - keepRecent);
}

QString ConversationSummarizer::summarizePrompt(const QList<ConvTurn>& turns, int maxChars) {
    QString convo;
    for (const ConvTurn& t : turns) {
        const QString line = QString("%1: %2\n").arg(t.role == "ai" ? "Asistan" : "Kullanıcı", t.text);
        if (convo.size() + line.size() > maxChars) break;
        convo += line;
    }
    return "Aşağıdaki konuşmayı, sonraki turlar için bağlam olacak şekilde kısa ve madde "
           "madde özetle. Önemli kararları, dosya adlarını ve açık soruları koru.\n\n" + convo;
}

QString ConversationSummarizer::merge(const QString& summary, const QList<ConvTurn>& recent,
                                      int maxChars) {
    QString out;
    if (!summary.trimmed().isEmpty()) out += "[Önceki konuşma özeti]\n" + summary.trimmed() + "\n\n";
    for (const ConvTurn& t : recent) {
        const QString line = QString("%1: %2\n").arg(t.role == "ai" ? "Asistan" : "Kullanıcı", t.text);
        if (out.size() + line.size() > maxChars) break;
        out += line;
    }
    return out;
}
