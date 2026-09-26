#pragma once
#include <QList>

// Stage 33: anahtar kelime + anlamsal sıralamaları birleştiren hibrit sıralayıcı.
class HybridRanker {
public:
    // Reciprocal Rank Fusion: skor = wK/(k+rankK) + wS/(k+rankS).
    // keywordOrder/semanticOrder: iyi→kötü sıralı belge kimlikleri.
    static QList<int> fuse(const QList<int>& keywordOrder, const QList<int>& semanticOrder,
                           double kwWeight = 1.0, double semWeight = 1.0, int rrfK = 60,
                           int topK = 8);

    // Normalleştirilmiş ham skorlarla ağırlıklı birleşim.
    struct Item {
        int id = -1;
        double keywordScore = 0.0;
        double semanticScore = 0.0;
    };
    static QList<Item> combine(const QList<Item>& items, double kwWeight = 0.5,
                               double semWeight = 0.5, int topK = 8);
};
