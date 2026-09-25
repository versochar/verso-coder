#pragma once
#include <QJsonArray>
#include <QList>
#include <QString>

// Stage 17: LSP foldingRange — sunucu aralığı normalizasyon + girinti birleşimi.
struct FoldRangeLsp {
    int startLine = 0; // 0-based
    int endLine = 0;
    QString kind;      // "comment" | "imports" | "region" | ""
};

class FoldingRanges {
public:
    // LSP yanıtı (dizi ya da {result:[...]}) → normalize (sıralı, kırpılmış, iç içe)
    static QList<FoldRangeLsp> parse(const QJsonArray& arr, int lineCount);
    static QList<FoldRangeLsp> parseResult(const QJsonObject& res, int lineCount);
    // Sunucu aralığı + girinti aralığı birleşimi (sunucu öncelikli, çakışmayan eklenir)
    static QList<FoldRangeLsp> merge(const QList<FoldRangeLsp>& server,
                                     const QList<QPair<int, int>>& indent, int minSize = 2);
    static bool covers(const QList<FoldRangeLsp>& ranges, int line0);
};
