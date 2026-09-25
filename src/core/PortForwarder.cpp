#include "PortForwarder.h"
#include "SshSession.h"

QString ForwardRule::spec() const {
    return QString("%1:%2:%3").arg(localPort).arg(targetHost).arg(targetPort);
}

PortForwarder::PortForwarder(QObject* parent) : QObject(parent) {}

QStringList PortForwarder::forwardArgs(const ConnectionProfile& p, const ForwardRule& rule) {
    QStringList a = SshSession::sshBaseArgs(p);
    a << "-N" // komut çalıştırma, sadece yönlendirme
      << "-L" << rule.spec()
      << SshSession::sshTarget(p);
    return a;
}

bool PortForwarder::start(const ForwardRule& rule) {
    if (!rule.valid() || !m_profile.isValid() || m_live.contains(rule.label))
        return false;
    auto* p = new QProcess(this);
    p->start("ssh", forwardArgs(m_profile, rule));
    if (!p->waitForStarted(8000)) {
        p->deleteLater();
        emit tunnelDown(rule.label, "ssh başlatılamadı");
        return false;
    }
    Live l;
    l.rule = rule;
    l.proc = p;
    m_live[rule.label] = l;
    connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, label = rule.label](int code, QProcess::ExitStatus) {
                m_live.remove(label);
                emit tunnelDown(label, QString("ssh kapandı (kod %1)").arg(code));
            });
    emit tunnelUp(rule.label);
    return true;
}

void PortForwarder::stop(const QString& label) {
    auto it = m_live.find(label);
    if (it == m_live.end()) return;
    if (it->proc) {
        it->proc->terminate();
        if (!it->proc->waitForFinished(1500)) it->proc->kill();
        it->proc->deleteLater();
    }
    m_live.erase(it);
    emit tunnelDown(label, "kapatıldı");
}

void PortForwarder::stopAll() {
    for (const QString& k : m_live.keys()) stop(k);
}

bool PortForwarder::isActive(const QString& label) const {
    return m_live.contains(label);
}

QList<ForwardRule> PortForwarder::active() const {
    QList<ForwardRule> out;
    for (const auto& l : m_live) out << l.rule;
    return out;
}
