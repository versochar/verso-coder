#pragma once
#include "../core/AboutInfo.h"
#include "../core/ToolchainProbe.h"
#include <QDialog>
#include <QFutureWatcher>

class BackupManager;
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

signals:
    void backupRestored(const QString& originalPath);

private slots:
    void refreshToolchain();
    void refreshPerformance();
    void refreshBackups();
    void checkUpdate();
    void restoreBackup();
    void deleteBackup();

private:
    QTabWidget* m_tabs;
    QTextBrowser* m_about;
    QTextEdit* m_updateIn;
    QTextEdit* m_tools;
    QTextEdit* m_perf;
    QTreeWidget* m_backups;
    BackupManager* m_bm = nullptr;
    QFutureWatcher<QList<ToolInfo>> m_probeWatcher;
};
