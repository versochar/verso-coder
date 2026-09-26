#include "ExternalTools.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

QList<ToolDef> ExternalTools::parse(const QString& json, QString* error) {
    QList<ToolDef> out;
    if (error) error->clear();
    QJsonDocument d = QJsonDocument::fromJson(json.toUtf8());
    if (!d.isObject() && !d.isArray()) {
        if (error) *error = "geçersiz JSON";
        return out;
    }
    const QJsonArray arr = d.isArray() ? d.array() : d.object().value("tools").toArray();
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        ToolDef t;
        t.label = o.value("label").toString().trimmed();
        t.command = o.value("command").toString();
        t.cwd = o.value("cwd").toString();
        if (!t.label.isEmpty() && !t.command.isEmpty()) out << t;
    }
    return out;
}

QString ExternalTools::configPathForRoot(const QString& root) {
    return QDir(root).absoluteFilePath(".verso/tools.json");
}

QString ExternalTools::commandId(const QString& label) {
    QString id = label.toLower();
    id.replace(QRegularExpression(R"([^a-z0-9çğıöşü]+)"), "-");
    id.replace(QRegularExpression(R"(^-+|-+$)"), "");
    if (id.isEmpty()) id = "arac";
    return "tool." + id.left(40);
}
