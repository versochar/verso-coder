#include "SshConfig.h"
#include <QDir>
#include <QFile>
#include <QRegularExpression>

QList<SshConfig::Entry> SshConfig::parse(const QString& text) {
    QList<SshConfig::Entry> out;
    Entry cur;
    bool inHost = false;
    auto flush = [&]() {
        if (inHost && !cur.host.isEmpty() && !cur.host.contains('*') &&
            cur.host != "*") {
            if (cur.hostName.isEmpty()) cur.hostName = cur.host;
            out << cur;
        }
        cur = Entry();
        inHost = false;
    };
    for (const QString& raw : text.split('\n')) {
        QString ln = raw.trimmed();
        if (ln.isEmpty() || ln.startsWith('#')) continue;
        // "Anahtar Değer" ya da "Anahtar=Değer"
        QString key, val;
        const int eq = ln.indexOf('=');
        const int sp = ln.indexOf(QRegularExpression(R"(\s)"));
        if (eq > 0 && (sp < 0 || eq < sp)) {
            key = ln.left(eq).trimmed();
            val = ln.mid(eq + 1).trimmed();
        } else if (sp > 0) {
            key = ln.left(sp).trimmed();
            val = ln.mid(sp).trimmed();
        } else {
            continue;
        }
        if (key.compare("Host", Qt::CaseInsensitive) == 0) {
            flush();
            cur.host = val.split(QRegularExpression(R"(\s)"), Qt::SkipEmptyParts).value(0);
            inHost = true;
        } else if (!inHost) {
            continue;
        } else if (key.compare("HostName", Qt::CaseInsensitive) == 0) {
            cur.hostName = val;
        } else if (key.compare("User", Qt::CaseInsensitive) == 0) {
            cur.user = val;
        } else if (key.compare("Port", Qt::CaseInsensitive) == 0) {
            cur.port = val.toInt() > 0 ? val.toInt() : 22;
        } else if (key.compare("IdentityFile", Qt::CaseInsensitive) == 0) {
            if (cur.identityFile.isEmpty())
                cur.identityFile = val.split(QRegularExpression(R"(\s)")).value(0);
        }
    }
    flush();
    return out;
}

QList<SshConfig::Entry> SshConfig::parseFile(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    return parse(QString::fromUtf8(f.readAll()));
}

ConnectionProfile SshConfig::toProfile(const Entry& e) {
    ConnectionProfile p;
    p.name = e.host;
    p.host = e.hostName;
    p.port = e.port;
    p.user = e.user;
    p.keyPath = e.identityFile.startsWith('~')
        ? QDir::homePath() + e.identityFile.mid(1)
        : e.identityFile;
    return p;
}
