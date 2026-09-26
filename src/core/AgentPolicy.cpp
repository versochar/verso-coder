#include "AgentPolicy.h"
#include <QSet>

bool AgentPolicy::isReadOnlyTool(const QString& name) {
    static const QSet<QString> readOnly = {
        "read_file",   "read_range", "list_dir",  "search",     "grep_lines",
        "get_problems", "find_symbol", "git_status", "git_diff", "health_scan"};
    return readOnly.contains(name);
}

bool AgentPolicy::isMutatingTool(const QString& name) {
    static const QSet<QString> mutating = {
        "write_file", "run_command", "run_tests"};
    return mutating.contains(name);
}

QStringList AgentPolicy::readOnlyTools() {
    return {"read_file",   "read_range", "list_dir",   "search",       "grep_lines",
            "get_problems", "find_symbol", "git_status", "git_diff", "health_scan"};
}

bool AgentPolicy::toolAllowed(const QString& name) const {
    if (isReadOnlyTool(name)) return true;      // salt-okunur her zaman serbest
    if (isMutatingTool(name)) {
        if (name == "write_file") return allowWrite;
        if (name == "run_command") return allowCommand;
        if (name == "run_tests") return allowTests;
        return false;
    }
    return false; // bilinmeyen araç: kapalı (güvenli varsayılan)
}

bool AgentPolicy::autonomousAllows(const QString& name) const {
    // Otonom hedef ajanı yalnızca okuma + raporlama yapar.
    if (!autonomous) return toolAllowed(name);
    return isReadOnlyTool(name);
}

bool AgentPolicy::needsApproval(const QString& name) const {
    if (isReadOnlyTool(name)) return false;
    if (!isMutatingTool(name)) return true; // bilinmeyen araç: onay iste
    return requireApproval;
}

QString AgentPolicy::summary() const {
    if (autonomous) return QString("otonom (salt-okunur) · %1 adım").arg(maxSteps);
    QStringList on;
    if (allowWrite) on << "yazma";
    if (allowCommand) on << "komut";
    if (allowTests) on << "test";
    if (on.isEmpty()) return QString("salt-okunur · %1 adım").arg(maxSteps);
    return QString("%1 açık · %2 adım")
        .arg(on.join("+"), QString::number(maxSteps));
}

AgentPolicy AgentPolicy::safeDefault() {
    AgentPolicy p;
    p.autonomous = false;
    p.allowWrite = false;
    p.allowCommand = false;
    p.allowTests = false;
    p.maxSteps = 5;
    p.requireApproval = true;
    return p;
}

AgentPolicy AgentPolicy::autonomousReadOnly(int steps) {
    AgentPolicy p;
    p.autonomous = true;
    p.allowWrite = false;
    p.allowCommand = false;
    p.allowTests = false;
    p.maxSteps = steps > 0 ? steps : 4;
    p.requireApproval = true;
    return p;
}
