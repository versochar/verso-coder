#include "SymbolTree.h"
#include "LspClient.h"
#include <QJsonArray>

static SymbolNode nodeFromDoc(const QJsonObject& o, const QString& container) {
    SymbolNode n;
    n.name = o["name"].toString();
    n.detail = o["detail"].toString();
    n.kind = o["kind"].toInt();
    n.container = container;
    const QJsonObject r = o["range"].toObject().isEmpty()
        ? o["selectionRange"].toObject() : o["range"].toObject();
    const QJsonObject s = r["start"].toObject();
    const QJsonObject e = r["end"].toObject();
    // Bazı sunucular range yerine location verir (SymbolInformation)
    if (o.contains("location") && o["location"].isObject()) {
        const QJsonObject loc = o["location"].toObject();
        n.path = LspClient::uriToPath(loc["uri"].toString());
        const QJsonObject rs = loc["range"].toObject()["start"].toObject();
        n.line = rs["line"].toInt();
        n.col = rs["character"].toInt();
    } else {
        n.line = s["line"].toInt();
        n.col = s["character"].toInt();
        n.endLine = e["line"].toInt();
    }
    QJsonArray ch = o["children"].toArray();
    for (const QJsonValue& c : ch)
        if (c.isObject()) n.children << nodeFromDoc(c.toObject(), n.name);
    return n;
}

QList<SymbolNode> SymbolTree::parseDocumentArray(const QJsonArray& arr,
                                                const QString& container) {
    QList<SymbolNode> out;
    for (const QJsonValue& v : arr) {
        if (!v.isObject()) continue;
        QJsonObject o = v.toObject();
        if (o.contains("name"))
            out << nodeFromDoc(o, container);
        else if (o.contains("location")) {
            // SymbolInformation biçimi
            SymbolNode n;
            n.name = o["name"].toString();
            n.kind = o["kind"].toInt();
            n.container = o["containerName"].toString();
            const QJsonObject loc = o["location"].toObject();
            n.path = LspClient::uriToPath(loc["uri"].toString());
            const QJsonObject s = loc["range"].toObject()["start"].toObject();
            n.line = s["line"].toInt();
            n.col = s["character"].toInt();
            out << n;
        }
    }
    return out;
}

QList<SymbolNode> SymbolTree::parseDocument(const QJsonObject& res) {
    if (res.contains("result") && res["result"].isArray())
        return parseDocumentArray(res["result"].toArray());
    QJsonArray arr;
    for (auto it = res.begin(); it != res.end(); ++it) { Q_UNUSED(it); }
    // Düz dizi olamaz (QJsonObject) — çağıran parseDocumentArray kullansın
    return {};
}

QList<SymbolNode> SymbolTree::parseWorkspace(const QJsonObject& res) {
    QJsonArray arr = res["result"].toArray();
    if (arr.isEmpty() && res.contains("items")) arr = res["items"].toArray();
    // result tek obje olabilir
    if (arr.isEmpty() && res.contains("result") && res["result"].isObject()) {
        QJsonObject r = res["result"].toObject();
        if (r.contains("name")) return parseDocumentArray({r});
    }
    return parseDocumentArray(arr);
}

QList<SymbolNode> SymbolTree::flatten(const QList<SymbolNode>& roots) {
    QList<SymbolNode> out;
    std::function<void(const SymbolNode&)> walk = [&](const SymbolNode& n) {
        out << n;
        for (const SymbolNode& c : n.children) walk(c);
    };
    for (const SymbolNode& n : roots) walk(n);
    return out;
}

QString SymbolTree::kindTitle(int kind) {
    switch (kind) {
    case 5: return "Sınıf";
    case 6: return "Metot";
    case 11: return "Arayüz";
    case 12: return "Fonksiyon";
    case 13: return "Değişken";
    case 10: return "Enum";
    case 16: return "Alan";
    case 23: return "Struct";
    case 8: return "Modül";
    default: return "Sembol";
    }
}

QString SymbolTree::kindIcon(int kind) {
    switch (kind) {
    case 5: case 23: case 11: return "▣";
    case 6: case 12: return "ƒ";
    case 13: case 16: return "v";
    case 8: return "▤";
    case 10: return "≡";
    default: return "•";
    }
}
