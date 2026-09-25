#include "ProfileStore.h"
#include <QSettings>

static const char* kGroup = "uiProfiles";

QStringList ProfileStore::names() {
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    QStringList out = q.childKeys();
    out.sort(Qt::CaseInsensitive);
    return out;
}

bool ProfileStore::save(const QString& name, const QString& json) {
    const QString n = sanitize(name);
    if (n.isEmpty() || json.trimmed().isEmpty()) return false;
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    q.setValue(n, json);
    return true;
}

QString ProfileStore::load(const QString& name) {
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    return q.value(sanitize(name)).toString();
}

bool ProfileStore::remove(const QString& name) {
    const QString n = sanitize(name);
    if (!exists(n)) return false;
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    q.remove(n);
    return true;
}

bool ProfileStore::exists(const QString& name) {
    return names().contains(sanitize(name));
}

QString ProfileStore::sanitize(const QString& raw) {
    QString out;
    for (QChar ch : raw.trimmed()) {
        if (ch.isLetterOrNumber() || ch == '-' || ch == '_' || ch == ' ')
            out += ch;
    }
    return out.trimmed().left(40);
}
