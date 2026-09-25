#include "WorkspaceConfig.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>

QString WorkspaceConfig::configPath(const QString& root) {
    return QDir(root).absoluteFilePath(".verso/workspace.json");
}

// Mitsune döneminden kalan yapılandırma (salt okuma desteği)
static QString legacyPath(const QString& root) {
    return QDir(root).absoluteFilePath(".mitsune/workspace.json");
}

bool WorkspaceConfig::exists(const QString& root) {
    return QFile::exists(configPath(root)) || QFile::exists(legacyPath(root));
}

WorkspaceSettings WorkspaceConfig::load(const QString& root) {
    WorkspaceSettings w;
    QString path = configPath(root);
    if (!QFile::exists(path)) path = legacyPath(root);
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return w;
    QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
    if (o.isEmpty()) return w;
    w.raw = o;
    QJsonObject ed = o.value("editor").toObject();
    w.fontSize = ed.value("fontSize").toInt(0);
    w.tabWidth = ed.value("tabWidth").toInt(0);
    w.largeFileMb = ed.value("largeFileMb").toInt(0);
    if (ed.contains("useEditorConfig")) w.useEditorConfig = ed.value("useEditorConfig").toBool(true);
    w.keymapPreset = o.value("keymapPreset").toString();
    w.buildTaskLabel = o.value("buildTask").toString();
    return w;
}

bool WorkspaceConfig::save(const QString& root, const WorkspaceSettings& w) {
    QJsonObject o = w.raw;
    QJsonObject ed = o.value("editor").toObject();
    if (w.fontSize > 0) ed["fontSize"] = w.fontSize; else ed.remove("fontSize");
    if (w.tabWidth > 0) ed["tabWidth"] = w.tabWidth; else ed.remove("tabWidth");
    if (w.largeFileMb > 0) ed["largeFileMb"] = w.largeFileMb; else ed.remove("largeFileMb");
    ed["useEditorConfig"] = w.useEditorConfig;
    o["editor"] = ed;
    if (!w.keymapPreset.isEmpty()) o["keymapPreset"] = w.keymapPreset; else o.remove("keymapPreset");
    if (!w.buildTaskLabel.isEmpty()) o["buildTask"] = w.buildTaskLabel; else o.remove("buildTask");

    const QString path = configPath(root);
    QDir().mkpath(QDir(root).absoluteFilePath(".verso"));
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    return true;
}
