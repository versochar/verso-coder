#include "ConnectionProfile.h"
#include <QSettings>

static const char* kGroup = "remoteProfiles";

QString ConnectionProfile::sanitize(const QString& raw) {
    QString out;
    for (QChar ch : raw.trimmed()) {
        if (ch.isLetterOrNumber() || ch == '-' || ch == '_' || ch == ' ' || ch == '.')
            out += ch;
    }
    return out.trimmed().left(40);
}

QString ConnectionProfile::toUri(const QString& path) const {
    QString u = "ssh://";
    if (!user.isEmpty()) u += user + "@";
    u += host;
    if (port != 22) u += ":" + QString::number(port);
    u += path.isEmpty() ? remoteRoot : path;
    return u;
}

QString ConnectionProfile::display() const {
    QString u = user.isEmpty() ? host : user + "@" + host;
    if (port != 22) u += ":" + QString::number(port);
    return name.isEmpty() ? u : QString("%1 (%2)").arg(name, u);
}

QMap<QString, ConnectionProfile> ConnectionProfiles::all() {
    QMap<QString, ConnectionProfile> out;
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    for (const QString& g : q.childGroups()) {
        q.beginGroup(g);
        ConnectionProfile p;
        p.name = g;
        p.host = q.value("host").toString();
        p.port = q.value("port", 22).toInt();
        p.user = q.value("user").toString();
        p.keyPath = q.value("key").toString();
        p.jumpHost = q.value("jump").toString();
        p.remoteRoot = q.value("root").toString();
        p.remoteCmd = q.value("shell").toString();
        p.trustNewHosts = q.value("trustNew", false).toBool();
        q.endGroup();
        if (p.isValid()) out[g] = p;
    }
    return out;
}

bool ConnectionProfiles::save(const ConnectionProfile& p) {
    const QString n = ConnectionProfile::sanitize(p.name.isEmpty() ? p.host : p.name);
    if (n.isEmpty() || p.host.trimmed().isEmpty()) return false;
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    q.beginGroup(n);
    q.setValue("host", p.host.trimmed());
    q.setValue("port", qBound(1, p.port, 65535));
    q.setValue("user", p.user.trimmed());
    q.setValue("key", p.keyPath.trimmed());
    q.setValue("jump", p.jumpHost.trimmed());
    q.setValue("root", p.remoteRoot.trimmed());
    q.setValue("shell", p.remoteCmd.trimmed());
    q.setValue("trustNew", p.trustNewHosts);
    q.endGroup();
    q.endGroup();
    return true;
}

bool ConnectionProfiles::remove(const QString& name) {
    if (!exists(name)) return false;
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    q.remove(name);
    return true;
}

bool ConnectionProfiles::exists(const QString& name) {
    return all().contains(name);
}

QStringList ConnectionProfiles::names() {
    QStringList out = all().keys();
    out.sort(Qt::CaseInsensitive);
    return out;
}

ConnectionProfile ConnectionProfiles::get(const QString& name) {
    return all().value(name);
}
