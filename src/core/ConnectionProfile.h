#pragma once
#include <QMap>
#include <QString>
#include <QStringList>

// Stage 16: uzak bağlantı profili — host/port/kullanıcı/anahtar/jump/uzak kök.
// QSettings'te "remoteProfiles/<ad>" altında saklanır.
struct ConnectionProfile {
    QString name;          // görünen ad (QSettings anahtarı için temizlenir)
    QString host;          // örn. "192.168.1.10" ya da "sunucu.ornek.com"
    int port = 22;
    QString user;          // boşsa yerel kullanıcı
    QString keyPath;       // özel anahtar (boşsa agent/varsayılan)
    QString jumpHost;      // "kullanici@ara-host:22" (boşsa yok)
    QString remoteRoot;    // uzak çalışma kökü, örn. "/home/ali/proje"
    QString remoteCmd;     // uzak shell (boşsa varsayılan)
    bool trustNewHosts = false; // ilk bağlanışta bilinmeyen host anahtarına güven (TOFU)

    bool isValid() const { return !host.trimmed().isEmpty(); }
    // ssh://[user@]host[:port]/path biçimi
    QString toUri(const QString& path = QString()) const;
    QString display() const; // "ad (kullanıcı@host:port)"
    static QString sanitize(const QString& raw);
};

class ConnectionProfiles {
public:
    static QMap<QString, ConnectionProfile> all();
    static bool save(const ConnectionProfile& p);
    static bool remove(const QString& name);
    static bool exists(const QString& name);
    static QStringList names();
    static ConnectionProfile get(const QString& name);
};
