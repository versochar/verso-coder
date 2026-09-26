#include "TerminalPanel.h"
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTextCursor>
#include <QTimer>
#include <QVBoxLayout>

TerminalPanel::TerminalPanel(QWidget* parent) : QWidget(parent), m_dir(QDir::homePath()) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);
    auto* top = new QHBoxLayout();
    m_cwd = new QLabel(m_dir, this);
    m_cwd->setStyleSheet("color:#858585;font-size:11px;");
    // Stage 21: dizin takibi
    m_follow = new QPushButton("📁", this);
    m_follow->setCheckable(true);
    m_follow->setToolTip("Etkin dosyanın dizinini takip et");
    m_follow->setFixedWidth(30);
    auto* bBuild = new QPushButton("▶ Derle & Çalıştır", this);
    bBuild->setToolTip("Aktif dosyayı dile göre derler/çalıştırır");
    auto* bKill = new QPushButton("■", this);
    bKill->setFixedWidth(30);
    bKill->setToolTip("Çalışan komutu durdur");
    auto* bClear = new QPushButton("Temizle", this);
    auto* bNew = new QPushButton("＋", this);
    bNew->setFixedWidth(30);
    bNew->setToolTip("Yeni kabuk sekmesi (Stage 30)");
    // Stage 30: kabuk profilleri
    m_shellBox = new QComboBox(this);
    m_shellBox->addItems(shellProfiles());
    m_shellBox->setCurrentText(m_shellProg);
    m_shellBox->setToolTip("Kabuk profili (yeni sekmelere uygulanır)");
    top->addWidget(m_cwd, 1);
    top->addWidget(m_shellBox);
    top->addWidget(m_follow);
    top->addWidget(bBuild);
    top->addWidget(bKill);
    top->addWidget(bClear);
    top->addWidget(bNew);
    m_in = new QLineEdit(this);
    m_in->setPlaceholderText("$ komut yaz... (Enter)");
    m_in->setFont(QFont("Consolas, monospace", 10));

    m_tabs = new QTabWidget(this);
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    lay->addLayout(top);
    lay->addWidget(m_tabs, 1);
    lay->addWidget(m_in);

    connect(m_in, &QLineEdit::returnPressed, this, &TerminalPanel::sendInput);
    connect(bKill, &QPushButton::clicked, this, [this]() {
        Session& s = cur();
        if (s.proc && s.proc->state() != QProcess::NotRunning) {
            s.proc->kill();
            appendOut("\n[killed]\n");
        }
    });
    connect(bClear, &QPushButton::clicked, this,
            [this]() { m_out->clear(); });
    connect(bBuild, &QPushButton::clicked, this, [this]() { emit buildRequested(); });
    connect(bNew, &QPushButton::clicked, this, &TerminalPanel::newSession);
    connect(m_tabs, &QTabWidget::tabCloseRequested, this, &TerminalPanel::closeSession);
    connect(m_tabs, &QTabWidget::currentChanged, this, [this]() {
        // Girdi ve göstergeler etkin oturumu izler
        Session& s = cur();
        m_out = s.out;
        m_cwd->setText(s.dir);
    });
    connect(m_shellBox, &QComboBox::currentTextChanged, this, [this](const QString& t) {
        setShell(t);
    });
    // Stage 21: komut geçmişi (Yukarı/Aşağı)
    m_in->installEventFilter(this);
    loadHistory();

    newSession();
}

TerminalPanel::~TerminalPanel() {
    for (Session& s : m_terms) {
        if (s.proc && s.proc->state() != QProcess::NotRunning) {
            s.proc->write("exit\n");
            s.proc->waitForFinished(500);
        }
        delete s.proc;
    }
}

QStringList TerminalPanel::shellProfiles() {
    QStringList out = {"bash", "sh"};
    for (const QString& c : {"zsh", "fish", "dash"}) {
        QString p = QStandardPaths::findExecutable(c);
        if (!p.isEmpty()) out << c;
    }
    return out;
}

TerminalPanel::Session& TerminalPanel::cur() {
    int i = m_tabs ? m_tabs->currentIndex() : -1;
    if (i < 0 || i >= m_terms.size()) {
        static Session dummy;
        return dummy;
    }
    return m_terms[i];
}

const TerminalPanel::Session& TerminalPanel::cur() const {
    int i = m_tabs ? m_tabs->currentIndex() : -1;
    if (i < 0 || i >= m_terms.size()) {
        static const Session dummy;
        return dummy;
    }
    return m_terms[i];
}

void TerminalPanel::newSession() {
    Session s;
    s.dir = m_dir;
    s.proc = new QProcess(this);
    s.out = new QTextEdit(m_tabs);
    s.out->setReadOnly(true);
    s.out->setFont(QFont("Consolas, monospace", 10));
    connect(s.proc, &QProcess::readyReadStandardOutput, this, &TerminalPanel::readOutput);
    connect(s.proc, &QProcess::readyReadStandardError, this, &TerminalPanel::readOutput);
    connect(s.proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalPanel::onFinished);
    m_terms << s;
    m_tabs->addTab(s.out, QString("kabuk %1").arg(m_terms.size()));
    m_tabs->setCurrentIndex(m_tabs->count() - 1);
    m_out = s.out;
    m_cwd->setText(s.dir);
    ensureShell();
}

