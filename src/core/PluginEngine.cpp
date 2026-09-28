#include "PluginEngine.h"
#include "PathGuard.h"
#include <QApplication>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QEventLoop>
#include <QInputDialog>
#include <QJSEngine>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QRegularExpression>
#include <QSettings>
#include <QWidget>

PluginEngine::PluginEngine(QObject* parent) : QObject(parent) {}

// ---- bildirim başlığı: // @permission <ad> (virgüllü çoklu izin de olur) ---
QStringList PluginEngine::scanPermissions(const QString& source) {
    QStringList out;
    static QRegularExpression re(R"(^\s*//\s*@permission\s+(.+)$)",
                                 QRegularExpression::MultilineOption);
    auto it = re.globalMatch(source);
    static const QStringList known = {"fs.read", "fs.write", "events", "ui", "net"};
    while (it.hasNext()) {
        for (const QString& tok :
             it.next().captured(1).split(QRegularExpression(R"([\s,]+)"),
                                         Qt::SkipEmptyParts)) {
            if (known.contains(tok) && !out.contains(tok)) {
            out << tok;
        } else if (!known.contains(tok)) {
            out << QString("?") + tok; // bilinmeyen: yükleyici uyarır
        }
        }
    }
    return out;
}

QString PluginEngine::scanHeader(const QString& source, const QString& key) {
    static QRegularExpression re(R"(^\s*//\s*@(\w+)\s+(.+)$)",
                                 QRegularExpression::MultilineOption);
    auto it = re.globalMatch(source);
    while (it.hasNext()) {
        auto m = it.next();
        if (m.captured(1) == key) return m.captured(2).trimmed().left(120);
    }
    return {};
}

QString PluginEngine::sealKey(const QString& id) {
    return "plugin/seal/" + id;
}

void PluginEngine::addLog(const QString& id, const QString& msg) {
    m_log << QString("[%1] %2").arg(id, msg.left(300));
    while (m_log.size() > 200) m_log.removeFirst();
}

// ---- VersoApi v1 ----
void VersoApi::log(const QString& msg) { emit apiLog(m_id, msg.left(500)); }

void VersoApi::registerCommand(const QString& cmdId, const QString& title,
                               const QJSValue& fn) {
    if (!fn.isCallable() || cmdId.trimmed().isEmpty()) return;
    emit apiRegister(m_id, cmdId.trimmed(), title, fn);
}

// Stage 41: eklenti dosya erişimi PathGuard ile çalışma alanına kapsanır.
// İzin olsa bile kök dışı okuma/yazma reddedilir (ajanla aynı kural).
static QString scopedPath(const PluginEngine* eng, const QString& in, QString* why) {
    if (!eng) {
        if (why) *why = QStringLiteral("motor yok");
        return {};
    }
    const QString root = eng->workspaceRoot();
    if (root.isEmpty()) {
        if (why) *why = QStringLiteral("çalışma alanı açık değil");
        return {};
    }
    QString abs, reason;
    PathGuard g(root);
    if (!g.resolve(in, abs, &reason)) {
        if (why) *why = reason;
        return {};
    }
    return abs;
}

QString VersoApi::readFile(const QString& path) {
    if (!need("fs.read")) return {};
    QString why;
    const QString abs = scopedPath(engine(), path, &why);
    if (abs.isEmpty()) {
        emit apiLog(m_id, QString("okuma reddedildi (%1): %2").arg(why, path.left(120)));
        return {};
    }
    QFile f(abs);
    if (!f.open(QIODevice::ReadOnly)) return {};
    return QString::fromUtf8(f.read(1 << 20));
}

bool VersoApi::writeFile(const QString& path, const QString& text) {
    if (!need("fs.write")) return false;
    QString why;
    const QString abs = scopedPath(engine(), path, &why);
    if (abs.isEmpty()) {
        emit apiLog(m_id, QString("yazma reddedildi (%1): %2").arg(why, path.left(120)));
        return false;
    }
    QFile f(abs);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(text.toUtf8());
    return true;
}

const PluginEngine* VersoApi::engine() const {
    return qobject_cast<const PluginEngine*>(parent());
}

bool VersoApi::need(const QString& perm) {
    if (m_perms.contains(perm)) return true;
    emit apiDenied(m_id, perm);
    return false;
}

