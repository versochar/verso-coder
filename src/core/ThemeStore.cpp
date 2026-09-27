#include "ThemeStore.h"
#include "VersoPaths.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>

// yardımcı: dosyadaki "name" alanı (dosya adıyla eşleşmeli)
static QString readThemeName(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return QString();
    return QJsonDocument::fromJson(f.readAll()).object().value("name").toString();
}

ThemeStore& ThemeStore::instance() {
    static ThemeStore s;
    return s;
}

QString ThemeStore::builtinThemesDir() {
    // Stage 50-debug: kurulu paket (/usr/share) dahil tüm adaylar
    return VersoPaths::subDir("themes");
}

QString ThemeStore::customThemesDir() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/themes";
}

QString ThemeStore::findBuiltinFile(const QString& name) {
    const QString p = QDir(builtinThemesDir()).absoluteFilePath(name + ".json");
    return QFile::exists(p) ? p : QString();
}

QString ThemeStore::customFile(const QString& name) {
    return QDir(customThemesDir()).absoluteFilePath(name + ".json");
}

QStringList ThemeStore::builtinNames() const {
    QStringList out;
    QDir d(builtinThemesDir());
    for (const QFileInfo& f : d.entryInfoList(QStringList{"*.json"}, QDir::Files, QDir::Name))
        if (isValidTheme(f.baseName()) && f.baseName() == readThemeName(f.absoluteFilePath()))
            out << f.baseName();
    if (out.isEmpty()) out << "dark" << "light"; // kaynak ağacı yoksa güvenli çekirdek
    return out;
}

QStringList ThemeStore::customNames() const {
    QStringList out;
    QDir d(customThemesDir());
    if (!d.exists()) return out;
    for (const QFileInfo& f : d.entryInfoList(QStringList{"*.json"}, QDir::Files, QDir::Name))
        if (isValidTheme(f.baseName()) && f.baseName() == readThemeName(f.absoluteFilePath()))
            out << f.baseName();
    return out;
}

QStringList ThemeStore::themeNames() const {
    QStringList out = builtinNames();
    for (const QString& c : customNames())
        if (!out.contains(c)) out << c;
    return out;
}

bool ThemeStore::hasTheme(const QString& name) const {
    return !findBuiltinFile(name).isEmpty() || QFile::exists(customFile(name));
}

ThemeTokens ThemeStore::theme(const QString& name) const {
    QString path = findBuiltinFile(name);
    if (path.isEmpty() && QFile::exists(customFile(name))) path = customFile(name);
    if (path.isEmpty()) return ThemeTokens::defaults(true);
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return ThemeTokens::defaults(true);
    ThemeTokens t = ThemeTokens::fromJson(QString::fromUtf8(f.readAll()));
    t.name = name; // dosya adı yetkilidir
    return t.isValid() ? t : ThemeTokens::defaults(true);
}

bool ThemeStore::isValidTheme(const QString& name) const {
    QString path = findBuiltinFile(name);
    if (path.isEmpty() && QFile::exists(customFile(name))) path = customFile(name);
    if (path.isEmpty()) return false;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    return ThemeTokens::fromJson(QString::fromUtf8(f.readAll())).isValid();
}

bool ThemeStore::importTheme(const QString& srcJsonPath, QString* importedName) {
    QFile f(srcJsonPath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QString json = QString::fromUtf8(f.readAll());
    ThemeTokens t = ThemeTokens::fromJson(json);
    if (!t.isValid()) return false;
    // güvenli dosya adı: temanın görünen adından anahtar türet
    QString key = t.name.toLower();
    key.replace(QRegularExpression("[^a-z0-9\\-\\+]"), "-");
    if (key.isEmpty()) key = "custom";
    QDir().mkpath(customThemesDir());
    QFile dst(customFile(key));
    if (!dst.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    dst.write(json.toUtf8());
    if (importedName) *importedName = key;
    return true;
}

bool ThemeStore::exportTheme(const QString& name, const QString& dstPath) const {
    const ThemeTokens t = theme(name);
    if (name.isEmpty() || !t.isValid()) return false;
    QFile f(dstPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(t.toJsonString().toUtf8());
    return true;
}