void TerminalPanel::closeSession(int i) {
    if (m_terms.size() <= 1) {
        if (m_out) m_out->clear(); // son sekmeyi kapatma, temizle
        return;
    }
    if (i < 0 || i >= m_terms.size()) return;
    Session s = m_terms.takeAt(i);
    if (s.proc) {
        if (s.proc->state() != QProcess::NotRunning) {
            s.proc->write("exit\n");
            s.proc->waitForFinished(500);
        }
        s.proc->deleteLater();
    }
    if (s.out) s.out->deleteLater();
    m_tabs->removeTab(i);
    Session& c = cur();
    m_out = c.out;
    if (m_out) m_cwd->setText(c.dir);
}

int TerminalPanel::sessionCount() const { return m_terms.size(); }

void TerminalPanel::appendOutTo(Session& s, const QString& t, const QColor& c) {
    if (!s.out) return;
    if (!c.isValid()) {
        s.out->moveCursor(QTextCursor::End);
        s.out->insertPlainText(t);
    } else {
        s.out->moveCursor(QTextCursor::End);
        QTextCharFormat f;
        f.setForeground(c);
        QTextCursor cur = s.out->textCursor();
        cur.setCharFormat(f);
        cur.insertText(t);
    }
    s.out->moveCursor(QTextCursor::End);
}

void TerminalPanel::appendOut(const QString& t, const QColor& c) {
    appendOutTo(cur(), t, c);
}

// Stage 29: kabuğa metin gönder (eklenti katkısı)
void TerminalPanel::sendText(const QString& text) {
    Session& s = cur();
    ensureShell(s);
    appendOut("$ " + text.trimmed().left(200) + "\n", QColor("#569cd6"));
    if (s.proc && s.proc->state() != QProcess::NotRunning)
        s.proc->write((text.trimmed() + "\n").toUtf8());
}

void TerminalPanel::setWorkdir(const QString& dir) {
    m_dir = dir;
    Session& s = cur();
    s.dir = dir;
    m_cwd->setText(dir);
    if (s.proc && s.proc->state() != QProcess::NotRunning)
        s.proc->write(("cd " + dir + "\n").toUtf8());
}

void TerminalPanel::ensureShell() { ensureShell(cur()); }

void TerminalPanel::ensureShell(Session& s) {
    if (!s.proc || s.proc->state() != QProcess::NotRunning || s.starting) return;
    s.starting = true;
    s.proc->setWorkingDirectory(s.dir.isEmpty() ? m_dir : s.dir);
    QString prog = m_shellProg.isEmpty() ? "bash" : m_shellProg;
    s.proc->start(prog, (prog == "bash") ? QStringList({"--noprofile", "--norc"}) : QStringList());
    s.proc->waitForStarted(3000);
    s.starting = false;
    if (s.proc->state() != QProcess::NotRunning)
        s.proc->write(("cd " + (s.dir.isEmpty() ? m_dir : s.dir) + "\n").toUtf8());
    else
        appendOutTo(s, "[terminal başlatılamadı: " + prog + " yok?]\n", QColor("#f44747"));
}

void TerminalPanel::setShell(const QString& prog) {
    if (prog == m_shellProg) return;
    m_shellProg = prog;
    // Çalışan oturumu yeniden başlat
    Session& s = cur();
    if (s.proc && s.proc->state() != QProcess::NotRunning) {
        s.proc->write("exit\n");
        s.proc->waitForFinished(500);
    }
    ensureShell(s);
}

void TerminalPanel::sendInput() {
    QString c = m_in->text();
    if (c.isEmpty()) return;
    pushHistory(c);
    Session& s = cur();
    ensureShell(s);
    appendOut("$ " + c + "\n", QColor("#569cd6"));
    m_in->clear();
    if (s.proc) s.proc->write((c + "\n").toUtf8());
}

void TerminalPanel::readOutput() {
    auto* p = qobject_cast<QProcess*>(sender());
    if (!p) return;
    const QString t = QString::fromUtf8(p->readAllStandardOutput()) +
                      QString::fromUtf8(p->readAllStandardError());
    for (Session& s : m_terms) {
        if (s.proc == p) {
            appendOutTo(s, t);
            break;
        }
    }
}

void TerminalPanel::onFinished(int code) {
    Q_UNUSED(code);
    auto* p = qobject_cast<QProcess*>(sender());
    for (Session& s : m_terms) {
        if (s.proc != p) continue;
        appendOutTo(s, QString("\n[çıkış %1 — yeniden başlatmak için Enter]\n").arg(code));
        // Otomatik yeniden başlat
        QTimer::singleShot(300, this, [this, p]() {
            for (Session& s2 : m_terms)
                if (s2.proc == p) ensureShell(s2);
        });
        break;
    }
}

