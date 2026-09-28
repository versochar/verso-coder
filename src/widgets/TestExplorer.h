#pragma once
#include "../core/TestParser.h"
#include <QWidget>

class QProgressBar;
class QPlainTextEdit;
class QToolBar;
class QTreeWidget;

// Stage 14: test gezgini — keşif + ağaç + çalıştır + sonuç.
class TestExplorer : public QWidget {
    Q_OBJECT
public:
    explicit TestExplorer(QWidget* parent = nullptr);

    void setTests(const QList<TestCase>& tests);
    void setResults(const QList<TestCase>& results);
    void setRunning(bool on);
    void appendLog(const QString& text);
    void clearLog();
    QString logText() const;
    void setSummary(int pass, int fail, int skip);

signals:
    void discoverRequested();
    void runAllRequested();
    void runOneRequested(const QString& testId);
    void runFailedRequested();
    void stopRequested();

public:
    QStringList failedIds() const { return m_failed; }

private:
    void onItemDoubleClicked();
    void applyFilter(const QString& text);

    QToolBar* m_bar;
    QTreeWidget* m_tree;
    QPlainTextEdit* m_log;
    QProgressBar* m_bar2;
    class QLabel* m_summary;
    class QLineEdit* m_filter;
    QList<TestCase> m_tests;
    QStringList m_failed;
};
