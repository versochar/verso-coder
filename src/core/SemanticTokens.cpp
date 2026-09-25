#include "SemanticTokens.h"
#include <QJsonArray>

QList<SemanticToken> SemanticTokens::decodeData(const QList<int>& data, int& lineBase) {
    QList<SemanticToken> out;
    int line = lineBase, col = 0;
    for (int i = 0; i + 4 < data.size() + 1; i += 5) {
        int dLine = data[i], dCol = data[i + 1];
        int len = data[i + 2], type = data[i + 3], mods = data[i + 4];
        if (dLine != 0) { line += dLine; col = dCol; }
        else col += dCol;
        SemanticToken t;
        t.line = line;
        t.col = col;
        t.len = len;
        t.type = type;
        t.mods = mods;
        out << t;
    }
    lineBase = line;
    return out;
}

QList<SemanticToken> SemanticTokens::decode(const QJsonObject& res) {
    QJsonObject r = res.contains("result") ? res["result"].toObject() : res;
    QJsonArray arr = r["data"].toArray();
    QList<int> data;
    for (const QJsonValue& v : arr) data << v.toInt();
    int base = 0;
    return decodeData(data, base);
}

QStringList SemanticTokens::legend(const QJsonObject& res) {
    // initialize sonucundaki legend'den de okunabilir; burada yanıttaki legend
    QStringList out;
    QJsonArray arr = res["tokenTypes"].toArray();
    for (const QJsonValue& v : arr) out << v.toString();
    return out;
}

QString SemanticTokens::roleFor(const QString& tokenType) {
    const QString t = tokenType.toLower();
    if (t.contains("keyword")) return "keyword";
    if (t.contains("string")) return "string";
    if (t.contains("comment")) return "comment";
    if (t.contains("number")) return "number";
    if (t.contains("function") || t.contains("method")) return "func";
    if (t.contains("type") || t.contains("class") || t.contains("struct")
        || t.contains("enum") || t.contains("interface")) return "type";
    if (t.contains("variable") || t.contains("parameter") || t.contains("property"))
        return "var";
    return "other";
}
