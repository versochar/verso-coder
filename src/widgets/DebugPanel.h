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
    // Stage 26
    void setWatches(const QList<QPair<QString, QString>>& items);
    void setRegisters(const QList<QPair<QString, QString>>& regs);
    void setMemory(const QString& addr, const QString& dump);
    void setDisas(const QList<QMap<QString, QString>>& rows);
    void setThreads(const QList<QPair<QString, QString>>& threads, const QString& current);

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
    // Stage 26
    void watchAddRequested(const QString& expr);
    void watchRemoveRequested(const QString& name);
    void variableEditRequested(const QString& name);
    void memoryReadRequested(const QString& addr);
    void threadSelected(const QString& id);

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
    // Stage 26
    QTreeWidget* m_watch = nullptr;
    class QLineEdit* m_watchEdit = nullptr;
    QTreeWidget* m_regs = nullptr;
    QTextEdit* m_mem = nullptr;
    class QLineEdit* m_memAddr = nullptr;
    QTreeWidget* m_disas = nullptr;
    QListWidget* m_threads = nullptr;
    QStringList m_cmdHist;
    int m_histPos = -1;

protected:
    bool eventFilter(QObject* o, QEvent* e) override;
};
