#include "InlayHintList.h"
#include <QJsonArray>

QList<InlayHint> InlayHintList::parseArray(const QJsonArray& arr) {
    QList<InlayHint> out;
    for (const QJsonValue& v : arr) {
        if (!v.isObject()) continue;
        QJsonObject o = v.toObject();
        InlayHint h;
        h.line = o["position"].toObject()["line"].toInt();
        h.col = o["position"].toObject()["character"].toInt();
        const QJsonValue lab = o["label"];
        if (lab.isString()) h.label = lab.toString();
        else if (lab.isArray()) {
            for (const QJsonValue& p : lab.toArray())
                h.label += p.isString() ? p.toString() : p.toObject()["value"].toString();
        }
        h.kind = o["kind"].toInt(0);
        if (!h.label.isEmpty()) out << h;
    }
    return out;
}

QList<InlayHint> InlayHintList::parse(const QJsonObject& res) {
    QJsonValue r = res.contains("result") ? res["result"] : QJsonValue(res);
    if (r.isArray()) return parseArray(r.toArray());
    return {};
}

QList<InlayHint> InlayHintList::forLine(const QList<InlayHint>& hints, int line) {
    QList<InlayHint> out;
    for (const InlayHint& h : hints)
        if (h.line == line) out << h;
    return out;
}
