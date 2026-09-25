#pragma once
#include "ConnectionProfile.h"
#include <QObject>
#include <QProcess>

// Stage 16: port yönlendirme — `ssh -L yerelkaynak:hedef` tünel yöneticisi.
struct ForwardRule {
    int localPort = 0;       // yerelde dinlenen port
    QString targetHost;      // uzaktan erişilen host (uzak bakış açısından)
    int targetPort = 0;
    QString label;           // görünen ad
    bool valid() const { return localPort > 0 && targetPort > 0 && !targetHost.isEmpty(); }
    QString spec() const;    // "yerel:hedef:hedefport" ssh -L biçimi
};

class PortForwarder : public QObject {
    Q_OBJECT
public:
    explicit PortForwarder(QObject* parent = nullptr);

    void setProfile(const ConnectionProfile& p) { m_profile = p; }
    // Tünel başlat/durdur (tek kural = tek ssh süreci)
    bool start(const ForwardRule& rule);
    void stop(const QString& label);
    void stopAll();
    bool isActive(const QString& label) const;
    QList<ForwardRule> active() const;

    // Saf: ssh -L argüman kurma (test edilebilir)
    static QStringList forwardArgs(const ConnectionProfile& p, const ForwardRule& rule);

signals:
    void tunnelUp(const QString& label);
    void tunnelDown(const QString& label, const QString& reason);

private:
    ConnectionProfile m_profile;
    struct Live { ForwardRule rule; QProcess* proc = nullptr; };
    QMap<QString, Live> m_live;
};
