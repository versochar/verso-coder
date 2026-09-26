#pragma once
#include <QJsonArray>
#include <QList>
#include <QString>

// Stage 25: harici araç komutları — `.verso/tools.json`:
// [{label, command, cwd?}]. Saf ayrıştırma (test edilebilir).
struct ToolDef {
    QString label;
    QString command;
    QString cwd;
};

class ExternalTools {
public:
    static QList<ToolDef> parse(const QString& json, QString* error = nullptr);
    static QString configPathForRoot(const QString& root);
    static QString commandId(const QString& label); // "tool.<temizlenmiş>"
};