// ---- VersoApi v2 ----
void VersoApi::onEvent(const QString& name, const QJSValue& fn) {
    static const QStringList evs = {"save", "open", "startup", "language"};
    if (!need("events") || !fn.isCallable() || !evs.contains(name.trimmed())) return;
    emit apiEventHandler(m_id, name.trimmed(), fn);
}

void VersoApi::showStatus(const QString& text, int timeoutMs) {
    if (!need("ui")) return;
    emit apiStatus(m_id, text.left(160), qBound(0, timeoutMs, 10000));
}

QStringList VersoApi::pickItems(const QJSValue& itemsV) {
    QStringList items;
    if (itemsV.isArray()) {
        // JS dizisi: virgülle yapıştırma, öğe öğe al
        const quint32 n = itemsV.property("length").toUInt();
        for (quint32 i = 0; i < n; i++) {
            const QString s = itemsV.property(i).toString();
            if (!s.isEmpty()) items << s;
        }
        return items;
    }
    const QString itemsJson = itemsV.toString();
    const QJsonDocument d = QJsonDocument::fromJson(itemsJson.toUtf8());
    if (d.isArray())
        for (const QJsonValue& v : d.array()) {
            const QString s = v.isObject() ? v.toObject()["label"].toString()
                                           : v.toString();
            if (!s.isEmpty()) items << s;
        }
    else if (!itemsJson.trimmed().isEmpty()) {
        items = itemsJson.split('\n', Qt::SkipEmptyParts);
    }
    return items;
}

QString VersoApi::quickPick(const QJSValue& itemsV,
                              const QString& placeholder) {
    if (!need("ui")) return {};
    const QStringList items = pickItems(itemsV);
    if (items.isEmpty()) return {};
    if (!qobject_cast<QApplication*>(QCoreApplication::instance())) return {};
    bool ok = false;
    QWidget* parent = qobject_cast<QWidget*>(this->parent());
    while (parent && parent->parentWidget()) parent = parent->parentWidget();
    const QString sel = QInputDialog::getItem(parent, "Eklenti", placeholder, items, 0,
                                              false, &ok);
    return ok ? sel : QString();
}

QString VersoApi::inputBox(const QString& prompt, const QString& def) {
    if (!need("ui")) return {};
    if (!qobject_cast<QApplication*>(QCoreApplication::instance())) return {};
    bool ok = false;
    QWidget* parent = qobject_cast<QWidget*>(this->parent());
    while (parent && parent->parentWidget()) parent = parent->parentWidget();
    const QString v = QInputDialog::getText(parent, "Eklenti", prompt, QLineEdit::Normal,
                                            def, &ok);
    return ok ? v : QString();
}

void VersoApi::sendTerminal(const QString& text) {
    if (!need("ui") || text.trimmed().isEmpty()) return;
    emit apiTerminal(m_id, text);
}

