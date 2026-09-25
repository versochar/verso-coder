#pragma once
#include <QJsonObject>
#include <QString>

// Proje çalışma alanı ayarları: <root>/.verso/workspace.json
// 0 değerleri = "devral" (global ayarı kullan).
struct WorkspaceSettings {
    int fontSize = 0;      // >0 ise geçersiz kılar
    int tabWidth = 0;
    int largeFileMb = 0;
    bool useEditorConfig = true;
    QString keymapPreset;  // boşsa global
    QString buildTaskLabel;
    QJsonObject raw;       // bilinmeyen alanları koru
};

class WorkspaceConfig {
public:
    static QString configPath(const QString& root);
    static WorkspaceSettings load(const QString& root);
    static bool save(const QString& root, const WorkspaceSettings& w);
    static bool exists(const QString& root);
};
