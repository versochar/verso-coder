#pragma once
#include <QString>
#include <QStringList>

// Komut satırı seçenekleri (Stage 8).
struct StartupOptions {
    QStringList paths;      // açılacak dosya/klasörler
    int line = 0;           // --line N
    QString command;        // --command <id> (açılışta komut çalıştır)
    bool isNew = false;     // --new (boş pencere)
    bool wait = false;      // --wait
    bool noRestore = false; // --no-restore (oturum geri yükleme kapalı)
    bool help = false;      // --help / -h
    bool version = false;   // --version / -v
};

class StartupArgs {
public:
    // args: program adı hariç argümanlar (argv[1..]).
    static StartupOptions parse(const QStringList& args);
    static QString helpText();
};
