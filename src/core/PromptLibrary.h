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

    // Stage 37: istem başındaki "!görev" yönlendirmesini çözer.
    // "!test Bu fonksiyonu test et" → görev test, gövde "Bu fonksiyonu test et"
    static bool hasTaskHint(const QString& raw);
    static QString taskHint(const QString& raw);   // "test" | "" (yoksa boş)
    static QString stripTaskHint(const QString& raw);
    static QStringList knownTaskHints();
    static bool isKnownTaskHint(const QString& h);
};
