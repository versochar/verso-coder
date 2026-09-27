#include "CommandAudit.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>

QJsonObject CommandAuditEntry::toJson() const {
    QJsonObject o;
    o["at"] = at.toString(Qt::ISODate);
    o["tool"] = tool;
    o["command"] = command;
    o["approved"] = approved;
    o["denied"] = denied;
    o["exit"] = exitCode;
    o["ms"] = double(durationMs);
    if (!note.isEmpty()) o["note"] = note;
    return o;
}

CommandAuditEntry CommandAuditEntry::fromJson(const QJsonObject& o) {
    CommandAuditEntry e;
    e.at = QDateTime::fromString(o.value("at").toString(), Qt::ISODate);
    e.tool = o.value("tool").toString();
    e.command = o.value("command").toString();
    e.approved = o.value("approved").toBool();
    e.denied = o.value("denied").toBool();
    e.exitCode = o.value("exit").toInt();
    e.durationMs = qint64(o.value("ms").toDouble());
    e.note = o.value("note").toString();
    return e;
}

QString CommandAuditEntry::line() const {
    const QString mark = denied ? QStringLiteral("⊘") : (exitCode == 0 ? QStringLiteral("✓")
                                                                        : QStringLiteral("✗"));
    QString s = QString("%1  %2  %3")
                    .arg(at.toString("HH:mm:ss"), mark, tool);
    QString cmd = command;
    cmd.replace(QLatin1Char('\n'), QLatin1Char(' '));
    if (cmd.size() > 120) cmd = cmd.left(117) + "…";
    s += QStringLiteral("  ") + cmd;
    if (durationMs > 0) s += QStringLiteral("  (%1 sn)").arg(double(durationMs) / 1000.0, 0, 'f', 1);
    if (denied && !note.isEmpty()) s += QStringLiteral("  — ") + note;
    return s;
}

CommandAudit::CommandAudit(const QString& file) {
    m_file = file;
    if (m_file.isEmpty()) {
        m_file = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                 + QStringLiteral("/command-audit.json");
    }
    load();
}

bool CommandAudit::load() {
    QFile f(m_file);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QJsonArray arr = QJsonDocument::fromJson(f.readAll()).array();
    m_entries.clear();
    for (const QJsonValue& v : arr) m_entries.append(CommandAuditEntry::fromJson(v.toObject()));
    return true;
}

bool CommandAudit::save() const {
    if (m_file.isEmpty()) return false;
    QDir().mkpath(QFileInfo(m_file).absolutePath());
    QFile f(m_file);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    QJsonArray arr;
    for (const CommandAuditEntry& e : m_entries) arr.append(e.toJson());
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    return true;
}

void CommandAudit::clear() {
    m_entries.clear();
    save();
}

void CommandAudit::setMaxEntries(int n) {
    m_max = qMax(10, n);
    while (m_entries.size() > m_max) m_entries.removeFirst();
    save();
}

void CommandAudit::record(const CommandAuditEntry& e) {
    m_entries.append(e);
    while (m_entries.size() > m_max) m_entries.removeFirst();
    save();
}

void CommandAudit::record(const QString& tool, const QString& command, bool approved,
                          bool denied, int exitCode, qint64 durationMs, const QString& note) {
    CommandAuditEntry e;
    e.at = QDateTime::currentDateTime();
    e.tool = tool;
    e.command = command;
    e.approved = approved;
    e.denied = denied;
    e.exitCode = exitCode;
    e.durationMs = durationMs;
    e.note = note;
    record(e);
}

QList<CommandAuditEntry> CommandAudit::last(int n) const {
    QList<CommandAuditEntry> out;
    const int start = qMax(0, int(m_entries.size()) - qMax(1, n));
    for (int i = start; i < m_entries.size(); ++i) out.append(m_entries.at(i));
    return out;
}

QList<CommandAuditEntry> CommandAudit::search(const QString& needle) const {
    QList<CommandAuditEntry> out;
    const QString n = needle.trimmed();
    if (n.isEmpty()) return out;
    for (int i = m_entries.size() - 1; i >= 0; --i) {
        const CommandAuditEntry& e = m_entries.at(i);
        if (e.command.contains(n, Qt::CaseInsensitive) || e.tool.contains(n, Qt::CaseInsensitive) ||
            e.note.contains(n, Qt::CaseInsensitive))
            out.append(e);
    }
    return out;
}

CommandAudit::Summary CommandAudit::summary() const {
    Summary s;
    for (const CommandAuditEntry& e : m_entries) {
        ++s.total;
        if (e.denied) ++s.denied;
        s.totalMs += e.durationMs;
    }
    return s;
}

QString CommandAudit::toText(const QList<CommandAuditEntry>& list) {
    QString out;
    for (const CommandAuditEntry& e : list) out += e.line() + QLatin1Char('\n');
    return out;
}
