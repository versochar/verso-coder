#include "CallHierarchyTree.h"
#include "LspClient.h"
#include <QJsonArray>

static CallNode nodeFromItem(const QJsonObject& o) {
    CallNode n;
    n.name = o["name"].toString();
    n.detail = o["detail"].toString();
    n.uri = o["uri"].toString();
    const QJsonObject r = o["range"].toObject()["start"].toObject();
    if (r.isEmpty()) {
        const QJsonObject s = o["selectionRange"].toObject()["start"].toObject();
        n.line = s["line"].toInt();
        n.col = s["character"].toInt();
    } else {
        n.line = r["line"].toInt();
        n.col = r["character"].toInt();
    }
    return n;
}

QList<CallNode> CallHierarchyTree::parsePrepare(const QJsonObject& res) {
    QJsonValue r = res.contains("result") ? res["result"] : QJsonValue(res);
    QList<CallNode> out;
    if (r.isArray())
        for (const QJsonValue& v : r.toArray())
            if (v.isObject()) out << nodeFromItem(v.toObject());
    else if (r.isObject()) out << nodeFromItem(r.toObject());
    return out;
}

QList<CallNode> CallHierarchyTree::parseCalls(const QJsonObject& res, bool incoming) {
    QJsonValue r = res.contains("result") ? res["result"] : QJsonValue(res);
    QList<CallNode> out;
    if (!r.isArray()) return out;
    for (const QJsonValue& v : r.toArray()) {
        if (!v.isObject()) continue;
        QJsonObject o = v.toObject();
        QJsonObject item = o[incoming ? "from" : "to"].toObject();
        CallNode n = nodeFromItem(item);
        // Aralık özeti
        QJsonArray ranges = o[incoming ? "fromRanges" : "toRanges"].toArray();
        if (!ranges.isEmpty()) {
            QJsonObject s = ranges.first().toObject()["start"].toObject();
            n.rangeSummary = QString("satır %1").arg(s["line"].toInt() + 1);
        }
        out << n;
    }
    return out;
}

QJsonObject CallHierarchyTree::itemParams(const CallNode& n) {
    return QJsonObject{{"uri", n.uri},
                       {"range", QJsonObject{{"start", QJsonObject{{"line", n.line},
                                                                   {"character", n.col}}},
                                             {"end", QJsonObject{{"line", n.line},
                                                                 {"character", n.col}}}}},
                       {"selectionRange", QJsonObject{{"start", QJsonObject{{"line", n.line},
                                                                           {"character", n.col}}},
                                                     {"end", QJsonObject{{"line", n.line},
                                                                         {"character", n.col}}}}},
                       {"name", n.name}};
}
