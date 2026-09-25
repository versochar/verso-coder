#pragma once
#include <QString>

// Stage 15: bağlam bütçesi — kaba token tahmini + kırpma (1 token ≈ 4 karakter).
class ContextBudget {
public:
    static int estimate(const QString& text); // ~token
    // maxTokens aşılırsa baştan + sondan dengeli kırp, ortaya "[...kırpıldı...]" koy
    static QString trim(const QString& text, int maxTokens);
    static int maxFor(int contextWindow); // model penceresinin yarısı (yanıt payı)
};
