#include "ContextBudget.h"

int ContextBudget::estimate(const QString& text) {
    return qMax(1, text.size() / 4);
}

int ContextBudget::maxFor(int contextWindow) {
    return qMax(512, contextWindow / 2);
}

QString ContextBudget::trim(const QString& text, int maxTokens) {
    if (estimate(text) <= maxTokens) return text;
    const int maxChars = maxTokens * 4;
    const int head = maxChars * 2 / 3;
    const int tail = maxChars - head;
    return text.left(head) + "\n\n[...kırpıldı: bağlam bütçesi...]\n\n" + text.right(tail);
}
