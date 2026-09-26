#include "DebugPanel.h"
#include "../core/BreakpointStore.h"
#include <QAction>
#include <QCompleter>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
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
    m_vars->setToolTip("Değeri değiştirmek için çift tıklayın");
    connect(m_vars, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem* it, int) {
                if (it && !it->text(0).isEmpty()) emit variableEditRequested(it->text(0));
            });
    vlay->addWidget(m_vars, 1);
    m_eval = new QLineEdit(varTab);
    m_eval->setPlaceholderText("İfade değerlendir (Enter)...");
    connect(m_eval, &QLineEdit::returnPressed, this, [this]() {
        const QString t = m_eval->text().trimmed();
        if (!t.isEmpty()) emit evaluateRequested(t);
    });
    vlay->addWidget(m_eval);
    m_tabs->addTab(varTab, "Değişkenler");

    // Stage 26: izleme (watch) sekmesi
    auto* watchTab = new QWidget(this);
    auto* wlay = new QVBoxLayout(watchTab);
    wlay->setContentsMargins(0, 0, 0, 0);
    m_watch = new QTreeWidget(watchTab);
    m_watch->setHeaderLabels({"İfade", "Değer"});
    m_watch->setRootIsDecorated(false);
    wlay->addWidget(m_watch, 1);
    auto* wrow = new QHBoxLayout();
    m_watchEdit = new QLineEdit(watchTab);
    m_watchEdit->setPlaceholderText("İzlenecek ifade (Enter)...");
    auto* wAdd = new QPushButton("Ekle", watchTab);
    auto* wDel = new QPushButton("Sil", watchTab);
    wrow->addWidget(m_watchEdit, 1);
    wrow->addWidget(wAdd);
    wrow->addWidget(wDel);
    wlay->addLayout(wrow);
    connect(m_watchEdit, &QLineEdit::returnPressed, this, [this]() {
        const QString t = m_watchEdit->text().trimmed();
        if (!t.isEmpty()) { emit watchAddRequested(t); m_watchEdit->clear(); }
    });
    connect(wAdd, &QPushButton::clicked, m_watchEdit, &QLineEdit::returnPressed);
    connect(wDel, &QPushButton::clicked, this, [this]() {
        auto* it = m_watch->currentItem();
        if (it) emit watchRemoveRequested(it->data(0, Qt::UserRole).toString());
    });
    m_tabs->addTab(watchTab, "İzleme");

    m_bps = new QListWidget(this);
    connect(m_bps, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* it) {
        emit breakpointToggled(it->data(Qt::UserRole).toString(),
                               it->data(Qt::UserRole + 1).toInt());
    });
    m_tabs->addTab(m_bps, "Kesme Noktaları");

    // Stage 26: yazmaçlar
    m_regs = new QTreeWidget(this);
    m_regs->setHeaderLabels({"Yazmaç", "Değer"});
    m_regs->setRootIsDecorated(false);
    m_tabs->addTab(m_regs, "Yazmaçlar");

    // Stage 26: bellek görüntüleyici
    auto* memTab = new QWidget(this);
    auto* mlay = new QVBoxLayout(memTab);
    mlay->setContentsMargins(0, 0, 0, 0);
    auto* mrow = new QHBoxLayout();
    m_memAddr = new QLineEdit(memTab);
    m_memAddr->setPlaceholderText("Adres (örn. $sp, 0x... )");
    auto* mGo = new QPushButton("Oku", memTab);
    mrow->addWidget(m_memAddr, 1);
    mrow->addWidget(mGo);
    mlay->addLayout(mrow);
    m_mem = new QTextEdit(memTab);
    m_mem->setReadOnly(true);
    m_mem->setFontFamily("monospace");
    mlay->addWidget(m_mem, 1);
    connect(mGo, &QPushButton::clicked, this, [this]() {
        const QString a = m_memAddr->text().trimmed();
        if (!a.isEmpty()) emit memoryReadRequested(a);
    });
    connect(m_memAddr, &QLineEdit::returnPressed, mGo, &QPushButton::click);
    m_tabs->addTab(memTab, "Bellek");

    // Stage 26: disassembly
    m_disas = new QTreeWidget(this);
    m_disas->setHeaderLabels({"Adres", "Fonksiyon", "Komut"});
    m_disas->setRootIsDecorated(false);
    m_tabs->addTab(m_disas, "Disas");

    // Stage 26: iş parçacıkları
    m_threads = new QListWidget(this);
    connect(m_threads, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* it) {
        emit threadSelected(it->data(Qt::UserRole).toString());
    });
    m_tabs->addTab(m_threads, "Threads");

    auto* conTab = new QWidget(this);
    auto* clay = new QVBoxLayout(conTab);
    clay->setContentsMargins(0, 0, 0, 0);
    m_console = new QTextEdit(conTab);
    m_console->setReadOnly(true);
    m_console->setFontFamily("monospace");
    clay->addWidget(m_console, 1);
    m_cmd = new QLineEdit(conTab);
    m_cmd->setPlaceholderText("gdb komutu (örn. print x)...");
    // Stage 26: geçmiş (Yukarı/Aşağı) + komut tamamlama
    static const QStringList gdbWords = {"print ", "info ", "info registers", "info threads",
        "info breakpoints", "backtrace", "frame ", "break ", "watch ", "delete ",
        "continue", "next", "step", "finish", "until", "call ", "set variable ",
        "x/", "disassemble", "thread ", "help "};
    auto* comp = new QCompleter(gdbWords, m_cmd);
    comp->setCaseSensitivity(Qt::CaseInsensitive);
    m_cmd->setCompleter(comp);
    m_cmd->installEventFilter(this);
    connect(m_cmd, &QLineEdit::returnPressed, this, [this]() {
        const QString t = m_cmd->text().trimmed();
        if (!t.isEmpty()) {
            m_cmdHist.removeAll(t);
            m_cmdHist.prepend(t);
            while (m_cmdHist.size() > 50) m_cmdHist.removeLast();
            m_histPos = -1;
            emit consoleRequested(t);
            m_cmd->clear();
        }
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

// Stage 26: konsol geçmişi (Yukarı/Aşağı)
bool DebugPanel::eventFilter(QObject* o, QEvent* e) {
    if (o == m_cmd && e->type() == QEvent::KeyPress) {
        auto* k = static_cast<QKeyEvent*>(e);
        if ((k->key() == Qt::Key_Up || k->key() == Qt::Key_Down) && !m_cmdHist.isEmpty()) {
            if (k->key() == Qt::Key_Up) m_histPos = qMin(m_histPos + 1, m_cmdHist.size() - 1);
            else m_histPos = qMax(m_histPos - 1, -1);
            m_cmd->setText(m_histPos < 0 ? QString() : m_cmdHist[m_histPos]);
            return true;
        }
    }
    return QWidget::eventFilter(o, e);
}

void DebugPanel::setWatches(const QList<QPair<QString, QString>>& items) {
    m_watch->clear();
    for (const auto& it : items) {
        auto* r = new QTreeWidgetItem(m_watch, {it.first, it.second});
        r->setData(0, Qt::UserRole, it.first);
    }
}

void DebugPanel::setRegisters(const QList<QPair<QString, QString>>& regs) {
    m_regs->clear();
    for (const auto& r : regs) new QTreeWidgetItem(m_regs, {r.first, r.second});
}

void DebugPanel::setMemory(const QString& addr, const QString& dump) {
    m_mem->setPlainText(addr + "\n" + dump);
}

void DebugPanel::setDisas(const QList<QMap<QString, QString>>& rows) {
    m_disas->clear();
    for (const auto& r : rows)
        new QTreeWidgetItem(m_disas, {r.value("address"), r.value("func"), r.value("inst")});
}

void DebugPanel::setThreads(const QList<QPair<QString, QString>>& threads,
                            const QString& current) {
    m_threads->clear();
    for (const auto& t : threads) {
        auto* it = new QListWidgetItem(t.second, m_threads);
        it->setData(Qt::UserRole, t.first);
        if (t.first == current) {
            QFont f = it->font();
            f.setBold(true);
            it->setFont(f);
            m_threads->setCurrentItem(it);
        }
    }
}
