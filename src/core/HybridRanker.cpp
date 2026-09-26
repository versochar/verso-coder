#include "HybridRanker.h"
#include <QHash>
#include <algorithm>

QList<int> HybridRanker::fuse(const QList<int>& keywordOrder, const QList<int>& semanticOrder,
                              double kwWeight, double semWeight, int rrfK, int topK) {
    const double k = qMax(1, rrfK);
    QHash<int, double> score;
    for (int i = 0; i < keywordOrder.size(); ++i)
        score[keywordOrder[i]] += kwWeight / (k + (i + 1));
    for (int i = 0; i < semanticOrder.size(); ++i)
        score[semanticOrder[i]] += semWeight / (k + (i + 1));
    QList<QPair<double, int>> ranked;
    ranked.reserve(score.size());
    for (auto it = score.begin(); it != score.end(); ++it)
        ranked.append({it.value(), it.key()});
    std::sort(ranked.begin(), ranked.end(), [](const QPair<double, int>& a,
                                               const QPair<double, int>& b) {
        if (a.first != b.first) return a.first > b.first;
        return a.second < b.second;
    });
    QList<int> out;
    for (int i = 0; i < ranked.size() && (topK <= 0 || out.size() < topK); ++i)
        out << ranked[i].second;
    return out;
}

QList<HybridRanker::Item> HybridRanker::combine(const QList<Item>& items, double kwWeight,
                                                double semWeight, int topK) {
    if (items.isEmpty()) return {};
    double maxK = 0, maxS = 0;
    for (const Item& it : items) {
        maxK = qMax(maxK, it.keywordScore);
        maxS = qMax(maxS, it.semanticScore);
    }
    QList<Item> out = items;
    for (Item& it : out) {
        const double nk = maxK > 0 ? it.keywordScore / maxK : 0.0;
        const double ns = maxS > 0 ? it.semanticScore / maxS : 0.0;
        it.keywordScore = kwWeight * nk + semWeight * ns; // birleşik skoru buraya yaz
    }
    std::sort(out.begin(), out.end(), [](const Item& a, const Item& b) {
        if (a.keywordScore != b.keywordScore) return a.keywordScore > b.keywordScore;
        return a.id < b.id;
    });
    if (topK > 0 && out.size() > topK) out = out.mid(0, topK);
    return out;
}
