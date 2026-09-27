#include "ColorBlind.h"
#include <QJsonValue>
#include "ThemeTokens.h"
#include <QJsonDocument>
#include <QJsonObject>

ColorBlind::Mode ColorBlind::fromName(const QString& name) {
    if (name == "deuteranopia") return Mode::Deuteranopia;
    if (name == "protanopia") return Mode::Protanopia;
    if (name == "tritanopia") return Mode::Tritanopia;
    return Mode::None;
}

QString ColorBlind::name(Mode m) {
    if (m == Mode::Deuteranopia) return "deuteranopia";
    if (m == Mode::Protanopia) return "protanopia";
    if (m == Mode::Tritanopia) return "tritanopia";
    return "none";
}

QStringList ColorBlind::names() {
    return {"none", "deuteranopia", "protanopia", "tritanopia"};
}

QString ColorBlind::title(const QString& name) {
    if (name == "deuteranopia") return "Deuteranopi (yeşil-kırmızı)";
    if (name == "protanopia") return "Protanopi (kırmızı-körlük)";
    if (name == "tritanopia") return "Tritanopi (mavi-sarı)";
    return "Normal";
}

QColor ColorBlind::adjust(const QColor& c, Mode m) {
    if (m == Mode::None || !c.isValid()) return c;
    const double r = c.redF(), g = c.greenF(), b = c.blueF();
    double or_, og, ob;
    if (m == Mode::Protanopia) {
        or_ = 0.567 * r + 0.433 * g; og = 0.558 * r + 0.442 * g; ob = 0.242 * g + 0.758 * b;
    } else if (m == Mode::Deuteranopia) {
        or_ = 0.625 * r + 0.375 * g; og = 0.700 * r + 0.300 * g; ob = 0.300 * g + 0.700 * b;
    } else {
        or_ = 0.950 * r + 0.050 * g; og = 0.433 * g + 0.567 * b; ob = 0.475 * g + 0.525 * b;
    }
    QColor out;
    out.setRgbF(qBound(0.0, or_, 1.0), qBound(0.0, og, 1.0), qBound(0.0, ob, 1.0), c.alphaF());
    return out;
}

static void adjustObject(QJsonObject& o, ColorBlind::Mode m) {
    for (const QString& k : o.keys()) {
        QJsonValue v = o.value(k);
        if (v.isString()) {
            const QColor c(v.toString());
            if (c.isValid()) o[k] = ColorBlind::adjust(c, m).name();
        } else if (v.isObject()) {
            QJsonObject sub = v.toObject();
            adjustObject(sub, m);
            o[k] = sub;
        }
    }
}

ThemeTokens ColorBlind::applyTo(const ThemeTokens& tk, Mode m) {
    if (m == Mode::None) return tk;
    QJsonObject o = tk.toJson();
    if (o.contains("colors") && o.value("colors").isObject()) {
        QJsonObject colors = o.value("colors").toObject();
        adjustObject(colors, m);
        o["colors"] = colors;
    }
    QString err;
    ThemeTokens out = ThemeTokens::fromJson(
        QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)), &err);
    if (!err.isEmpty()) return tk; // güvenlik: çözümlenemezse orijinal
    out.name = tk.name;
    return out;
}
