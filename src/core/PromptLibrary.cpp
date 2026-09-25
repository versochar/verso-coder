#include "PromptLibrary.h"
#include <QSettings>

static const char* kGroup = "aiPrompts";

QMap<QString, QString> PromptLibrary::all() {
    QMap<QString, QString> out;
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    for (const QString& k : q.childKeys()) out[k] = q.value(k).toString();
    return out;
}

bool PromptLibrary::save(const QString& name, const QString& prompt) {
    const QString n = sanitize(name);
    if (n.isEmpty() || prompt.trimmed().isEmpty()) return false;
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    q.setValue(n, prompt.trimmed());
    return true;
}

bool PromptLibrary::remove(const QString& name) {
    if (!exists(name)) return false;
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    q.remove(name);
    return true;
}

bool PromptLibrary::exists(const QString& name) {
    return all().contains(name);
}

QStringList PromptLibrary::names() {
    QStringList out = all().keys();
    out.sort(Qt::CaseInsensitive);
    return out;
}

QString PromptLibrary::sanitize(const QString& raw) {
    QString out;
    for (QChar ch : raw.trimmed()) {
        if (ch.isLetterOrNumber() || ch == '-' || ch == '_' || ch == ' ')
            out += ch;
    }
    return out.trimmed().left(40);
}
