#pragma once
#include <QMap>
#include <QString>
#include <QStringList>

// Stage 15: özel istem kitaplığı (QSettings kalıcı: ad → istem).
class PromptLibrary {
public:
    static QMap<QString, QString> all();
    static bool save(const QString& name, const QString& prompt);
    static bool remove(const QString& name);
    static bool exists(const QString& name);
    static QStringList names();
    static QString sanitize(const QString& raw);
};
