#include "DebugPanel.h"
#include "../core/BreakpointStore.h"
#include <QAction>
#include <QFileInfo>
#include <QLineEdit>
#include <QListWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>

DebugPanel::DebugPanel(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    m_bar = new QToolBar(this);
    m_actStart = m_bar->addAction("▶ Başlat", this, &DebugPanel::startRequested);
    m_actStart->setToolTip("Hata ayıklamayı başlat (Ctrl+F5)");
    m_actStop = m_bar->addAction("■ Durdur", this, &DebugPanel::stopRequested);
    m_actCont = m_bar->addAction("⏵ Devam", this, &DebugPanel::continueRequested);
    m_actCont->setToolTip("Devam et (F5 ile çakışmaması için panelden)");
    m_actNext = m_bar->addAction("⤼ Adım Üstü", this, &DebugPanel::nextRequested);
    m_actNext->setToolTip("Sonraki satır (F10)");
    m_actStep = m_bar->addAction("⤵ Adım İçi", this, &DebugPanel::stepRequested);
    m_actStep->setToolTip("İçeri adım (F11)");
    m_actFinish = m_bar->addAction("⤴ Bitir", this, &DebugPanel::finishRequested);
    m_actFinish->setToolTip("Fonksiyondan çık (Shift+F11)");
    lay->addWidget(m_bar);

    m_tabs = new QTabWidget(this);
    m_stack = new QTreeWidget(this);
    m_stack->setHeaderLabels({"Düzey", "Fonksiyon", "Dosya:Satır"});
    m_stack->setRootIsDecorated(false);
    connect(m_stack, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem* it) {
                if (it) emit frameSelected(it->data(0, Qt::UserRole).toInt());
            });
    m_tabs->addTab(m_stack, "Yığın");

    auto* varTab = new QWidget(this);
    auto* vlay = new QVBoxLayout(varTab);
    vlay->setContentsMargins(0, 0, 0, 0);
    m_vars = new QTreeWidget(varTab);
    m_vars->setHeaderLabels({"Ad", "Değer", "Tür"});
    m_vars->setRootIsDecorated(false);
    vlay->addWidget(m_vars, 1);
    m_eval = new QLineEdit(varTab);
    m_eval->setPlaceholderText("İfade değerlendir (Enter)...");
    connect(m_eval, &QLineEdit::returnPressed, this, [this]() {
        const QString t = m_eval->text().trimmed();
        if (!t.isEmpty()) emit evaluateRequested(t);
    });
    vlay->addWidget(m_eval);
    m_tabs->addTab(varTab, "Değişkenler");

    m_bps = new QListWidget(this);
    connect(m_bps, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* it) {
        emit breakpointToggled(it->data(Qt::UserRole).toString(),
                               it->data(Qt::UserRole + 1).toInt());
    });
    m_tabs->addTab(m_bps, "Kesme Noktaları");

    auto* conTab = new QWidget(this);
    auto* clay = new QVBoxLayout(conTab);
    clay->setContentsMargins(0, 0, 0, 0);
    m_console = new QTextEdit(conTab);
    m_console->setReadOnly(true);
    m_console->setFontFamily("monospace");
    clay->addWidget(m_console, 1);
    m_cmd = new QLineEdit(conTab);
    m_cmd->setPlaceholderText("gdb komutu (örn. print x)...");
    connect(m_cmd, &QLineEdit::returnPressed, this, [this]() {
        const QString t = m_cmd->text().trimmed();
        if (!t.isEmpty()) { emit consoleRequested(t); m_cmd->clear(); }
    });
    clay->addWidget(m_cmd);
    m_tabs->addTab(conTab, "Konsol");
    lay->addWidget(m_tabs, 1);

    setRunning(false, false);
}

void DebugPanel::setRunning(bool running, bool stopped) {
    m_actStart->setEnabled(!running);
    m_actStop->setEnabled(running);
    m_actCont->setEnabled(running && stopped);
    m_actNext->setEnabled(running && stopped);
    m_actStep->setEnabled(running && stopped);
    m_actFinish->setEnabled(running && stopped);
}

void DebugPanel::setFrames(const QList<DebugFrame>& frames) {
    m_stack->clear();
    for (const DebugFrame& f : frames) {
        auto* it = new QTreeWidgetItem(m_stack);
        it->setText(0, QString("#%1").arg(f.level));
        it->setText(1, f.func.isEmpty() ? "??" : f.func);
        it->setText(2, f.file.isEmpty() ? ""
                                        : QString("%1:%2").arg(QFileInfo(f.file).fileName())
                                                          .arg(f.line + 1));
        it->setData(0, Qt::UserRole, f.level);
        it->setToolTip(2, f.file);
    }
    if (m_stack->topLevelItemCount() > 0) m_stack->setCurrentItem(m_stack->topLevelItem(0));
}

void DebugPanel::setVariables(const QList<DebugVar>& vars) {
    m_vars->clear();
    for (const DebugVar& v : vars) {
        auto* it = new QTreeWidgetItem(m_vars);
        it->setText(0, v.name);
        it->setText(1, v.value.left(200));
        it->setText(2, v.type);
    }
}

void DebugPanel::setBreakpoints(const QList<Breakpoint>& bps, const QString& currentFile) {
    m_bps->clear();
    for (const Breakpoint& b : bps) {
        auto* it = new QListWidgetItem(
            QString("%1 %2:%3%4%5").arg(b.enabled ? "●" : "○")
                .arg(QFileInfo(b.file).fileName()).arg(b.line)
                .arg(b.isLogPoint() ? " [log]" : "")
                .arg(b.condition.isEmpty() ? "" : " [koşul]"),
            m_bps);
        it->setData(Qt::UserRole, b.file);
        it->setData(Qt::UserRole + 1, b.line);
        if (b.file == currentFile)
            it->setForeground(palette().highlight().color());
    }
}

void DebugPanel::appendOutput(const QString& text, bool isError) {
    m_console->moveCursor(QTextCursor::End);
    if (isError) {
        m_console->setTextColor(palette().color(QPalette::BrightText).isValid()
                                    ? QColor("#f44747")
                                    : QColor("#f44747"));
    }
    m_console->insertPlainText(text);
    m_console->setTextColor(palette().text().color());
    m_console->moveCursor(QTextCursor::End);
}

void DebugPanel::clearOutput() {
    m_console->clear();
}
