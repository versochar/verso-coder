#include "LaunchConfig.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

QString LaunchConfig::configPath(const QString& root) {
    return QDir(root).absoluteFilePath(".verso/launch.json");
}

static QString legacyPath(const QString& root) {
    return QDir(root).absoluteFilePath(".mitsune/launch.json");
}

LaunchConfig LaunchConfig::defaults(const QString& root) {
    LaunchConfig c;
    // Kökteki ilk çalıştırılabilir adayı tahmin et (build/*)
    QDir build(QDir(root).absoluteFilePath("build"));
    if (build.exists()) {
        const QStringList exes = build.entryList(QDir::Files | QDir::Executable);
        if (!exes.isEmpty()) c.program = "build/" + exes.first();
    }
    if (c.program.isEmpty()) c.program = "build/program";
    c.cwd = "";
    c.stopAtEntry = false;
    return c;
}

LaunchConfig LaunchConfig::load(const QString& root) {
    LaunchConfig c = defaults(root);
    QString path = configPath(root);
    if (!QFile::exists(path)) path = legacyPath(root);
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return c;
    const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
    if (o.isEmpty()) return c;
    if (!o.value("program").toString().isEmpty()) c.program = o.value("program").toString();
    const QJsonValue av = o.value("args");
    if (av.isArray()) {
        c.args.clear();
        for (const QJsonValue& v : av.toArray()) c.args << v.toString();
    } else if (av.isString() && !av.toString().isEmpty()) {
        c.args = av.toString().split(' ', Qt::SkipEmptyParts);
    }
    c.cwd = o.value("cwd").toString();
    c.preBuild = o.value("preBuild").toString();
    c.stopAtEntry = o.value("stopAtEntry").toBool(false);
    return c;
}

bool LaunchConfig::save(const QString& root) const {
    QDir().mkpath(QDir(root).absoluteFilePath(".verso"));
    QJsonObject o;
    o["program"] = program;
    QJsonArray arr;
    for (const QString& a : args) arr << a;
    o["args"] = arr;
    o["cwd"] = cwd;
    o["preBuild"] = preBuild;
    o["stopAtEntry"] = stopAtEntry;
    QFile f(configPath(root));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    return true;
}

QString LaunchConfig::resolvedProgram(const QString& root) const {
    if (QDir::isAbsolutePath(program)) return program;
    return QDir(root).absoluteFilePath(program);
}

QString LaunchConfig::resolvedCwd(const QString& root) const {
    if (!cwd.isEmpty())
        return QDir::isAbsolutePath(cwd) ? cwd : QDir(root).absoluteFilePath(cwd);
    return root;
}
