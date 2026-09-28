#pragma once
#include <QDialog>

class PluginEngine;
class QListWidget;
class QTextEdit;

// Stage 29: eklenti yöneticisi — kur/kaldır, aç/kapa, izinler,
// karantina, ayarlar, günlük.
class PluginManagerDialog : public QDialog {
    Q_OBJECT
public:
    explicit PluginManagerDialog(PluginEngine* engine, QWidget* parent = nullptr);

private slots:
    void refreshList();
    void installFromFolder();
    void removeSelected();
    void toggleEnabled();
    void editPermissions();
    void clearQuarantineSel();
    void editSettings();
    void showLog();
    void openStore();

private:
    PluginEngine* m_eng;
    QListWidget* m_list = nullptr;
    QTextEdit* m_info = nullptr;
};
