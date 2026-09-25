#include "ToolchainProbe.h"
#include <QProcess>
#include <QStandardPaths>

QString ToolchainProbe::locate(const QString& prog) {
    if (prog.contains('/')) return prog; // mutlak/bağıl yol verilmiş
    return QStandardPaths::findExecutable(prog);
}

ToolInfo ToolchainProbe::probe(const QString& name, const QString& prog,
                               const QStringList& versionArgs, int timeoutMs) {
    ToolInfo t;
    t.name = name;
    t.command = prog;
    t.path = locate(prog);
    if (t.path.isEmpty()) return t;
    t.found = true;
    QProcess p;
    p.start(t.path, versionArgs);
    if (p.waitForFinished(timeoutMs)) {
        QString out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
        if (out.isEmpty()) out = QString::fromUtf8(p.readAllStandardError()).trimmed();
        t.version = out.split('\n').value(0).trimmed();
    }
    return t;
}

QList<ToolInfo> ToolchainProbe::probeAll() {
    QList<ToolInfo> out;
    out << probe("Git", "git", {"--version"});
    out << probe("CMake", "cmake", {"--version"});
    out << probe("GCC/G++", "g++", {"--version"});
    out << probe("Clang", "clang", {"--version"});
    out << probe("clangd (LSP)", "clangd", {"--version"});
    out << probe("pylsp (LSP)", "pylsp", {"--version"});
    out << probe("Python", "python3", {"--version"});
    out << probe("Ollama", "ollama", {"--version"});
    out << probe("Bash", "bash", {"--version"});
    return out;
}

QString ToolchainProbe::report(const QList<ToolInfo>& tools) {
    QString s;
    for (const ToolInfo& t : tools) {
        s += QString("[%1] %2").arg(t.found ? "✓" : "✗", t.name.leftJustified(14, ' '));
        if (t.found) {
            s += t.path;
            if (!t.version.isEmpty()) s += "  —  " + t.version.left(80);
        } else {
            s += "(bulunamadı)";
        }
        s += "\n";
    }
    return s;
}
