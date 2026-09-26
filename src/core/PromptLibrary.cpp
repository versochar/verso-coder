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

// --- Stage 37: görev ipucu ---
QStringList PromptLibrary::knownTaskHints() {
    return {QStringLiteral("explain"), QStringLiteral("review"), QStringLiteral("test"),
            QStringLiteral("doc"),   QStringLiteral("commit"), QStringLiteral("summarize"),
            QStringLiteral("theme"),  QStringLiteral("vision")};
}

bool PromptLibrary::isKnownTaskHint(const QString& h) {
    return knownTaskHints().contains(h.trimmed().toLower());
}

bool PromptLibrary::hasTaskHint(const QString& raw) {
    const QString t = raw.trimmed();
    if (!t.startsWith(QLatin1Char('!'))) return false;
    const int sp = t.indexOf(QChar(' '));
    const QString head = sp < 0 ? t.mid(1) : t.mid(1, sp - 1);
    return isKnownTaskHint(head);
}

QString PromptLibrary::taskHint(const QString& raw) {
    if (!hasTaskHint(raw)) return {};
    const QString t = raw.trimmed();
    const int sp = t.indexOf(QChar(' '));
    return (sp < 0 ? t.mid(1) : t.mid(1, sp - 1)).trimmed().toLower();
}

QString PromptLibrary::stripTaskHint(const QString& raw) {
    if (!hasTaskHint(raw)) return raw;
    const QString t = raw.trimmed();
    const int sp = t.indexOf(QChar(' '));
    return (sp < 0) ? QString() : t.mid(sp + 1).trimmed();
}
