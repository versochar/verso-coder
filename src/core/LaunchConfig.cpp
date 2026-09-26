#include "LaunchConfig.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
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
    const QString p = expandVars(program, root);
    if (QDir::isAbsolutePath(p)) return p;
    return QDir(root).absoluteFilePath(p);
}

QString LaunchConfig::resolvedCwd(const QString& root) const {
    if (cwd.isEmpty()) return root;
    const QString c = expandVars(cwd, root);
    return QDir::isAbsolutePath(c) ? c : QDir(root).absoluteFilePath(c);
}

QStringList LaunchConfig::resolvedArgs(const QString& root, const QString& file) const {
    QStringList out;
    for (const QString& a : args) out << expandVars(a, root, file);
    return out;
}

QString LaunchConfig::expandVars(const QString& text, const QString& root,
                                 const QString& file) {
    QString out = text;
    out.replace("${workspaceFolder}", root);
    out.replace("${file}", file);
    out.replace("${fileBasename}", QFileInfo(file).fileName());
    out.replace("${fileDirname}", QFileInfo(file).absolutePath());
    out.replace("${pathSeparator}", QDir::separator());
    return out;
}

QStringList LaunchConfig::knownKeys() {
    return {"program", "args", "cwd", "preBuild", "stopAtEntry"};
}

QStringList LaunchConfig::unknownKeys(const QString& root) {
    QString path = configPath(root);
    if (!QFile::exists(path)) path = legacyPath(root);
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
    QStringList out;
    for (const QString& k : o.keys())
        if (!knownKeys().contains(k)) out << k;
    return out;
}
