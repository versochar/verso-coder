#include "CodeActionList.h"
#include <QJsonArray>

QList<CodeActionItem> CodeActionList::parseArray(const QJsonArray& arr) {
    QList<CodeActionItem> out;
    for (const QJsonValue& v : arr) {
        if (!v.isObject()) continue;
        QJsonObject o = v.toObject();
        CodeActionItem it;
        it.title = o["title"].toString();
        if (it.title.isEmpty()) continue;
        it.kind = o["kind"].toString();
        // Command biçimi: {title, command} (kind yok)
        it.hasEdit = o.contains("edit");
        it.hasCommand = o.contains("command") || o.contains("command_");
        if (!it.hasEdit && !it.hasCommand && o.contains("command")) it.hasCommand = true;
        // command alanı string veya obje olabilir
        if (o.contains("command") && !o["command"].isObject() && o["command"].isString())
            it.hasCommand = true;
        it.raw = o;
        out << it;
    }
    return out;
}

QList<CodeActionItem> CodeActionList::parse(const QJsonObject& res) {
    QJsonValue r = res.contains("result") ? res["result"] : QJsonValue(res);
    if (r.isArray()) return parseArray(r.toArray());
    return {};
}

QList<CodeActionItem> CodeActionList::filterByKind(const QList<CodeActionItem>& items,
                                                  const QString& kindPrefix) {
    if (kindPrefix.isEmpty()) return items;
    QList<CodeActionItem> out;
    for (const CodeActionItem& it : items)
        if (it.kind.startsWith(kindPrefix)) out << it;
    return out;
}
