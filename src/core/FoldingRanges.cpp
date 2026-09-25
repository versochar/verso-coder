#include "FoldingRanges.h"
#include <QJsonObject>
#include <algorithm>

QList<FoldRangeLsp> FoldingRanges::parse(const QJsonArray& arr, int lineCount) {
    QList<FoldRangeLsp> out;
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        FoldRangeLsp r;
        r.startLine = qBound(0, o.value("startLine").toInt(), qMax(0, lineCount - 1));
        r.endLine = qBound(0, o.value("endLine").toInt(), qMax(0, lineCount - 1));
        r.kind = o.value("kind").toString();
        if (r.endLine > r.startLine) out << r;
    }
    std::sort(out.begin(), out.end(), [](const FoldRangeLsp& a, const FoldRangeLsp& b) {
        if (a.startLine != b.startLine) return a.startLine < b.startLine;
        return a.endLine > b.endLine;
    });
    // Aynı başlangıçlı kopyaları ele (en geniş kalsın — sıralama garantiler)
    QList<FoldRangeLsp> uniq;
    for (const FoldRangeLsp& r : out) {
        if (!uniq.isEmpty() && uniq.last().startLine == r.startLine) continue;
        uniq << r;
    }
    return uniq;
}

QList<FoldRangeLsp> FoldingRanges::parseResult(const QJsonObject& res, int lineCount) {
    if (res.contains("result")) return parse(res["result"].toArray(), lineCount);
    return {};
}

QList<FoldRangeLsp> FoldingRanges::merge(const QList<FoldRangeLsp>& server,
                                        const QList<QPair<int, int>>& indent,
                                        int minSize) {
    QList<FoldRangeLsp> out = server;
    for (const auto& pr : indent) {
        if (pr.second - pr.first + 1 < minSize) continue;
        bool clash = false;
        for (const FoldRangeLsp& s : server) {
            // Başlangıç satırları çakışıyorsa sunucu kazanır
            if (qAbs(s.startLine - pr.first) <= 1) { clash = true; break; }
        }
        if (!clash) out << FoldRangeLsp{pr.first, pr.second, "indent"};
    }
    std::sort(out.begin(), out.end(), [](const FoldRangeLsp& a, const FoldRangeLsp& b) {
        return a.startLine < b.startLine;
    });
    return out;
}

bool FoldingRanges::covers(const QList<FoldRangeLsp>& ranges, int line0) {
    for (const FoldRangeLsp& r : ranges)
        if (line0 >= r.startLine && line0 <= r.endLine) return true;
    return false;
}
