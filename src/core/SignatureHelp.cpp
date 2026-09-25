#include "SignatureHelp.h"
#include <QJsonArray>

SignatureHelpData SignatureHelp::parse(const QJsonObject& res) {
    SignatureHelpData d;
    QJsonObject r = res.contains("result") ? res["result"].toObject() : res;
    // result null olabilir
    if (r.isEmpty()) return d;
    d.activeSig = r["activeSignature"].toInt(0);
    d.activeParam = r["activeParameter"].toInt(0);
    for (const QJsonValue& v : r["signatures"].toArray()) {
        if (!v.isObject()) continue;
        QJsonObject o = v.toObject();
        SigInfo s;
        s.label = o["label"].toString();
        const QJsonValue doc = o["documentation"];
        s.doc = doc.isString() ? doc.toString() : doc.toObject()["value"].toString();
        for (const QJsonValue& pv : o["parameters"].toArray()) {
            if (!pv.isObject()) continue;
            QJsonObject po = pv.toObject();
            SigParam p;
            const QJsonValue lab = po["label"];
            if (lab.isString()) p.label = lab.toString();
            else if (lab.isArray()) {
                QJsonArray la = lab.toArray();
                p.label = QString("%1-%2")
                    .arg(la.size() > 0 ? la.at(0).toInt() : 0)
                    .arg(la.size() > 1 ? la.at(1).toInt() : 0);
            }
            const QJsonValue pd = po["documentation"];
            p.doc = pd.isString() ? pd.toString() : pd.toObject()["value"].toString();
            s.params << p;
        }
        d.sigs << s;
    }
    return d;
}

QString SignatureHelp::render(const SignatureHelpData& d) {
    if (d.empty()) return QString();
    const SigInfo& s = d.sigs[qBound(0, d.activeSig, d.sigs.size() - 1)];
    QString out = s.label;
    if (d.sigs.size() > 1)
        out += QString("  (%1/%2)").arg(d.activeSig + 1).arg(d.sigs.size());
    return out;
}
