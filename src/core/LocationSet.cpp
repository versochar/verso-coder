#include "LocationSet.h"
#include "LspClient.h"
#include <QJsonArray>

static bool fillLoc(LspLocation& l, const QJsonObject& loc) {
    if (!loc.contains("uri")) return false;
    l.path = LspClient::uriToPath(loc["uri"].toString());
    const QJsonObject r = loc["range"].toObject();
    const QJsonObject s = r["start"].toObject(), e = r["end"].toObject();
    l.line = s["line"].toInt();
    l.col = s["character"].toInt();
    l.endLine = e["line"].toInt();
    l.endCol = e["character"].toInt();
    return true;
}

QList<LspLocation> LocationSet::parseArray(const QJsonArray& arr) {
    QList<LspLocation> out;
    for (const QJsonValue& v : arr) {
        if (!v.isObject()) continue;
        QJsonObject o = v.toObject();
        if (o.contains("uri")) {
            LspLocation l;
            if (fillLoc(l, o)) out << l;
        } else if (o.contains("targetUri")) {
            // LocationLink
            LspLocation l;
            l.path = LspClient::uriToPath(o["targetUri"].toString());
            const QJsonObject r = o["targetRange"].toObject();
            const QJsonObject s = r["start"].toObject(), e = r["end"].toObject();
            l.line = s["line"].toInt();
            l.col = s["character"].toInt();
            l.endLine = e["line"].toInt();
            l.endCol = e["character"].toInt();
            out << l;
        }
    }
    return out;
}

QList<LspLocation> LocationSet::parse(const QJsonObject& res) {
    QJsonValue r = res.contains("result") ? res["result"] : QJsonValue(res);
    if (r.isArray()) return parseArray(r.toArray());
    if (r.isObject()) {
        QJsonObject o = r.toObject();
        if (o.contains("uri") || o.contains("targetUri")) return parseArray({o});
    }
    return {};
}

QStringList LocationSet::files(const QList<LspLocation>& locs) {
    QStringList out;
    for (const LspLocation& l : locs)
        if (!out.contains(l.path)) out << l.path;
    std::sort(out.begin(), out.end());
    return out;
}

QList<LspLocation> LocationSet::forFile(const QList<LspLocation>& locs, const QString& path) {
    QList<LspLocation> out;
    for (const LspLocation& l : locs)
        if (l.path == path) out << l;
    std::sort(out.begin(), out.end(), [](const LspLocation& a, const LspLocation& b) {
        if (a.line != b.line) return a.line < b.line;
        return a.col < b.col;
    });
    return out;
}
