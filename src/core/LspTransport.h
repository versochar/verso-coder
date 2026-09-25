#pragma once
#include "ConnectionProfile.h"
#include <QString>
#include <QStringList>

// Stage 16: LSP aktarımı — yerel süreç ya da `ssh host -- uzak-lsp` stdio köprüsü.
// LspClient::startRemote() bunu kullanır; komut kurma saf (test edilebilir).
class LspTransport {
public:
    enum class Kind { Local, Remote };
    Kind kind = Kind::Local;
    QString program;          // yerel: "clangd" ; uzak: uzak komut ("clangd")
    QStringList args;
    QString workdir;          // yerel kök (uzakta profil.remoteRoot kullanılır)
    ConnectionProfile profile; // kind==Remote ise

    // LspClient'ın başlatacağı gerçek yerel süreç komutu:
    // yerel → (program, args); uzak → ("ssh", [base..., hedef, "uzakKomut"])
    static void toProcess(const LspTransport& t, QString& outProgram, QStringList& outArgs);
    static QString remoteCommandLine(const LspTransport& t); // günlüğe yazılabilir özet
    static LspTransport local(const QString& program, const QStringList& args,
                              const QString& workdir);
    static LspTransport remote(const ConnectionProfile& p, const QString& remoteLsp,
                               const QStringList& args = {});
};
