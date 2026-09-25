#include "RemoteTerminal.h"
#include "../core/SshSession.h"
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QVBoxLayout>

RemoteTerminal::RemoteTerminal(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    m_out = new QPlainTextEdit(this);
    m_out->setReadOnly(true);
    m_out->setFont(QFont("monospace", 10));
    lay->addWidget(m_out, 1);
    m_in = new QLineEdit(this);
    m_in->setPlaceholderText("uzak komut (Enter gönderir, Ctrl+C için: \\x03 yazın)...");
    m_in->setFont(QFont("monospace", 10));
    lay->addWidget(m_in);
    connect(m_in, &QLineEdit::returnPressed, this, &RemoteTerminal::sendInput);
    connect(&m_proc, &QProcess::readyReadStandardOutput, this, &RemoteTerminal::readOutput);
    connect(&m_proc, &QProcess::readyReadStandardError, this, &RemoteTerminal::readOutput);
    connect(&m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus) { onFinished(code); });
}

RemoteTerminal::~RemoteTerminal() { stop(); }

bool RemoteTerminal::start(const ConnectionProfile& p) {
    stop();
    // Etkileşimli kabuk: BatchMode YOK (parola/2FA'ya izin ver), -t pty
    QStringList args;
    if (p.port != 22 && p.port > 0) args << "-p" << QString::number(p.port);
    if (!p.keyPath.trimmed().isEmpty()) args << "-i" << p.keyPath.trimmed();
    if (!p.jumpHost.trimmed().isEmpty()) args << "-J" << p.jumpHost.trimmed();
    args << "-o" << "ConnectTimeout=10" << "-o" << "ServerAliveInterval=30"
         << "-o" << "ServerAliveCountMax=3";
    args << "-t" << SshSession::sshTarget(p)
         << SshSession::remoteShellCmd(p, "${SHELL:-bash} -l", p.remoteRoot);
    m_proc.start("ssh", args);
    if (!m_proc.waitForStarted(10000)) {
        appendOut("[uzak terminal başlatılamadı]\n");
        emit statusMessage("Uzak terminal açılamadı.");
        return false;
    }
    appendOut(QString("[uzak kabuk: %1]\n").arg(p.display()));
    emit statusMessage("Uzak terminal açık.");
    return true;
}

void RemoteTerminal::stop() {
    if (m_proc.state() != QProcess::NotRunning) {
        m_proc.terminate();
        if (!m_proc.waitForFinished(1200)) m_proc.kill();
    }
}

bool RemoteTerminal::isRunning() const {
    return m_proc.state() != QProcess::NotRunning;
}

void RemoteTerminal::readOutput() {
    appendOut(QString::fromUtf8(m_proc.readAllStandardOutput()));
    appendOut(QString::fromUtf8(m_proc.readAllStandardError()));
}

void RemoteTerminal::sendInput() {
    const QString t = m_in->text();
    m_in->clear();
    if (m_proc.state() == QProcess::NotRunning) {
        appendOut("[bağlantı kapalı]\n");
        return;
    }
    // "\x03" yazımı gerçek Ctrl+C'ye çevrilir
    QString send = (t == "\\x03") ? QString(QChar(3)) : (t + "\n");
    m_proc.write(send.toUtf8());
}

void RemoteTerminal::onFinished(int code) {
    appendOut(QString("\n[uzak kabuk kapandı: kod %1]\n").arg(code));
    emit statusMessage("Uzak terminal kapandı.");
}

void RemoteTerminal::appendOut(const QString& t) {
    m_out->moveCursor(QTextCursor::End);
    m_out->insertPlainText(t);
    m_out->moveCursor(QTextCursor::End);
}
