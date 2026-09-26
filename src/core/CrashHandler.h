#pragma once
#include <QString>
#include <QStringList>

// Stage 31: çökme yakalayıcı — SIGSEGV/SIGABRT'te iz düşümü yazar.
// Opt-in (ayar crashReport); rapor taslağı sonraki açılışta önerilir.
class CrashHandler {
public:
    static QString crashDir();
    static void install();   // sinyal yakalayıcıları kur
    static void uninstall();
    // Test edilebilir yardımcılar
    static QString dumpName(); // "crash-<pid>-<zaman>.log"
    static bool writeDump(const QString& path, const QString& reason);
    static QStringList pendingDumps(); // gönderilmemiş dökümler
    static void clearDumps();
};
