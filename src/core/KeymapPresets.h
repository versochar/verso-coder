#pragma once
#include <QMap>
#include <QString>
#include <QStringList>

// Hazır kısayol profilleri (VS Code / JetBrains / Vim).
// Dönen harita: komut-id -> "Ctrl+P" (AppSettings.shortcuts override'ları).
class KeymapPresets {
public:
    static QStringList names();                              // {"Varsayılan","VS Code","JetBrains","Vim"}
    static QMap<QString, QString> preset(const QString& name);
};
