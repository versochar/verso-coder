#include "KeymapPresets.h"

QStringList KeymapPresets::names() {
    return {"Varsayılan", "VS Code", "JetBrains", "Vim"};
}

QMap<QString, QString> KeymapPresets::preset(const QString& name) {
    QMap<QString, QString> m;
    const QString n = name.toLower();
    if (n == "varsayılan" || n == "varsayilan" || n == "default") {
        return m; // override yok → koddaki varsayılanlar kullanılır
    }
    if (n == "vs code" || n == "vscode" || n == "vs" ) {
        m["file.save"] = "Ctrl+S";
        m["file.saveAll"] = "Ctrl+K, Ctrl+S";
        m["nav.quickOpen"] = "Ctrl+P";
        m["nav.palette"] = "Ctrl+Shift+P";
        m["nav.search"] = "Ctrl+Shift+F";
        m["nav.gotoLine"] = "Ctrl+G";
        m["run.build"] = "Ctrl+F5";
        m["lsp.definition"] = "F12";
        m["lsp.hover"] = "Ctrl+K, Ctrl+I";
        m["view.terminal"] = "Ctrl+`";
        m["edit.addNext"] = "Ctrl+D";
        return m;
    }
    if (n == "jetbrains" || n == "intellij") {
        m["file.save"] = "Ctrl+S";
        m["file.saveAll"] = "Ctrl+Shift+S";
        m["nav.quickOpen"] = "Ctrl+Shift+N";
        m["nav.palette"] = "Ctrl+Shift+A";
        m["nav.search"] = "Ctrl+Shift+F";
        m["nav.gotoLine"] = "Ctrl+G";
        m["run.build"] = "Shift+F10";
        m["lsp.definition"] = "Ctrl+B";
        m["lsp.hover"] = "Ctrl+Q";
        m["view.terminal"] = "Alt+F12";
        m["edit.duplicate"] = "Ctrl+D";
        return m;
    }
    if (n == "vim") {
        m["file.save"] = "Ctrl+S";
        m["nav.palette"] = "Ctrl+Shift+P";
        m["nav.quickOpen"] = "Ctrl+P";
        m["nav.search"] = "Ctrl+Shift+F";
        m["lsp.definition"] = "F12";
        m["run.build"] = "F5";
        return m;
    }
    return m; // bilinmeyen profil → boş
}