QString TerminalPanel::buildCommand(const QString& suffix, const QString& filePath) {
    QString q = "\"" + filePath + "\"";
    QString out = "\"/tmp/verso-run/" + QFileInfo(filePath).completeBaseName() + "\"";
    QString s = suffix.toLower();
    if (s == "py") return "python3 " + q;
    if (s == "sh") return "bash " + q;
    if (s == "js") return "node " + q;
    if (s == "c") return QString("mkdir -p /tmp/verso-run && gcc %1 -o %2 && %2").arg(q, out);
    if (s == "cpp" || s == "cc" || s == "cxx")
        return QString("mkdir -p /tmp/verso-run && g++ -std=c++17 %1 -o %2 && %2").arg(q, out);
    return {};
}

void TerminalPanel::runBuildFor(const QString& filePath) {
    if (filePath.isEmpty()) { appendOut("[önce bir dosya aç]\n"); return; }
    QString cmd = buildCommand(QFileInfo(filePath).suffix(), filePath);
    if (cmd.isEmpty()) { appendOut("[bu dil için derleme şablonu yok: " + QFileInfo(filePath).suffix() + "]\n"); return; }
    Session& s = cur();
    ensureShell(s);
    appendOut("$ " + cmd + "\n", QColor("#569cd6"));
    if (s.proc) s.proc->write((cmd + "\necho [exit:$?]\n").toUtf8());
}

// Stage 21: komut geçmişi (kalıcı, en çok 100)
void TerminalPanel::loadHistory() {
    m_hist = QSettings("Verso", "VersoCoder").value("terminal/history").toStringList();
    m_histPos = m_hist.size();
}

void TerminalPanel::pushHistory(const QString& c) {
    m_hist.removeAll(c);
    m_hist << c;
    while (m_hist.size() > 100) m_hist.removeFirst();
    m_histPos = m_hist.size();
    QSettings("Verso", "VersoCoder").setValue("terminal/history", m_hist);
}

bool TerminalPanel::eventFilter(QObject* o, QEvent* e) {
    if (o == m_in && e->type() == QEvent::KeyPress) {
        auto* k = static_cast<QKeyEvent*>(e);
        if (k->key() == Qt::Key_Up || k->key() == Qt::Key_Down) {
            if (m_hist.isEmpty()) return false;
            m_histPos += (k->key() == Qt::Key_Up) ? -1 : 1;
            m_histPos = qBound(0, m_histPos, m_hist.size());
            m_in->setText(m_histPos < m_hist.size() ? m_hist[m_histPos] : QString());
            return true;
        }
    }
    return QWidget::eventFilter(o, e);
}

// Stage 21: etkin dosya dizinini takip et (açıkken dizin değişince cd)
void TerminalPanel::syncToDir(const QString& dir) {
    if (!m_follow || !m_follow->isChecked()) return;
    if (dir.isEmpty()) return;
    Session& s = cur();
    if (dir == s.dir) return;
    s.dir = dir;
    m_dir = dir;
    m_cwd->setText(dir);
    if (s.proc && s.proc->state() != QProcess::NotRunning)
        s.proc->write(("cd " + dir + "\n").toUtf8());
}

bool TerminalPanel::followDir() const {
    return m_follow && m_follow->isChecked();
}

// Stage 28: çıkışta bul (tüm eşleşmeler sarı, etkin olan turuncu)
bool TerminalPanel::findFirst(const QString& text) {
    m_findText = text;
    QList<QTextEdit::ExtraSelection> extra;
    if (!text.isEmpty() && m_out) {
        QTextCharFormat all;
        all.setBackground(QColor("#665c1e"));
        QTextCharFormat cur;
        cur.setBackground(QColor("#b8860b"));
        cur.setForeground(QColor("#000000"));
        QTextDocument* doc = m_out->document();
        QTextCursor c(doc);
        bool first = true;
        int n = 0;
        while (!c.isNull() && !c.atEnd() && n < 500) {
            c = doc->find(text, c);
            if (c.isNull()) break;
            QTextEdit::ExtraSelection s;
            s.cursor = c;
            s.format = first ? cur : all;
            extra << s;
            first = false;
            ++n;
        }
        m_out->setExtraSelections(extra);
    } else if (m_out) {
        m_out->setExtraSelections({});
    }
    return findNext(true);
}

bool TerminalPanel::findNext(bool forward) {
    if (m_findText.isEmpty() || !m_out) return false;
    QTextDocument::FindFlags fl = forward ? QTextDocument::FindFlags()
                                          : QTextDocument::FindBackward;
    QTextCursor anchor = m_out->textCursor();
    QTextCursor c = m_out->document()->find(m_findText, anchor, fl);
    if (c.isNull()) { // başa/sana sar
        QTextCursor edge(m_out->document());
        if (!forward) edge.movePosition(QTextCursor::End);
        c = m_out->document()->find(m_findText, edge, fl);
    }
    if (c.isNull()) return false;
    m_out->setTextCursor(c);
    return true;
}
