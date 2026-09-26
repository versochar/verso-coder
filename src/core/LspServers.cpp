#include "LspServers.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

QList<LspServerDef> LspServers::table() {
    return {
        {"cpp", "clangd", {"--offset-encoding=utf-8"},
         {"cpp", "h", "hpp", "c", "cc", "cxx"},
         "clangd kurun (örn. apt install clangd) ya da Ayarlar'da yolu verin"},
        {"python", "pylsp", {},
         {"py"},
         "pip install python-lsp-server (jedi/rope için: + pylsp-rope)"},
        {"rust", "rust-analyzer", {},
         {"rs"},
         "rustup component add rust-analyzer"},
        {"go", "gopls", {},
         {"go"},
         "go install golang.org/x/tools/gopls@latest"},
        {"javascript", "typescript-language-server", {"--stdio"},
         {"js", "jsx", "ts", "tsx"},
         "npm i -g typescript-language-server typescript"},
        {"java", "jdtls", {},
         {"java"},
         "jdtls kurun (paket yöneticinizden)"},
    };
}

const LspServerDef* LspServers::forSuffix(const QString& suffix) {
    static const QList<LspServerDef> t = table();
    const QString s = suffix.toLower();
    for (const LspServerDef& d : t)
        if (d.suffixes.contains(s)) return &d;
    return nullptr;
}

QString LspServers::findProgram(const QString& program) {
    return QStandardPaths::findExecutable(program);
}

QStringList LspServers::pythonVenvs(const QString& root) {
    QStringList out;
    const QStringList cands = {".venv", "venv", ".env"};
    for (const QString& c : cands) {
        const QString p = QDir(root).absoluteFilePath(c);
        if (QFileInfo::exists(p + "/bin/python") || QFileInfo::exists(p + "/bin/pylsp"))
            out << p;
    }
    const QString env = qEnvironmentVariable("VIRTUAL_ENV");
    if (!env.isEmpty() && !out.contains(env)) out.prepend(env);
    return out;
}

QString LspServers::pythonExe(const QString& root) {
    for (const QString& v : pythonVenvs(root)) {
        if (QFileInfo::exists(v + "/bin/pylsp")) return v + "/bin/pylsp";
    }
    return findProgram("pylsp");
}
