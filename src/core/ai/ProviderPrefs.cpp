#include "ProviderPrefs.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

static const char* kModels = "aiProviderModels";
static const char* kUrls = "aiProviderUrls";

static QJsonObject readMap(const char* key) {
    QSettings q("Verso", "VersoCoder");
    return QJsonDocument::fromJson(q.value(key).toString().toUtf8()).object();
}

static void writeMap(const char* key, const QJsonObject& o) {
    QSettings q("Verso", "VersoCoder");
    q.setValue(key, QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)));
}

QString ProviderPrefs::activeProvider() {
    QSettings q("Verso", "VersoCoder");
    return q.value("ai/provider", "ollama").toString();
}

void ProviderPrefs::setActiveProvider(const QString& id) {
    if (id.trimmed().isEmpty()) return;
    QSettings q("Verso", "VersoCoder");
    q.setValue("ai/provider", id.trimmed());
}

QString ProviderPrefs::modelFor(const QString& id, const QString& fallback) {
    const QString v = readMap(kModels).value(id).toString();
    return v.isEmpty() ? fallback : v;
}

void ProviderPrefs::setModel(const QString& id, const QString& model) {
    if (id.isEmpty()) return;
    QJsonObject o = readMap(kModels);
    if (model.trimmed().isEmpty()) o.remove(id);
    else o[id] = model.trimmed();
    writeMap(kModels, o);
}

QString ProviderPrefs::urlFor(const QString& id) {
    return readMap(kUrls).value(id).toString();
}

void ProviderPrefs::setUrl(const QString& id, const QString& url) {
    if (id.isEmpty()) return;
    QJsonObject o = readMap(kUrls);
    QString u = url.trimmed();
    while (u.endsWith('/')) u.chop(1);
    if (u.isEmpty()) o.remove(id);
    else o[id] = u;
    writeMap(kUrls, o);
}

ProviderSpec ProviderPrefs::resolve(const QString& id) {
    const QString want = id.isEmpty() ? activeProvider() : id;
    ProviderSpec spec = ProviderRegistry::byId(want);
    if (spec.id.isEmpty()) spec = ProviderRegistry::defaultProvider();
    const QString u = urlFor(spec.id);
    if (!u.isEmpty()) spec.baseUrl = u; // özel sunucu
    return spec;
}

QStringList ProviderPrefs::configuredProviders() {
    QStringList out = readMap(kModels).keys();
    for (const QString& id : readMap(kUrls).keys())
        if (!out.contains(id)) out << id;
    return out;
}

void ProviderPrefs::reset() {
    writeMap(kModels, QJsonObject());
    writeMap(kUrls, QJsonObject());
    QSettings q("Verso", "VersoCoder");
    q.remove("ai/provider");
}
