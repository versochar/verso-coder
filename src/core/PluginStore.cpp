#include "PluginStore.h"
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>
#include <cstdlib>

QString PluginStore::defaultRegistry() {
    return "https://raw.githubusercontent.com/versochar/verso-coder-plugins/main";
}

QString PluginStore::registryBase() {
    if (const char* e = std::getenv("VERSO_PLUGIN_REGISTRY"))
        if (*e) return QString::fromUtf8(e);
    const QString s =
        QSettings("Verso", "VersoCoder").value("plugin/registry").toString().trimmed();
    if (!s.isEmpty()) return s;
    return defaultRegistry();
}

QString PluginStore::indexUrl() { return registryBase() + "/index.json"; }

QString PluginStore::pluginUrl(const Entry& e) {
    return registryBase() + "/" + e.file;
}

QByteArray PluginStore::fetchUrl(const QString& url, QString* error, int timeoutMs) {
    QNetworkAccessManager nam;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QNetworkReply* reply = nam.get(QNetworkRequest(QUrl(url)));
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();
    QByteArray out;
    if (!reply->isFinished()) {
        if (error) *error = "zaman aşımı (" + url.left(80) + ")";
    } else if (reply->error() != QNetworkReply::NoError) {
        if (error) *error = reply->errorString().left(200);
    } else {
        out = reply->readAll();
    }
    reply->deleteLater();
    return out;
}

QList<PluginStore::Entry> PluginStore::parseIndex(const QByteArray& json, QString* error) {
    QList<Entry> out;
    QJsonDocument d = QJsonDocument::fromJson(json);
    if (!d.isObject() || !d.object().value("plugins").isArray()) {
        if (error) *error = "index.json çözümlenemedi";
        return out;
    }
    int skipped = 0;
    for (const QJsonValue& v : d.object().value("plugins").toArray()) {
        if (!v.isObject()) {
            ++skipped;
            continue;
        }
        const QJsonObject o = v.toObject();
        Entry e;
        e.id = o.value("id").toString().trimmed();
        e.name = o.value("name").toString().trimmed();
        e.version = o.value("version").toString().trimmed();
        e.description = o.value("description").toString().trimmed().left(300);
        e.author = o.value("author").toString().trimmed().left(80);
        e.file = o.value("file").toString().trimmed();
        e.minApp = o.value("minApp").toString().trimmed();
        for (const QJsonValue& p : o.value("permissions").toArray())
            e.permissions << p.toString().trimmed();
        if (!isValidId(e.id) || !e.valid()) {
            ++skipped;
            continue;
        }
        if (e.name.isEmpty()) e.name = e.id;
        out << e;
    }
    if (out.isEmpty() && error)
        *error = QString("kayıtta kullanılabilir eklenti yok (%1 atlandı)").arg(skipped);
    return out;
}

QList<PluginStore::Entry> PluginStore::fetchIndex(QString* error) {
    QString err;
    const QByteArray body = fetchUrl(indexUrl(), &err);
    if (!err.isEmpty()) {
        if (error) *error = err;
        return {};
    }
    return parseIndex(body, error);
}

bool PluginStore::isValidId(const QString& id) {
    static QRegularExpression re("^[a-z0-9][a-z0-9-]{0,39}$");
    return re.match(id).hasMatch();
}

bool PluginStore::install(const Entry& e, const QString& source, const QString& dir,
                          QString* error) {
    if (!isValidId(e.id)) {
        if (error) *error = "geçersiz eklenti kimliği";
        return false;
    }
    if (source.trimmed().isEmpty()) {
        if (error) *error = "boş eklenti kaynağı";
        return false;
    }
    QDir d(dir);
    if (!d.exists() && !QDir().mkpath(dir)) {
        if (error) *error = "eklenti dizini açılamadı";
        return false;
    }
    QFile f(d.filePath(e.id + ".js"));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) *error = "dosya yazılamadı";
        return false;
    }
    f.write(source.toUtf8());
    f.close();
    // installFromFolder ile aynı kural: ilk kurulumda mevcutları koru,
    // sonra yeniyi etkin listesine ekle (liste yoksa herkes etkindir).
    QSettings q("Verso", "VersoCoder");
    QStringList en = q.value("plugin/enabled").toStringList();
    if (!q.contains("plugin/enabled")) {
        for (const QFileInfo& fi : d.entryInfoList({"*.js"}, QDir::Files))
            if (fi.completeBaseName() != e.id && !en.contains(fi.completeBaseName()))
                en << fi.completeBaseName();
    }
    if (!en.contains(e.id)) en << e.id;
    q.setValue("plugin/enabled", en);
    return true;
}

QString PluginStore::installedVersion(const QString& dir, const QString& id) {
    QFile f(QDir(dir).filePath(id + ".js"));
    if (!f.open(QIODevice::ReadOnly)) return {};
    const QString src = QString::fromUtf8(f.readAll());
    static QRegularExpression re(R"(^\s*//\s*@version\s+(.+)$)",
                                 QRegularExpression::MultilineOption);
    auto m = re.match(src);
    return m.hasMatch() ? m.captured(1).trimmed().left(40) : QString();
}
