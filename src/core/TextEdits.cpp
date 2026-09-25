#include "TextEdits.h"
#include "LspClient.h"
#include <QJsonArray>
#include <QTextDocument>

QList<LspTextEdit> TextEdits::parseArray(const QJsonArray& arr, const QString& file) {
    QList<LspTextEdit> out;
    for (const QJsonValue& v : arr) {
        if (!v.isObject()) continue;
        QJsonObject o = v.toObject();
        QJsonObject r = o["range"].toObject();
        if (r.isEmpty()) continue;
        LspTextEdit e;
        e.file = file;
        e.startLine = r["start"].toObject()["line"].toInt();
        e.startCol = r["start"].toObject()["character"].toInt();
        e.endLine = r["end"].toObject()["line"].toInt();
        e.endCol = r["end"].toObject()["character"].toInt();
        e.newText = o["newText"].toString();
        out << e;
    }
    return out;
}

QList<LspTextEdit> TextEdits::parse(const QJsonObject& res) {
    QJsonValue r = res.contains("result") ? res["result"] : QJsonValue(res);
    if (r.isArray()) return parseArray(r.toArray());
    if (r.isObject() && r.toObject().contains("edits"))
        return parseArray(r.toObject()["edits"].toArray());
    return {};
}

QList<LspTextEdit> TextEdits::parseWorkspace(const QJsonObject& res) {
    QList<LspTextEdit> out;
    QJsonObject r = res.contains("result") ? res["result"].toObject() : res;
    if (r.contains("changes") && r["changes"].isObject()) {
        QJsonObject ch = r["changes"].toObject();
        for (auto it = ch.begin(); it != ch.end(); ++it)
            out << parseArray(it.value().toArray(), LspClient::uriToPath(it.key()));
    }
    if (r.contains("documentChanges") && r["documentChanges"].isArray()) {
        for (const QJsonValue& v : r["documentChanges"].toArray()) {
            if (!v.isObject()) continue;
            QJsonObject o = v.toObject();
            QString file;
            if (o["textDocument"].isObject())
                file = LspClient::uriToPath(o["textDocument"].toObject()["uri"].toString());
            out << parseArray(o["edits"].toArray(), file);
        }
    }
    // Düz TextEdit[] de olabilir
    if (out.isEmpty() && r.contains("edits")) out = parseArray(r["edits"].toArray());
    return out;
}

static int offsetOf(const QStringList& lines, int line, int col) {
    int off = 0;
    for (int i = 0; i < line && i < lines.size(); ++i) off += lines[i].size() + 1;
    return off + col;
}

QString TextEdits::apply(const QString& text, QList<LspTextEdit> edits) {
    QStringList lines = text.split('\n');
    struct R { int a, b; QString t; };
    QList<R> rs;
    for (const LspTextEdit& e : edits) {
        int a = offsetOf(lines, e.startLine, e.startCol);
        int b = offsetOf(lines, e.endLine, e.endCol);
        rs << R{a, b, e.newText};
    }
    // Sondan başa uygula (ofset kaymasını önler)
    std::sort(rs.begin(), rs.end(), [](const R& x, const R& y) { return x.a > y.a; });
    QString out = text;
    for (const R& r : rs) {
        int a = qBound(0, r.a, out.size());
        int b = qBound(a, r.b, out.size());
        out.replace(a, b - a, r.t);
    }
    return out;
}

QStringList TextEdits::preview(const QString& text, const QList<LspTextEdit>& edits,
                               int context) {
    Q_UNUSED(context);
    QStringList lines = text.split('\n');
    QStringList out;
    for (const LspTextEdit& e : edits) {
        const QString oldL = (e.startLine >= 0 && e.startLine < lines.size())
            ? lines[e.startLine].trimmed().left(80) : QString();
        const QString newL = e.newText.trimmed().left(80).replace('\n', QChar(0x23CE));
        out << QString("Satır %1: %2 → %3").arg(e.startLine + 1).arg(oldL, newL);
    }
    return out;
}
