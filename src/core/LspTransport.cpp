#include "LspTransport.h"
#include "SshSession.h"

LspTransport LspTransport::local(const QString& program, const QStringList& args,
                                 const QString& workdir) {
    LspTransport t;
    t.kind = Kind::Local;
    t.program = program;
    t.args = args;
    t.workdir = workdir;
    return t;
}

LspTransport LspTransport::remote(const ConnectionProfile& p, const QString& remoteLsp,
                                  const QStringList& args) {
    LspTransport t;
    t.kind = Kind::Remote;
    t.program = remoteLsp.isEmpty() ? "clangd" : remoteLsp;
    t.args = args;
    t.profile = p;
    t.workdir = p.remoteRoot;
    return t;
}

void LspTransport::toProcess(const LspTransport& t, QString& outProgram, QStringList& outArgs) {
    if (t.kind == Kind::Local) {
        outProgram = t.program;
        outArgs = t.args;
        return;
    }
    // Uzak LSP: ssh üzerinden stdio — LspClient QProcess'i ssh'a bağlar
    outProgram = "ssh";
    outArgs = SshSession::sshBaseArgs(t.profile);
    outArgs << SshSession::sshTarget(t.profile);
    QString cmd = t.program;
    for (const QString& a : t.args) cmd += " '" + QString(a).replace("'", "'\"'\"'") + "'";
    // Uzak kökte çalış (clangd --root için değil, göreli yollar için)
    outArgs << SshSession::remoteShellCmd(t.profile, cmd, t.profile.remoteRoot);
}

QString LspTransport::remoteCommandLine(const LspTransport& t) {
    QString prog;
    QStringList args;
    toProcess(t, prog, args);
    return prog + " " + args.join(' ');
}
