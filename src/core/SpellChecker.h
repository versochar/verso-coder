#pragma once
#include <QStringList>

// Hunspell sarmalayıcı: yorum/string içindeki kelimeleri denetler (TR + EN).
class SpellChecker {
public:
    SpellChecker();
    ~SpellChecker();
    SpellChecker(const SpellChecker&) = delete;
    SpellChecker& operator=(const SpellChecker&) = delete;

    bool load(const QString& lang); // örn. "tr_TR", "en_US"
    bool isOk() const { return m_ok; }
    QString lang() const { return m_lang; }
    bool check(const QString& word);
    QStringList suggest(const QString& word, int max = 5);

    static QStringList dictionaryDirs();
    static QString findDict(const QString& lang); // "lang" için .dic yolu ("" yoksa)

private:
    void* m_h = nullptr; // Hunspell*
    bool m_ok = false;
    QString m_lang;
};
