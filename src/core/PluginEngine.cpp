#include "PluginEngine.h"
#include <QDir>
#include <QFile>
#include <QJSEngine>
#include <QRegularExpression>

PluginEngine::PluginEngine(QObject* parent) : QObject(parent) {}

QStringList PluginEngine::scanPermissions(const QString& source) {
    QStringList out;
    static QRegularExpression re(R"(^\s*//\s*@permission\s+(\S+))",
                                 QRegularExpression::MultilineOption);
    auto it = re.globalMatch(source);
    while (it.hasNext()) {
        const QString p = it.next().captured(1);
        if ((p == "fs.read" || p == "fs.write" || p == "net") && !out.contains(p))
            out << p;
    }
    return out;
}

void VersoApi::log(const QString& msg) { emit apiLog(m_id, msg.left(500)); }

void VersoApi::registerCommand(const QString& cmdId, const QString& title,
                               const QJSValue& fn) {
    if (!fn.isCallable() || cmdId.trimmed().isEmpty()) return;
    emit apiRegister(m_id, cmdId.trimmed(), title, fn);
}

QString VersoApi::readFile(const QString& path) {
    if (!m_perms.contains("fs.read")) {
        emit apiDenied(m_id, "fs.read");
        return QString();
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(f.read(1 << 20));
}

bool VersoApi::writeFile(const QString& path, const QString& text) {
    if (!m_perms.contains("fs.write")) {
        emit apiDenied(m_id, "fs.write");
        return false;
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(text.toUtf8());
    return true;
}

QJSValue PluginEngine::makeVerso(const QString& id, const QStringList& perms) {
    auto* api = new VersoApi(id, perms, this);
    m_apis[id] = api;
    connect(api, &VersoApi::apiLog, this,
            [this](const QString& pid, const QString& msg) { emit pluginLog(pid, msg); });
    connect(api, &VersoApi::apiDenied, this,
            [this](const QString& pid, const QString& perm) { emit permissionDenied(pid, perm); });
    connect(api, &VersoApi::apiRegister, this, &PluginEngine::onApiRegister);
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

QList<PluginEngine::Plugin> PluginEngine::loadAll(const QString& dir) {
    m_plugins.clear();
    m_commands.clear();
    m_cmdOwner.clear();
    qDeleteAll(m_apis);
    m_apis.clear();
    m_dir = dir;
    QDir d(dir);
    if (!d.exists()) return m_plugins;
    for (const QFileInfo& fi : d.entryInfoList({"*.js"}, QDir::Files, QDir::Name)) {
        Plugin p;
        p.id = fi.completeBaseName();
        p.path = fi.absoluteFilePath();
        QFile f(p.path);
        if (!f.open(QIODevice::ReadOnly)) {
            p.error = "Okunamadı.";
            m_plugins << p;
            continue;
        }
        const QString src = QString::fromUtf8(f.readAll());
        p.permissions = scanPermissions(src);
        QJSValue verso = makeVerso(p.id, p.permissions);
        m_js.globalObject().setProperty("verso", verso);
        QJSValue res = m_js.evaluate(src, p.path);
        if (res.isError()) {
            p.error = QString("Satır %1: %2")
                          .arg(res.property("lineNumber").toInt())
                          .arg(res.toString().left(200));
        } else {
            p.loaded = true;
        }
        m_plugins << p;
    }
    // Global "verso"yu son eklentiye bağlı bırakma (sızıntıyı önle)
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
    if (!m_commands.contains(cmdId)) return QJSValue();
    // Betik çağrı anında da global "verso"yu sahibi eklentiye bağla
    // (loadAll sonunda silinmişti; aksi halde verso.* ReferenceError olur).
    if (VersoApi* api = m_apis.value(m_cmdOwner.value(cmdId), nullptr))
        m_js.globalObject().setProperty("verso", m_js.newQObject(api));
    QJSValueList args;
    if (!arg.isEmpty()) args << arg;
    QJSValue r = m_commands[cmdId].call(args);
    if (r.isError())
        emit pluginLog(m_cmdOwner.value(cmdId), "Hata: " + r.toString().left(200));
    return r;
}
