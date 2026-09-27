#pragma once
#include <QDialog>

class QLabel;
class QListWidget;
class QPlainTextEdit;
class QTabWidget;
class QString;
struct AppSettings;

// Stage 34: Ajan Paneli — politika/bütçe, proje sağlık skoru, beceri zinciri,
// bellek ve koşu günlüğü (tek tıkla geri alma).
class AgentPanelDialog : public QDialog {
    Q_OBJECT
public:
    explicit AgentPanelDialog(const QString& projectRoot, QWidget* parent = nullptr);

    void setRunCount(int n);

private slots:
    void refreshAll();
    void onRunSelected();
    void revertRun();
    void deleteRun();
    void forgetNote();
    void addNote();
    void runHealth();
    void refreshAudit(); // Stage 38: komut denetimi

private:
    QString m_root;

    QTabWidget* m_tabs = nullptr;
    QLabel* m_policyLabel = nullptr;
    QLabel* m_budgetLabel = nullptr;
    QLabel* m_healthLabel = nullptr;
    QPlainTextEdit* m_healthNotes = nullptr;
    QListWidget* m_skills = nullptr;
    QListWidget* m_memory = nullptr;
    QListWidget* m_runs = nullptr;
    QPlainTextEdit* m_runDetail = nullptr;
    // Stage 38: komut denetimi sekmesi
    class QTextBrowser* m_auditView = nullptr;
    class QLineEdit* m_auditSearch = nullptr;
    class QLabel* m_auditSummary = nullptr;
};
