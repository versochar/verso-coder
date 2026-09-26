#pragma once
#include "../core/AboutInfo.h"
#include "../core/ToolchainProbe.h"
#include <QDialog>
#include <QFutureWatcher>

class BackupManager;
class PluginEngine;
class QTabWidget;
class QTextBrowser;
class QTextEdit;
class QTreeWidget;

// Platform & dağıtım paneli: Hakkında + güncelleme kontrolü, araç zinciri
// teşhisi, performans metrikleri ve yedek yönetimi.
class DiagnosticsDialog : public QDialog {
    Q_OBJECT
public:
    explicit DiagnosticsDialog(QWidget* parent = nullptr);
    void setBackupManager(BackupManager* bm);
    void setPluginEngine(PluginEngine* eng); // Stage 29

signals:
    void backupRestored(const QString& originalPath);

private slots:
    void refreshToolchain();
    void refreshPerformance();
    void refreshBackups();
    void checkUpdate();
    void restoreBackup();
    void deleteBackup();
    void exportReport();   // Stage 22: tanı raporunu dosyaya yaz
    void refreshA11y();    // Stage 22: erişilebilirlik + çeviri denetimi
    void refreshPlugins(); // Stage 29: eklenti günlüğü

private:
    QTabWidget* m_tabs;
    QTextBrowser* m_about;
    QTextEdit* m_updateIn;
    QTextEdit* m_tools;
    QTextEdit* m_perf;
    QTreeWidget* m_backups;
    QTextEdit* m_a11y = nullptr; // Stage 22
    QTextEdit* m_plugins = nullptr;  // Stage 29
    class PluginEngine* m_pluginEng = nullptr;
    BackupManager* m_bm = nullptr;
    QFutureWatcher<QList<ToolInfo>> m_probeWatcher;
};
