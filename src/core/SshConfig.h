#pragma once
#include <QList>
#include <QString>

#include "ConnectionProfile.h"

// Stage 30: ~/.ssh/config içe aktarma — Host kayıtları → profil.
// Saf ayrıştırma (test edilebilir).
class SshConfig {
public:
    struct Entry {
        QString host; // ilk ad (jokerli kayıtlar atlanır)
        QString hostName;
        QString user;
        int port = 22;
        QString identityFile;
    };
    static QList<Entry> parse(const QString& text);
    static QList<Entry> parseFile(const QString& path);
    static ConnectionProfile toProfile(const Entry& e);
};
