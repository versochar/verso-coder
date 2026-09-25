#pragma once
#include <QMap>
#include <QString>
#include <QStringList>

// Stage 17: snippet deposu — dile göre gömülü snippet'ler + QSettings özel seti.
struct SnippetDef {
    QString prefix; // tetikleyici önek ("for")
    QString name;   // görünen ad ("for döngüsü")
    QString body;   // gövde ($1 ${2:x} $0 ...)
    QString lang;   // "cpp" | "py" | "*" (tümü)
};

class SnippetManager {
public:
    static QList<SnippetDef> builtin(); // gömülü set
    static QList<SnippetDef> custom();  // QSettings "snippets/<lang>/<prefix>"
    static QList<SnippetDef> forLang(const QString& suffix); // builtin + custom
    static bool saveCustom(const SnippetDef& s);
    static bool removeCustom(const QString& lang, const QString& prefix);
    // Önekle başlayanları puanla (önek eşleşme > içerme), en iyi önce
    static QList<SnippetDef> match(const QString& word, const QString& suffix);
    static QString langFor(const QString& suffix);
};