// Stage 30: HTTP GET (eşzamanlı, zaman aşımlı) — "net" izni gerekir
QString VersoApi::fetch(const QString& url, int timeoutMs) {
    if (!need("net")) return {};
    const QUrl u(url.trimmed());
    if (!u.isValid() || (u.scheme() != "http" && u.scheme() != "https")) return {};
    QNetworkAccessManager net;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QString out;
    QNetworkReply* r = net.get(QNetworkRequest(u));
    QObject::connect(r, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(qBound(1000, timeoutMs, 60000));
    loop.exec();
    if (r->error() == QNetworkReply::NoError)
        out = QString::fromUtf8(r->readAll()).left(1000000);
    r->deleteLater();
    return out;
}

void VersoApi::reportProblems(const QString& json) {
    if (!need("ui")) return;
    emit apiProblems(m_id, json.left(20000));
}

void VersoApi::registerView(const QString& viewId, const QString& title,
                            const QJSValue& fn) {
    if (!need("ui") || !fn.isCallable() || viewId.trimmed().isEmpty()) return;
    emit apiView(m_id, viewId.trimmed(), title, fn);
}

bool VersoApi::registerTheme(const QString& name, const QString& json) {
    if (!need("ui") || name.trimmed().isEmpty()) return false;
    emit apiTheme(m_id, name.trimmed(), json);
    return true;
}

void VersoApi::registerKeybinding(const QString& cmdId, const QString& keys) {
    if (!need("ui") || cmdId.trimmed().isEmpty() || keys.trimmed().isEmpty()) return;
    emit apiKeybinding(m_id, cmdId.trimmed(), keys.trimmed());
}

void VersoApi::execCommand(const QString& cmdId) {
    if (!need("ui") || cmdId.trimmed().isEmpty()) return;
    emit apiExec(m_id, cmdId.trimmed());
}

static QString stateGroup(const QString& id, bool ws, const QString& root) {
    QString g = "plugin/state/" + id;
    if (ws) g += "@" + QString::fromLatin1(
        QCryptographicHash::hash(root.toUtf8(), QCryptographicHash::Sha1).toHex().left(10));
    return g;
}

QString VersoApi::getGlobalState(const QString& key) {
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(stateGroup(m_id, false, QString()));
    return q.value(key).toString();
}

void VersoApi::setGlobalState(const QString& key, const QString& value) {
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(stateGroup(m_id, false, QString()));
    q.setValue(key, value);
}

QString VersoApi::getWorkspaceState(const QString& key) {
    auto* eng = qobject_cast<PluginEngine*>(this->parent());
    const QString root = eng ? eng->workspaceRoot() : QString();
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(stateGroup(m_id, true, root));
    return q.value(key).toString();
}

void VersoApi::setWorkspaceState(const QString& key, const QString& value) {
    auto* eng = qobject_cast<PluginEngine*>(this->parent());
    const QString root = eng ? eng->workspaceRoot() : QString();
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(stateGroup(m_id, true, root));
    q.setValue(key, value);
}

QString VersoApi::getConfig(const QString& key, const QString& def) {
    QSettings q("Verso", "VersoCoder");
    q.beginGroup("plugin/config/" + m_id);
    const QString v = q.value(key, def).toString();
    q.endGroup();
    return v;
}

void VersoApi::setConfig(const QString& key, const QString& value) {
    QSettings q("Verso", "VersoCoder");
    q.beginGroup("plugin/config/" + m_id);
    q.setValue(key, value);
    q.endGroup();
}

// ---- PluginEngine ----
QJSValue PluginEngine::makeVerso(const QString& id, const QStringList& perms) {
    auto* api = new VersoApi(id, perms, this);
    m_apis[id] = api;
    connect(api, &VersoApi::apiLog, this, [this](const QString& pid, const QString& msg) {
        addLog(pid, msg);
        emit pluginLog(pid, msg);
    });
    connect(api, &VersoApi::apiDenied, this,
            [this](const QString& pid, const QString& perm) { emit permissionDenied(pid, perm); });
    connect(api, &VersoApi::apiRegister, this, &PluginEngine::onApiRegister);
    connect(api, &VersoApi::apiEventHandler, this,
            [this](const QString& pid, const QString& ev, const QJSValue& fn) {
                m_handlers[pid][ev] = fn;
            });
    connect(api, &VersoApi::apiStatus, this,
            [this](const QString& pid, const QString& t, int ms) {
                emit statusRequested(pid, t, ms);
            });
    connect(api, &VersoApi::apiTerminal, this,
            [this](const QString& pid, const QString& t) { emit terminalRequested(pid, t); });
    connect(api, &VersoApi::apiProblems, this,
            [this](const QString& pid, const QString& j) { emit problemsReported(pid, j); });
    connect(api, &VersoApi::apiView, this,
            [this](const QString& pid, const QString& vid, const QString& title,
                   const QJSValue& fn) {
                const QString full = "plugin." + pid + ".view." + vid;
                m_views[full] = fn;
                m_viewOwner[full] = pid;
                emit viewRegistered(full, title);
            });
    connect(api, &VersoApi::apiTheme, this,
            [this](const QString& pid, const QString& n, const QString& j) {
                emit themeRegistered(pid, n, j);
            });
    connect(api, &VersoApi::apiExec, this,
            [this](const QString& pid, const QString& cmd) {
                if (!cmd.isEmpty()) emit execRequested(pid, cmd);
            });
    connect(api, &VersoApi::apiKeybinding, this,
            [this](const QString& pid, const QString& cmd, const QString& keys) {
                emit keybindingRegistered(pid, cmd, keys);
            });
    return m_js.newQObject(api);
}

void PluginEngine::onApiRegister(const QString& id, const QString& cmdId,
                                 const QString& title, const QJSValue& fn) {
    const QString full = "plugin." + id + "." + cmdId;
    m_commands[full] = fn;
    m_cmdOwner[full] = id;
    for (Plugin& pl : m_plugins)
        if (pl.id == id && !pl.commands.contains(full)) pl.commands << full;
    emit commandRegistered(full, title);
}

void PluginEngine::noteError(const QString& pluginId, const QString& msg) {
    const int n = m_errors.value(pluginId, 0) + 1;
    m_errors[pluginId] = n;
    addLog(pluginId, "Hata: " + msg.left(200));
    emit pluginLog(pluginId, "Hata: " + msg.left(200));
    // Stage 29: 3 hatada karantina
    if (n >= 3 && !pluginId.isEmpty()) {
        for (Plugin& pl : m_plugins) {
            if (pl.id != pluginId || pl.quarantined) continue;
            pl.quarantined = true;
            QSettings q("Verso", "VersoCoder");
            QStringList ql = q.value("plugin/quarantine").toStringList();
            if (!ql.contains(pluginId)) {
                ql << pluginId;
                q.setValue("plugin/quarantine", ql);
            }
            emit pluginQuarantined(pluginId, QString("%1 hata").arg(n));
            unload(pluginId);
            break;
        }
    }
}

QList<PluginEngine::Plugin> PluginEngine::loadAll(const QString& dir) {
    m_plugins.clear();
    m_commands.clear();
    m_cmdOwner.clear();
    m_views.clear();
    m_viewOwner.clear();
    m_handlers.clear();
    qDeleteAll(m_apis);
    m_apis.clear();
    m_dir = dir;
    QDir d(dir);
    if (!d.exists()) return m_plugins;
    const QStringList quar =
        QSettings("Verso", "VersoCoder").value("plugin/quarantine").toStringList();
    const bool enabledSet =
        QSettings("Verso", "VersoCoder").contains("plugin/enabled");
    const QStringList enabled =
        QSettings("Verso", "VersoCoder").value("plugin/enabled").toStringList();
    for (const QFileInfo& fi : d.entryInfoList({"*.js"}, QDir::Files, QDir::Name)) {
        Plugin p;
        p.id = fi.completeBaseName();
        p.path = fi.absoluteFilePath();
        if (quar.contains(p.id)) {
            p.error = "Karantinada (hata geçmişi).";
            p.quarantined = true;
            m_plugins << p;
            continue;
        }
        if (enabledSet && !enabled.contains(p.id)) {
            p.error = "Devre dışı (yönetici).";
            m_plugins << p;
            continue;
        }
        QFile f(p.path);
        if (!f.open(QIODevice::ReadOnly)) {
            p.error = "Okunamadı.";
            m_plugins << p;
            continue;
        }
        const QString src = QString::fromUtf8(f.readAll());
        // Stage 29: bütünlük mührü (ilk yüklemede mühürle, değişirse uyar)
        const QString seal = QString::fromLatin1(
            QCryptographicHash::hash(src.toUtf8(), QCryptographicHash::Sha256).toHex());
        QSettings q("Verso", "VersoCoder");
        const QString old = q.value(sealKey(p.id)).toString();
        if (old.isEmpty()) {
            q.setValue(sealKey(p.id), seal);
        } else if (old != seal) {
            addLog(p.id, "UYARI: dosya ilk yüklemeden sonra değişmiş (mühür uyuşmuyor).");
            emit pluginLog(p.id, "UYARI: dosya değişmiş olabilir.");
        }
        p.name = scanHeader(src, "name");
        if (p.name.isEmpty()) p.name = p.id;
        p.version = scanHeader(src, "version");
        QStringList perms = scanPermissions(src);
        // Stage 41: bilinmeyen izin bildirimi (sessiz yutma yok)
        QStringList unknown;
        for (const QString& tok : perms)
            if (tok.startsWith('?')) unknown << tok.mid(1);
        for (const QString& tok : unknown) perms.removeOne(QString("?") + tok);
        if (!unknown.isEmpty())
            addLog(p.id, QString("bilinmeyen izinler yoksayıldı: %1").arg(unknown.join(", ")));
        // Yönetici geçersiz kılma: listede yoksa reddet
        const QString okey = "plugin/perms/" + p.id;
        if (q.contains(okey)) perms = q.value(okey).toStringList();
        p.permissions = perms;
        QJSValue verso = makeVerso(p.id, perms);
        m_js.globalObject().setProperty("verso", verso);
        m_plugins << p; // değerlendirmeden ÖNCE: register/noteError listede bulsun
        Plugin& slot = m_plugins.last();
        QElapsedTimer tmr; // Stage 31: yavaş eklenti ölçümü
        tmr.start();
        QJSValue res = m_js.evaluate(src, p.path);
        slot.loadMs = int(tmr.elapsed());
        if (res.isError()) {
            slot.error = QString("Satır %1: %2")
                             .arg(res.property("lineNumber").toInt())
                             .arg(res.toString().left(200));
            noteError(slot.id, slot.error);
        } else {
            slot.loaded = true;
            // Stage 31: 2 sn üstü yavaş rozeti
            if (slot.loadMs > 2000) {
                addLog(slot.id, QString("YAVAŞ eklenti: %1 ms").arg(slot.loadMs));
                emit pluginLog(slot.id, QString("Yavaş yükleme (%1 ms) — devre dışı bırakmayı düşünün")
                                                  .arg(slot.loadMs));
            }
        }
    }
    m_js.globalObject().deleteProperty("verso");
    return m_plugins;
}

bool PluginEngine::unload(const QString& id) {
    for (int i = 0; i < m_plugins.size(); ++i) {
        if (m_plugins[i].id != id) continue;
        for (const QString& c : m_plugins[i].commands) {
            m_commands.remove(c);
            m_cmdOwner.remove(c);
        }
        for (auto it = m_views.begin(); it != m_views.end();) {
            if (m_viewOwner.value(it.key()) == id) {
                m_viewOwner.remove(it.key());
                it = m_views.erase(it);
            } else {
                ++it;
            }
        }
        m_handlers.remove(id);
        m_plugins.removeAt(i);
        delete m_apis.take(id);
        return true;
    }
    return false;
}

bool PluginEngine::isLoaded(const QString& id) const {
    for (const Plugin& p : m_plugins)
        if (p.id == id) return p.loaded;
    return false;
}

QJSValue PluginEngine::callCommand(const QString& cmdId, const QString& arg) {
    // Görünüm komutları renderView'e yönlenir
    if (m_views.contains(cmdId)) return QJSValue();
    if (!m_commands.contains(cmdId)) return QJSValue();
    if (VersoApi* api = m_apis.value(m_cmdOwner.value(cmdId), nullptr))
        m_js.globalObject().setProperty("verso", m_js.newQObject(api));
    QJSValueList args;
    if (!arg.isEmpty()) args << arg;
    QJSValue r = m_commands[cmdId].call(args);
    if (r.isError()) noteError(m_cmdOwner.value(cmdId), r.toString());
    return r;
}

QString PluginEngine::renderView(const QString& cmdId) {
    if (!m_views.contains(cmdId)) return {};
    if (VersoApi* api = m_apis.value(m_viewOwner.value(cmdId), nullptr))
        m_js.globalObject().setProperty("verso", m_js.newQObject(api));
    QJSValue r = m_views[cmdId].call();
    if (r.isError()) {
        noteError(m_viewOwner.value(cmdId), r.toString());
        return {};
    }
    return r.toString().left(100000);
}

void PluginEngine::fireEvent(const QString& event, const QString& arg) {
    for (auto it = m_handlers.constBegin(); it != m_handlers.constEnd(); ++it) {
        const QString pid = it.key();
        if (!it.value().contains(event)) continue;
        if (VersoApi* api = m_apis.value(pid, nullptr))
            m_js.globalObject().setProperty("verso", m_js.newQObject(api));
        QJSValueList args;
        if (!arg.isEmpty()) args << arg;
        QJSValue r = it.value()[event].call(args);
        if (r.isError()) noteError(pid, r.toString());
    }
}

void PluginEngine::setPermOverride(const QString& id, const QStringList& perms) {
    QSettings("Verso", "VersoCoder").setValue("plugin/perms/" + id, perms);
}

QStringList PluginEngine::permOverride(const QString& id) const {
    return QSettings("Verso", "VersoCoder").value("plugin/perms/" + id).toStringList();
}

void PluginEngine::clearQuarantine(const QString& id) {
    QSettings q("Verso", "VersoCoder");
    QStringList ql = q.value("plugin/quarantine").toStringList();
    ql.removeAll(id);
    q.setValue("plugin/quarantine", ql);
    m_errors.remove(id);
}

QString VersoApi::currentFile() {
    const PluginEngine* eng = engine();
    return eng ? eng->currentFilePath() : QString();
}
