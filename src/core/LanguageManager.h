#pragma once
#include <QApplication>
#include <QMap>
#include <QString>

// Basit TR/EN sözlük. QTranslator .qm dosyasına gerek kalmadan çalışır.
class LanguageManager {
public:
    static LanguageManager& instance();
    void setLanguage(const QString& lang); // "tr" | "en"
    QString lang() const { return m_lang; }
    QString t(const QString& key) const;   // çeviri al
    // Stage 22: kapsama denetimi — tüm anahtarlar + dil varlığı
    QStringList allKeys() const { return m_dict.keys(); }
    bool hasTranslation(const QString& key, const QString& lang) const;
private:
    LanguageManager();
    QString m_lang = "tr";
    QMap<QString, QMap<QString, QString>> m_dict;
};
