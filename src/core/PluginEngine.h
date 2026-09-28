#pragma once
#include <QJSEngine>
#include <QMap>
#include <QObject>
#include <QTimer>
#include <functional>
#include <QString>

// Stage 18: eklenti API v1 (JS betikleri, QJSEngine).
// Stage 29 v2: durum çubuğu, quickPick/inputBox, olaylar, depolama,
//   terminal, tanılama, görünüm, tema, tuş, ayar, exec katkıları.
// İzinler: "fs.read" "fs.write" "events" "ui" — izinsiz erişim engellenir.
class PluginEngine; // aşağıda

class VersoApi : public QObject {
    Q_OBJECT
public:
    VersoApi(const QString& pluginId, const QStringList& perms, QObject* parent = nullptr)
        : QObject(parent), m_id(pluginId), m_perms(perms) {}
    // Stage 41: dosya erişimi çalışma alanı köküne kapsanır. Motor canlı
    // kökü verir (proje değişince eski kök kullanılmaz).
    const PluginEngine* engine() const;

    // v1
    Q_INVOKABLE void log(const QString& msg);
    Q_INVOKABLE void registerCommand(const QString& cmdId, const QString& title,
                                     const QJSValue& fn);
    Q_INVOKABLE QString readFile(const QString& path);
    Q_INVOKABLE bool writeFile(const QString& path, const QString& text);
    // v2: olaylar
    Q_INVOKABLE void onEvent(const QString& name, const QJSValue& fn); // save|open|startup|language
    // v2: arayüz ("ui" izni)
    Q_INVOKABLE void showStatus(const QString& text, int timeoutMs = 0);
    Q_INVOKABLE QString quickPick(const QJSValue& items,
                                    const QString& placeholder = QString());
    // Başsız test için: dizi/JSON/satırlı metni listeye çevirir
    static QStringList pickItems(const QJSValue& v);
    Q_INVOKABLE QString inputBox(const QString& prompt, const QString& def = QString());
    Q_INVOKABLE void sendTerminal(const QString& text);
    Q_INVOKABLE QString fetch(const QString& url, int timeoutMs = 10000); // Stage 30: "net" izni
    Q_INVOKABLE void reportProblems(const QString& json); // [{file,line,message}]
    Q_INVOKABLE void registerView(const QString& viewId, const QString& title,
                                  const QJSValue& fn); // fn() → HTML
    Q_INVOKABLE bool registerTheme(const QString& name, const QString& json);
    Q_INVOKABLE void registerKeybinding(const QString& cmdId, const QString& keys);
    Q_INVOKABLE void execCommand(const QString& cmdId);
    Q_INVOKABLE QString currentFile(); // Stage 41: açık dosya yolu (okuma ayrı izin)
    // v2: depolama (izin gerekmez, eklentiye özel alan)
    Q_INVOKABLE QString getGlobalState(const QString& key);
    Q_INVOKABLE void setGlobalState(const QString& key, const QString& value);
    Q_INVOKABLE QString getWorkspaceState(const QString& key);
    Q_INVOKABLE void setWorkspaceState(const QString& key, const QString& value);
    Q_INVOKABLE QString getConfig(const QString& key, const QString& def = QString());
    Q_INVOKABLE void setConfig(const QString& key, const QString& value);
    // Zamanlayıcı (izin gerekmez; hata sayacı karantinaya işletir)
    Q_INVOKABLE int setTimeout(const QJSValue& fn, int ms);
    Q_INVOKABLE int setInterval(const QJSValue& fn, int ms);
    Q_INVOKABLE void clearTimer(int id);
    // Pano ("ui" izni; başsız ortamda sessizce boş)
    Q_INVOKABLE bool copyText(const QString& text);
    Q_INVOKABLE QString pasteText();
    // Dizin listeleme ("fs.read" izni, kök kapsamlı; JSON dizi döner)
    Q_INVOKABLE QString listDir(const QString& relPath);

signals:
    void apiLog(const QString& id, const QString& msg);
    void apiRegister(const QString& id, const QString& cmdId, const QString& title,
                     const QJSValue& fn);
    void apiDenied(const QString& id, const QString& perm);
    // v2 sinyalleri
    void apiEventHandler(const QString& id, const QString& event, const QJSValue& fn);
    void apiStatus(const QString& id, const QString& text, int timeoutMs);
    void apiTerminal(const QString& id, const QString& text);
    void apiProblems(const QString& id, const QString& json);
    void apiView(const QString& id, const QString& viewId, const QString& title,
                 const QJSValue& fn);
    void apiTheme(const QString& id, const QString& name, const QString& json);
    void apiKeybinding(const QString& id, const QString& cmdId, const QString& keys);
    void apiExec(const QString& id, const QString& cmdId);

private:
    bool need(const QString& perm);
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
        QString name; // // @name (yoksa id)
        QString version; // // @version
        QStringList permissions;
        QStringList commands; // kaydettiği komutlar
        QString error;
        bool loaded = false;
        bool quarantined = false;
        int loadMs = 0; // Stage 31: yükleme süresi
    };

    // Dizin tarama (*.js) + yükleme (başlık manifesti okunur)
    QList<Plugin> loadAll(const QString& dir);
    bool unload(const QString& id);
    bool isLoaded(const QString& id) const;
    QList<Plugin> plugins() const { return m_plugins; }
    QString pluginDir() const { return m_dir; }
    void setWorkspaceRoot(const QString& root) { m_wsRoot = root; }
    QString workspaceRoot() const { return m_wsRoot; }
    // Stage 41: açık dosya sağlayıcısı (MainWindow kurar)
    void setCurrentFileProvider(std::function<QString()> fn) { m_currentFile = fn; }
    QString currentFilePath() const { return m_currentFile ? m_currentFile() : QString(); }

    // Kayıtlı betik komutunu çalıştır (hata sayaçlı — 3 hatada karantina)
    QJSValue callCommand(const QString& cmdId, const QString& arg = QString());
    // Olay dağıtımı: save|open|startup|language (+arg: yol ya da "")
    void fireEvent(const QString& event, const QString& arg = QString());
    // Görünüm içeriğini üret (view komutları için)
    QString renderView(const QString& cmdId);
    // İzin geçersiz kılma (yönetici UI): id → izin listesi (boş = tümü reddet)
    // Zamanlayıcılar (motor sahiplenir; unload/loadAll temizler)
    int startTimer(const QString& pluginId, const QJSValue& fn, int ms, bool repeat);
    void stopTimer(const QString& pluginId, int id);
    void clearTimers(const QString& pluginId); // boş = tümü
    void setPermOverride(const QString& id, const QStringList& perms);
    QStringList permOverride(const QString& id) const;
    void clearQuarantine(const QString& id);
    QStringList logLines() const { return m_log; }

