#include "ThemeTokens.h"
#include <QJsonDocument>
#include <QJsonObject>

ThemeTokens ThemeTokens::defaults(bool dark) {
    ThemeTokens t;
    t.dark = dark;
    t.name = dark ? "dark" : "light";
    if (dark) {
        t.bg = "#1e1e1e";        t.surface = "#252526";  t.surfaceAlt = "#2d2d2d";
        t.border = "#3c3c3c";    t.text = "#d4d4d4";     t.textStrong = "#ffffff";
        t.textDim = "#858585";   t.accent = "#007acc";   t.success = "#4ec9b0";
        t.warning = "#dcdcaa";   t.error = "#f44747";    t.selection = "#264f78";
        t.lineHighlight = "#2a2d2e";
        t.gutterBg = "#1e1e1e";  t.gutterText = "#858585"; t.gutterActive = "#c6c6c6";
        t.indentGuide = "#404040"; t.bracket = "#ffd700"; t.cursor = "#aeafad";
        t.scrollbar = "#424242";
        t.synKeyword = "#569cd6"; t.synString = "#ce9178"; t.synComment = "#6a9955";
        t.synNumber = "#b5cea8";  t.synFunc = "#dcdcaa";   t.synType = "#4ec9b0";
    } else {
        t.bg = "#ffffff";        t.surface = "#f3f3f3";  t.surfaceAlt = "#e8e8e8";
        t.border = "#cccccc";    t.text = "#1e1e1e";     t.textStrong = "#000000";
        t.textDim = "#6e6e6e";   t.accent = "#007acc";   t.success = "#098658";
        t.warning = "#cca700";   t.error = "#cd3131";    t.selection = "#add6ff";
        t.lineHighlight = "#eef2f8";
        t.gutterBg = "#ffffff";  t.gutterText = "#6e6e6e"; t.gutterActive = "#171717";
        t.indentGuide = "#d0d0d0"; t.bracket = "#b38600"; t.cursor = "#333333";
        t.scrollbar = "#c1c1c1";
        t.synKeyword = "#0000ff"; t.synString = "#a31515"; t.synComment = "#008000";
        t.synNumber = "#098658";  t.synFunc = "#795e26";   t.synType = "#267f99";
    }
    return t;
}

static QColor col(const QJsonObject& o, const QString& key, const QColor& def) {
    const QString v = o.value(key).toString();
    QColor c(v);
    return c.isValid() ? c : def;
}

ThemeTokens ThemeTokens::fromJson(const QString& json, QString* error) {
    ThemeTokens t;
    if (error) error->clear();
    QJsonDocument d = QJsonDocument::fromJson(json.toUtf8());
    if (!d.isObject()) {
        if (error) *error = "geçersiz tema JSON'u";
        return t;
    }
    QJsonObject o = d.object();
    t.name = o.value("name").toString();
    if (t.name.isEmpty()) { if (error) *error = "tema adı yok"; return t; }
    t.dark = o.value("dark").toBool(true);
    QJsonObject c = o.value("colors").toObject();
    if (c.isEmpty()) { if (error) *error = "colors nesnesi yok"; return t; }
    const ThemeTokens def = defaults(t.dark);
    t.bg            = col(c, "bg", def.bg);
    t.surface       = col(c, "surface", def.surface);
    t.surfaceAlt    = col(c, "surfaceAlt", def.surfaceAlt);
    t.border        = col(c, "border", def.border);
    t.text          = col(c, "text", def.text);
    t.textStrong    = col(c, "textStrong", def.textStrong);
    t.textDim       = col(c, "textDim", def.textDim);
    t.accent        = col(c, "accent", def.accent);
    t.success       = col(c, "success", def.success);
    t.warning       = col(c, "warning", def.warning);
    t.error         = col(c, "error", def.error);
    t.selection     = col(c, "selection", def.selection);
    t.lineHighlight = col(c, "lineHighlight", def.lineHighlight);
    t.gutterBg      = col(c, "gutterBg", def.gutterBg);
    t.gutterText    = col(c, "gutterText", def.gutterText);
    t.gutterActive  = col(c, "gutterActive", def.gutterActive);
    t.indentGuide   = col(c, "indentGuide", def.indentGuide);
    t.bracket       = col(c, "bracket", def.bracket);
    t.cursor        = col(c, "cursor", def.cursor);
    t.scrollbar     = col(c, "scrollbar", def.scrollbar);
    QJsonObject syn = c.value("syntax").toObject();
    t.synKeyword    = col(syn, "keyword", def.synKeyword);
    t.synString     = col(syn, "string", def.synString);
    t.synComment    = col(syn, "comment", def.synComment);
    t.synNumber     = col(syn, "number", def.synNumber);
    t.synFunc       = col(syn, "func", def.synFunc);
    t.synType       = col(syn, "type", def.synType);
    return t;
}

QJsonObject ThemeTokens::toJson() const {
    QJsonObject c;
    c["bg"] = bg.name();            c["surface"] = surface.name();
    c["surfaceAlt"] = surfaceAlt.name(); c["border"] = border.name();
    c["text"] = text.name();        c["textStrong"] = textStrong.name();
    c["textDim"] = textDim.name();  c["accent"] = accent.name();
    c["success"] = success.name();  c["warning"] = warning.name();
    c["error"] = error.name();      c["selection"] = selection.name();
    c["lineHighlight"] = lineHighlight.name();
    c["gutterBg"] = gutterBg.name(); c["gutterText"] = gutterText.name();
    c["gutterActive"] = gutterActive.name();
    c["indentGuide"] = indentGuide.name(); c["bracket"] = bracket.name();
    c["cursor"] = cursor.name();    c["scrollbar"] = scrollbar.name();
    QJsonObject syn;
    syn["keyword"] = synKeyword.name(); syn["string"] = synString.name();
    syn["comment"] = synComment.name(); syn["number"] = synNumber.name();
    syn["func"] = synFunc.name();       syn["type"] = synType.name();
    c["syntax"] = syn;
    QJsonObject o;
    o["name"] = name;
    o["dark"] = dark;
    o["colors"] = c;
    return o;
}

QString ThemeTokens::toJsonString() const {
    return QString::fromUtf8(QJsonDocument(toJson()).toJson(QJsonDocument::Indented));
}

bool ThemeTokens::isValid() const {
    return !name.isEmpty() && bg.isValid() && text.isValid() && accent.isValid();
}

QColor ThemeTokens::mix(const QColor& a, const QColor& b, double t) {
    t = qBound(0.0, t, 1.0);
    return QColor(int(a.red()   + (b.red()   - a.red())   * t),
                  int(a.green() + (b.green() - a.green()) * t),
                  int(a.blue()  + (b.blue()  - a.blue())  * t));
}

QColor ThemeTokens::withAlphaF(const QColor& c, double a) {
    QColor r = c;
    r.setAlphaF(qBound(0.0, a, 1.0));
    return r;
}
