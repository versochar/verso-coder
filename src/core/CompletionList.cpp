#include "CompletionList.h"
#include <QJsonArray>

QList<CompletionItem> CompletionList::parse(const QJsonObject& res) {
    QJsonArray arr;
    if (res.contains("items") && res["items"].isArray()) arr = res["items"].toArray();
    else if (res.contains("result") && res["result"].isArray()) arr = res["result"].toArray();
    else if (res.contains("result") && res["result"].isObject()
             && res["result"].toObject()["items"].isArray())
        arr = res["result"].toObject()["items"].toArray();
    else if (!res.isEmpty() && res.begin() != res.end() && res.contains("label")) {
        // tek öğe (nadir)
    }
    QList<CompletionItem> out;
    auto one = [&](const QJsonObject& o) {
        CompletionItem it;
        it.label = o["label"].toString();
        if (it.label.isEmpty()) return;
        it.raw = o; // Stage 27
        // labelDetails.detail
        it.detail = o["detail"].toString();
        const QJsonObject ld = o["labelDetails"].toObject();
        if (!ld.isEmpty() && it.detail.isEmpty())
            it.detail = ld["detail"].toString() + ld["description"].toString();
        it.kind = o["kind"].toInt(1);
        if (o.contains("textEdit") && o["textEdit"].isObject())
            it.insertText = o["textEdit"].toObject()["newText"].toString();
        if (it.insertText.isEmpty()) it.insertText = o["insertText"].toString(it.label);
        it.sortText = o["sortText"].toString(it.label);
        it.filterText = o["filterText"].toString(it.label);
        out << it;
    };
    for (const QJsonValue& v : arr)
        if (v.isObject()) one(v.toObject());
    if (out.isEmpty() && res.contains("label")) one(res);
    return out;
}

QList<CompletionItem> CompletionList::filter(const QList<CompletionItem>& items,
                                            const QString& prefix, int limit) {
    QList<CompletionItem> out;
    const QString p = prefix.toLower();
    for (const CompletionItem& it : items) {
        const QString f = (it.filterText.isEmpty() ? it.label : it.filterText).toLower();
        if (p.isEmpty() || f.startsWith(p) || f.contains(p)) out << it;
    }
    std::sort(out.begin(), out.end(), [](const CompletionItem& a, const CompletionItem& b) {
        if (a.sortText != b.sortText) return a.sortText < b.sortText;
        return a.label.size() < b.label.size();
    });
    if (limit > 0 && out.size() > limit) out = out.mid(0, limit);
    return out;
}

QString CompletionList::kindTitle(int kind) {
    switch (kind) {
    case 2: return "Metot";
    case 3: return "Fonksiyon";
    case 4: return "Kurucu";
    case 5: return "Alan";
    case 6: return "Değişken";
    case 7: return "Sınıf";
    case 8: return "Arayüz";
    case 9: return "Modül";
    case 10: return "Özellik";
    case 14: return "Anahtar kelime";
    case 15: return "Parçacık";
    case 17: return "Dosya";
    case 18: return "Referans";
    case 22: return "Tür";
    default: return "Metin";
    }
}

QString CompletionList::kindIcon(int kind) {
    switch (kind) {
    case 2: case 3: case 4: return "ƒ";
    case 5: case 6: case 10: return "v";
    case 7: case 8: case 22: return "▣";
    case 9: case 17: return "▤";
    case 14: return "k";
    case 15: return "◈";
    default: return "•";
    }
}
