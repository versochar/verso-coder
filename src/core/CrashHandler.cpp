#include "CrashHandler.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <csignal>

#ifdef Q_OS_LINUX
#include <execinfo.h>
#include <unistd.h>
#endif

QString CrashHandler::crashDir() {
    const QString d =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/crashes";
    QDir().mkpath(d);
    return d;
}

QString CrashHandler::dumpName() {
    return QString("crash-%1-%2.log")
        .arg(QCoreApplication::applicationPid())
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss"));
}

bool CrashHandler::writeDump(const QString& path, const QString& reason) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    QString out = QString("Verso Coder çökme izi\nZaman: %1\nNeden: %2\n\n")
                      .arg(QDateTime::currentDateTime().toString(Qt::ISODate), reason);
#ifdef Q_OS_LINUX
    void* frames[64];
    const int n = backtrace(frames, 64);
    char** syms = backtrace_symbols(frames, n);
    for (int i = 0; i < n; ++i) out += QString::fromLatin1(syms[i]) + "\n";
    free(syms);
#else
    out += "(iz düşümü bu platformda yok)\n";
#endif
    f.write(out.toUtf8());
    return true;
}

static void crashSigHandler(int sig) {
    const QString reason = (sig == SIGSEGV) ? "SIGSEGV" : (sig == SIGABRT) ? "SIGABRT" : QString("sinyal %1").arg(sig);
    CrashHandler::writeDump(CrashHandler::crashDir() + "/" + CrashHandler::dumpName(),
                            reason);
    // Varsayılan davranışa dön + yeniden yükselt (çekirdek dökümü için)
    signal(sig, SIG_DFL);
    raise(sig);
}

void CrashHandler::install() {
    signal(SIGSEGV, crashSigHandler);
    signal(SIGABRT, crashSigHandler);
}

void CrashHandler::uninstall() {
    signal(SIGSEGV, SIG_DFL);
    signal(SIGABRT, SIG_DFL);
}

QStringList CrashHandler::pendingDumps() {
    QDir d(crashDir());
    return d.entryList({"crash-*.log"}, QDir::Files, QDir::Time);
}

void CrashHandler::clearDumps() {
    QDir d(crashDir());
    for (const QString& f : d.entryList({"crash-*.log"}, QDir::Files)) d.remove(f);
}