signals:
    void commandRegistered(const QString& cmdId, const QString& title);
    void pluginLog(const QString& id, const QString& msg);
    void permissionDenied(const QString& id, const QString& perm);
    void viewRegistered(const QString& cmdId, const QString& title);
    void themeRegistered(const QString& id, const QString& name, const QString& json);
    void keybindingRegistered(const QString& id, const QString& cmdId, const QString& keys);
    void execRequested(const QString& id, const QString& cmdId);
    void statusRequested(const QString& id, const QString& text, int timeoutMs);
    void terminalRequested(const QString& id, const QString& text);
    void problemsReported(const QString& id, const QString& json);
    void pluginQuarantined(const QString& id, const QString& reason);

private slots:
    void onApiRegister(const QString& id, const QString& cmdId, const QString& title,
                       const QJSValue& fn);

private:
    static QStringList scanPermissions(const QString& source);
    static QString scanHeader(const QString& source, const QString& key);
    static QString sealKey(const QString& id);
    QJSValue makeVerso(const QString& id, const QStringList& perms);
    void noteError(const QString& pluginId, const QString& msg);

    QJSEngine m_js;
    QList<Plugin> m_plugins;
    QString m_dir;
    QString m_wsRoot;
    QMap<QString, QJSValue> m_commands; // cmdId → fonksiyon
    QMap<QString, QString> m_cmdOwner;  // cmdId → plugin id
    QMap<QString, VersoApi*> m_apis;    // plugin id → canlı API nesnesi
    QMap<QString, QMap<QString, QJSValue>> m_handlers; // plugin → olay → fn
    QMap<QString, QJSValue> m_views;    // view cmdId → fn
    QMap<QString, QString> m_viewOwner;
    QMap<QString, int> m_errors;        // plugin → hata sayacı
    std::function<QString()> m_currentFile; // Stage 41
    QMap<int, QTimer*> m_timers;        // tanıtıcı → zamanlayıcı
    QMap<int, QString> m_timerOwner;    // tanıtıcı → plugin id
    QMap<QString, QJSValue> m_timerFns; // "pid#hid" → fonksiyon (canlı tutar)
    int m_timerSeq = 0;
    QStringList m_log;                  // son 200 günlük satırı
    void addLog(const QString& id, const QString& msg);
};
