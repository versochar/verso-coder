#pragma once
#include <QJSEngine>
#include <QMap>
#include <QObject>
#include <QString>

// Stage 18: eklenti API v1 (JS betikleri, QJSEngine).
// Betik ortamı: verso.{log,registerCommand,readFile,writeFile}
// İzinler: "fs.read" "fs.write" — izinsiz erişim engellenir + sinyal yayılır.
class VersoApi : public QObject {
    Q_OBJECT
public:
    VersoApi(const QString& pluginId, const QStringList& perms, QObject* parent = nullptr)
        : QObject(parent), m_id(pluginId), m_perms(perms) {}

    Q_INVOKABLE void log(const QString& msg);
    Q_INVOKABLE void registerCommand(const QString& cmdId, const QString& title,
                                     const QJSValue& fn);
    Q_INVOKABLE QString readFile(const QString& path);
    Q_INVOKABLE bool writeFile(const QString& path, const QString& text);

signals:
    void apiLog(const QString& id, const QString& msg);
    void apiRegister(const QString& id, const QString& cmdId, const QString& title,
                     const QJSValue& fn);
    void apiDenied(const QString& id, const QString& perm);

private:
    QString m_id;
    QStringList m_perms;
};

class PluginEngine : public QObject {
    Q_OBJECT
public:
    explicit PluginEngine(QObject* parent = nullptr);

    struct Plugin {
        QString id;   // dosya adı (uzantısız)
        QString path; // .js tam yolu
        QStringList permissions;
        QStringList commands; // kaydettiği komutlar
        QString error;
        bool loaded = false;
    };

    // Dizin tarama (*.js) + yükleme (izinler manifestten: // @permission fs.write)
    QList<Plugin> loadAll(const QString& dir);
    bool unload(const QString& id);
    bool isLoaded(const QString& id) const;
    QList<Plugin> plugins() const { return m_plugins; }
    QString pluginDir() const { return m_dir; }

    // Kayıtlı betik komutunu çalıştır
    QJSValue callCommand(const QString& cmdId, const QString& arg = QString());

signals:
    void commandRegistered(const QString& cmdId, const QString& title);
    void pluginLog(const QString& id, const QString& msg);
    void permissionDenied(const QString& id, const QString& perm);

private slots:
    void onApiRegister(const QString& id, const QString& cmdId, const QString& title,
                       const QJSValue& fn);

private:
    static QStringList scanPermissions(const QString& source);
    QJSValue makeVerso(const QString& id, const QStringList& perms);

    QJSEngine m_js;
    QList<Plugin> m_plugins;
    QString m_dir;
    QMap<QString, QJSValue> m_commands; // cmdId → fonksiyon
    QMap<QString, QString> m_cmdOwner;  // cmdId → plugin id
    QMap<QString, VersoApi*> m_apis;    // plugin id → canlı API nesnesi
};
