#include "SettingsIO.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QSettings>

QJsonObject SettingsIO::dump(QSettings& q) {
    QJsonObject root;
    for (const QString& key : q.allKeys()) {
        const QVariant v = q.value(key);
        QJsonObject entry;
        switch (v.typeId()) {
        case QMetaType::Bool: entry["t"] = "b"; entry["v"] = v.toBool(); break;
        case QMetaType::Int:
        case QMetaType::LongLong: entry["t"] = "i"; entry["v"] = double(v.toLongLong()); break;
        case QMetaType::Double: entry["t"] = "d"; entry["v"] = v.toDouble(); break;
        case QMetaType::QStringList: {
            entry["t"] = "sl";
            QJsonArray a;
            for (const QString& s : v.toStringList()) a.append(s);
            entry["v"] = a;
            break;
        }
        default: entry["t"] = "s"; entry["v"] = v.toString(); break;
        }
        root[key] = entry;
    }
    return root;
}

bool SettingsIO::load(QSettings& q, const QJsonObject& obj, QString* error) {
    if (error) error->clear();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        const QJsonObject e = it.value().toObject();
        const QString t = e.value("t").toString("s");
        const QJsonValue v = e.value("v");
        if (t == "b") q.setValue(it.key(), v.toBool());
        else if (t == "i") q.setValue(it.key(), qint64(v.toDouble()));
        else if (t == "d") q.setValue(it.key(), v.toDouble());
        else if (t == "sl") {
            QStringList sl;
            for (const QJsonValue& s : v.toArray()) sl << s.toString();
            q.setValue(it.key(), sl);
        } else q.setValue(it.key(), v.toString());
    }
    q.sync();
    return true;
}

bool SettingsIO::writeFile(const QString& path, QSettings& q, QString* error) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) *error = "yazılamadı: " + path;
        return false;
    }
    f.write(QJsonDocument(dump(q)).toJson(QJsonDocument::Indented));
    return true;
}

bool SettingsIO::readFile(const QString& path, QSettings& q, QString* error) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) *error = "açılamadı: " + path;
        return false;
    }
    QJsonDocument d = QJsonDocument::fromJson(f.readAll());
    if (!d.isObject()) {
        if (error) *error = "geçersiz JSON";
        return false;
    }
    return load(q, d.object(), error);
}
