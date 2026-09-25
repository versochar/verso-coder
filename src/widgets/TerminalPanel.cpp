#include "TerminalPanel.h"
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QVBoxLayout>

TerminalPanel::TerminalPanel(QWidget* parent) : QWidget(parent), m_dir(QDir::homePath()) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);
    auto* top = new QHBoxLayout();
    m_cwd = new QLabel(m_dir, this);
    m_cwd->setStyleSheet("color:#858585;font-size:11px;");
    auto* bBuild = new QPushButton("▶ Derle & Çalıştır", this);
    bBuild->setToolTip("Aktif dosyayı dile göre derler/çalıştırır");
    auto* bKill = new QPushButton("■", this);
    bKill->setFixedWidth(30);
    bKill->setToolTip("Çalışan komutu durdur");
    auto* bClear = new QPushButton("Temizle", this);
    top->addWidget(m_cwd, 1);
    top->addWidget(bBuild);
    top->addWidget(bKill);
    top->addWidget(bClear);

    m_out = new QTextEdit(this);
    m_out->setReadOnly(true);
    m_out->setFont(QFont("Consolas, monospace", 10));
    m_in = new QLineEdit(this);
    m_in->setPlaceholderText("$ komut yaz... (Enter)");
    m_in->setFont(QFont("Consolas, monospace", 10));

    lay->addLayout(top);
    lay->addWidget(m_out, 1);
    lay->addWidget(m_in);

    connect(m_in, &QLineEdit::returnPressed, this, &TerminalPanel::sendInput);
    connect(&m_shell, &QProcess::readyReadStandardOutput, this, &TerminalPanel::readOutput);
    connect(&m_shell, &QProcess::readyReadStandardError, this, &TerminalPanel::readOutput);
    connect(&m_shell, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalPanel::onFinished);
    connect(bKill, &QPushButton::clicked, this, [this]() {
        if (m_shell.state() != QProcess::NotRunning) { m_shell.kill(); appendOut("\n[killed]\n"); }
    });
    connect(bClear, &QPushButton::clicked, m_out, &QTextEdit::clear);
    connect(bBuild, &QPushButton::clicked, this, [this]() { emit buildRequested(); });

    ensureShell();
}

TerminalPanel::~TerminalPanel() {
    if (m_shell.state() != QProcess::NotRunning) {
        m_shell.write("exit\n");
        m_shell.waitForFinished(500);
    }
}

void TerminalPanel::setWorkdir(const QString& dir) {
    m_dir = dir;
    m_cwd->setText(dir);
    if (m_shell.state() != QProcess::NotRunning)
        m_shell.write(("cd " + dir + "\n").toUtf8());
}

void TerminalPanel::ensureShell() {
    if (m_shell.state() != QProcess::NotRunning || m_shellStarting) return;
    m_shellStarting = true;
    m_shell.setWorkingDirectory(m_dir);
    QString prog = m_shellProg.isEmpty() ? "bash" : m_shellProg;
    m_shell.start(prog, (prog == "bash") ? QStringList({"--noprofile", "--norc"}) : QStringList());
    m_shell.waitForStarted(3000);
    m_shellStarting = false;
    if (m_shell.state() != QProcess::NotRunning)
        m_shell.write(("cd " + m_dir + "\n").toUtf8());
    else
        appendOut("[terminal başlatılamadı: " + (m_shellProg.isEmpty() ? "bash" : m_shellProg) + " yok?]\n", QColor("#f44747"));
}

void TerminalPanel::setShell(const QString& prog) {
    if (prog == m_shellProg) return;
    m_shellProg = prog;
    if (m_shell.state() != QProcess::NotRunning) {
        m_shell.write("exit\n");
        m_shell.waitForFinished(500);
    }
    ensureShell();
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
    ensureShell();
    appendOut("$ " + cmd + "\n", QColor("#569cd6"));
    m_shell.write((cmd + "\necho [exit:$?]\n").toUtf8());
}

void TerminalPanel::sendInput() {
    QString c = m_in->text();
    if (c.isEmpty()) return;
    ensureShell();
    appendOut("$ " + c + "\n", QColor("#569cd6"));
    m_in->clear();
    m_shell.write((c + "\n").toUtf8());
}

void TerminalPanel::readOutput() {
    QString t = QString::fromUtf8(m_shell.readAllStandardOutput()) +
                QString::fromUtf8(m_shell.readAllStandardError());
    appendOut(t);
}

void TerminalPanel::onFinished(int) {
    appendOut("\n[shell kapandı — yeniden başlatılıyor]\n");
    ensureShell();
}

void TerminalPanel::appendOut(const QString& t, const QColor& c) {
    QTextCursor cur = m_out->textCursor();
    cur.movePosition(QTextCursor::End);
    QTextCharFormat f;
    if (c.isValid()) f.setForeground(c);
    cur.setCharFormat(f);
    cur.insertText(t);
    m_out->setTextCursor(cur);
    m_out->ensureCursorVisible();
}
