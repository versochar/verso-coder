#pragma once
#include <QList>
#include <QString>
#include <QStringList>

struct ToolInfo {
    QString name;    // görünen ad
    QString command; // çalıştırılabilir ad ("clangd")
    bool found = false;
    QString path;    // tam yol
    QString version; // ilk satır
};

// Harici araç zinciri tespiti: PATH'te ara + `--version` çalıştır.
class ToolchainProbe {
public:
    static QString locate(const QString& prog);
    static ToolInfo probe(const QString& name, const QString& prog,
                          const QStringList& versionArgs = {"--version"},
                          int timeoutMs = 4000);
    static QList<ToolInfo> probeAll();
    static QString report(const QList<ToolInfo>& tools);
};
