#pragma once
#include <QMap>
#include <QString>
#include <QStringList>

// Stage 27: dil sunucusu kayıt tablosu — sonek → sunucu.
// Saf mantık (test edilebilir); başlatma MainWindow::lspClientFor'da.
struct LspServerDef {
    QString lang;        // "cpp" | "python" | ...
    QString program;     // "clangd"
    QStringList args;
    QStringList suffixes;
    QString installHint; // bulunamazsa gösterilir
};

class LspServers {
public:
    static QList<LspServerDef> table();
    static const LspServerDef* forSuffix(const QString& suffix);
    static QString findProgram(const QString& program); // PATH araması
    // Python: proje venv'i (.venv/venv/$VIRTUAL_ENV) → pylsp, yoksa PATH
    static QString pythonExe(const QString& root);
    static QStringList pythonVenvs(const QString& root); // bulunan venv kökleri
};
