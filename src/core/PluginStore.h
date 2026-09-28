#pragma once
#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

// Eklenti mağazası istemcisi: resmi kayıt deposu bir git reposudur
// (verso-coder-plugins). Kökteki index.json listeler, plugins/<id>/plugin.js
// tek dosyalık eklentiyi verir. Ağ yoksa kurulum çalışmaz, hata döner.
class PluginStore {
public:
    struct Entry {
        QString id;          // dosya adı (uzantısız) + dizin adı
        QString name;        // görünen ad
        QString version;     // X.Y.Z
        QString description; // tek satır
        QString author;
        QString file;        // depodaki yol: plugins/<id>/plugin.js
        QString minApp;      // en düşük uygulama sürümü (boş = yok)
        QStringList permissions;
        bool valid() const { return !id.isEmpty() && !file.isEmpty() && !version.isEmpty(); }
    };

    static QString defaultRegistry();
    // Sıra: VERSO_PLUGIN_REGISTRY ortam değişkeni > plugin/registry ayarı > varsayılan
    static QString registryBase();
    static QString indexUrl();
    static QString pluginUrl(const Entry& e);
    // Zaman aşımlı eşzamanlı GET (http(s) ve file://). Hata boş değilse başarısız.
    static QByteArray fetchUrl(const QString& url, QString* error, int timeoutMs = 15000);
    static QList<Entry> parseIndex(const QByteArray& json, QString* error);
    static QList<Entry> fetchIndex(QString* error);
    // Kaynağı <dir>/<id>.js olarak yazar + etkin listesine ekler.
    static bool install(const Entry& e, const QString& source, const QString& dir,
                        QString* error);
    static QString installedVersion(const QString& dir, const QString& id);
    static bool isValidId(const QString& id);
    // Kurulup yenisi kayıtta olanlar (toplu güncelleme için)
    static QList<Entry> updatesAvailable(const QList<Entry>& remote, const QString& dir);
    // Çevrimdışı önbellek (son başarılı liste + tarihi)
    static void saveCache(const QList<Entry>& entries);
    static QList<Entry> loadCache(QDateTime* updated = nullptr);
};
