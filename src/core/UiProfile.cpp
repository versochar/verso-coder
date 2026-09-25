#include "UiProfile.h"
#include "SettingsManager.h"
#include <QJsonDocument>
#include <QJsonObject>

const char* UiProfile::profileFormatVersion() { return "1"; }

QStringList UiProfile::visualKeys() {
    return {"theme", "accentColor", "uiFontFamily", "uiFontSize", "editorFontSize",
            "lineHeight", "letterSpacing", "ligatures", "cursorWidth", "lineHighlightOn",
            "reducedMotion", "layoutPreset"};
}

QString UiProfile::exportJson(const AppSettings& s) {
    QJsonObject o;
    o["format"] = profileFormatVersion();
    o["kind"] = "verso-ui-profile";
    o["theme"] = s.theme;
    o["accentColor"] = s.accentColor;
    o["uiFontFamily"] = s.uiFontFamily;
    o["uiFontSize"] = s.uiFontSize;
    o["editorFontSize"] = s.fontSize;
    o["lineHeight"] = s.lineHeight;
    o["letterSpacing"] = s.letterSpacing;
    o["ligatures"] = s.ligatures;
    o["cursorWidth"] = s.cursorWidth;
    o["lineHighlightOn"] = s.lineHighlightOn;
    o["reducedMotion"] = s.reducedMotion;
    o["layoutPreset"] = s.layoutPreset;
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Indented));
}

static bool readStr(const QJsonObject& o, const QString& k, QString& out) {
    if (!o.contains(k)) return false;
    out = o.value(k).toString();
    return true;
}
static bool readInt(const QJsonObject& o, const QString& k, int& out) {
    if (!o.contains(k)) return false;
    out = o.value(k).toInt();
    return true;
}
static bool readDbl(const QJsonObject& o, const QString& k, double& out) {
    if (!o.contains(k)) return false;
    out = o.value(k).toDouble();
    return true;
}
static bool readBool(const QJsonObject& o, const QString& k, bool& out) {
    if (!o.contains(k)) return false;
    out = o.value(k).toBool();
    return true;
}

bool UiProfile::applyJson(AppSettings& s, const QString& json) {
    QJsonDocument d = QJsonDocument::fromJson(json.toUtf8());
    if (!d.isObject()) return false;
    QJsonObject o = d.object();
    // Geriye uyumluluk: Mitsune döneminden kalan profilleri de kabul et
    const QString kind = o.value("kind").toString();
    if (kind != "verso-ui-profile" && kind != "mitsune-ui-profile") return false;

    // Doğrulama: kopya üzerinde uygula, her şey tutarlıysa aktar
    AppSettings t = s;
    readStr(o, "theme", t.theme);
    readStr(o, "accentColor", t.accentColor);
    readStr(o, "uiFontFamily", t.uiFontFamily);
    readInt(o, "uiFontSize", t.uiFontSize);
    readInt(o, "editorFontSize", t.fontSize);
    readDbl(o, "lineHeight", t.lineHeight);
    readDbl(o, "letterSpacing", t.letterSpacing);
    readBool(o, "ligatures", t.ligatures);
    readInt(o, "cursorWidth", t.cursorWidth);
    readBool(o, "lineHighlightOn", t.lineHighlightOn);
    readBool(o, "reducedMotion", t.reducedMotion);
    readStr(o, "layoutPreset", t.layoutPreset);

    // sınır clamp'leri
    t.uiFontSize = qBound(9, t.uiFontSize, 20);
    t.fontSize = qBound(8, t.fontSize, 24);
    t.lineHeight = qBound(1.0, t.lineHeight, 2.0);
    t.letterSpacing = qBound(-5.0, t.letterSpacing, 25.0);
    t.cursorWidth = qBound(1, t.cursorWidth, 6);
    s = t;
    return true;
}
