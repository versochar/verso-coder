#include "ThemeValidator.h"
#include <QColor>
#include <QJsonDocument>
#include <QJsonObject>

QStringList ThemeValidator::requiredKeys() {
    return {"bg", "surface", "surfaceAlt", "border", "text", "textStrong", "textDim",
            "accent", "success", "warning", "error", "selection", "lineHighlight",
            "gutterBg", "gutterText", "gutterActive", "indentGuide", "bracket",
            "cursor", "scrollbar"};
}

QString ThemeValidator::extractJson(const QString& text) {
    const int a = text.indexOf('{');
    const int b = text.lastIndexOf('}');
    if (a < 0 || b <= a) return QString();
    return text.mid(a, b - a + 1);
}

bool ThemeValidator::validate(const QString& json, QString* error) {
    auto fail = [&](const QString& e) {
        if (error) *error = e;
        return false;
    };
    QJsonDocument d = QJsonDocument::fromJson(json.toUtf8());
    if (!d.isObject()) return fail("JSON nesnesi değil");
    QJsonObject o = d.object();
    if (!o.contains("colors") || !o.value("colors").isObject())
        return fail("colors nesnesi yok");
    QJsonObject c = o.value("colors").toObject();
    for (const QString& k : requiredKeys()) {
        if (!c.contains(k)) return fail("eksik renk: " + k);
        if (!QColor(c.value(k).toString()).isValid())
            return fail("geçersiz renk: " + k);
    }
    if (!c.contains("syntax") || !c.value("syntax").isObject())
        return fail("syntax nesnesi yok");
    QJsonObject s = c.value("syntax").toObject();
    for (const QString& k : {"keyword", "string", "comment", "number", "func", "type"}) {
        if (!s.contains(k)) return fail("eksik sözdizimi rengi: " + k);
        if (!QColor(s.value(k).toString()).isValid())
            return fail("geçersiz sözdizimi rengi: " + k);
    }
    if (error) error->clear();
    return true;
}
