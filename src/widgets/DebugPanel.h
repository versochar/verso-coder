#pragma once
#include "../core/GdbDriver.h"
#include <QWidget>

class QListWidget;
class QTabWidget;
class QTextEdit;
class QToolBar;
class QTreeWidget;

// Stage 14: hata ayıklama paneli — araç çubuğu + Yığın/Değişken/Kesme/Konsol.
class DebugPanel : public QWidget {
    Q_OBJECT
public:
    explicit DebugPanel(QWidget* parent = nullptr);

    void setRunning(bool running, bool stopped);
    void setFrames(const QList<DebugFrame>& frames);
    void setVariables(const QList<DebugVar>& vars);
    void setBreakpoints(const QList<struct Breakpoint>& bps, const QString& currentFile);
    void appendOutput(const QString& text, bool isError = false);
    void clearOutput();

signals:
    void startRequested();
    void stopRequested();
    void continueRequested();
    void nextRequested();
    void stepRequested();
    void finishRequested();
    void frameSelected(int frame);
    void breakpointToggled(const QString& file, int line);
    void evaluateRequested(const QString& expr);
    void consoleRequested(const QString& command);

private:
    QToolBar* m_bar;
    QAction* m_actStart;
    QAction* m_actStop;
    QAction* m_actCont;
    QAction* m_actNext;
    QAction* m_actStep;
    QAction* m_actFinish;
    QTabWidget* m_tabs;
    QTreeWidget* m_stack;
    QTreeWidget* m_vars;
    QListWidget* m_bps;
    QTextEdit* m_console;
    class QLineEdit* m_eval;
    class QLineEdit* m_cmd;
};
