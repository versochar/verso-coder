#pragma once
#include "MiParser.h"
#include <QObject>
#include <QProcess>
#include <functional>

// Stage 14: GDB/MI2 sürücüsü — kesme noktası, çalıştır/adım, yığın, değişken.
// Eşzamanlı kip (komut kuyruğu, token→handler); olaylar sinyal ile gelir.
struct DebugFrame {
    int level = 0;
    QString func;
    QString file;
    int line = 0; // 0-based (-1 bilinmiyor)
    QString addr;
};

struct DebugVar {
    QString name;
    QString value;
    QString type;
    int numChildren = 0;
};

class GdbDriver : public QObject {
    Q_OBJECT
public:
    explicit GdbDriver(QObject* parent = nullptr);
    ~GdbDriver() override;

    bool start(const QString& gdbPath = "gdb"); // gdb --interpreter=mi2
    void quit();
    bool isRunning() const;
    bool isReady() const { return m_ready; }
    bool isDebugging() const { return m_debugging; } // program yüklü/çalışıyor

    // Program yükleme + başlatma
    void launch(const QString& program, const QStringList& args, const QString& cwd,
                bool stopAtEntry, std::function<void(bool)> done);
    // Stage 16: uzak hedef — gdbserver'a bağlan (yerel gdb + `target remote`)
    void targetRemote(const QString& host, int port,
                      std::function<void(bool)> done);
    void launchRemote(const QString& localSymbols, const QString& host, int port,
                      std::function<void(bool)> done);
    static QString gdbserverCmd(int port, const QString& program,
                                const QString& args = QString());
    // Kesme noktaları (gdb number döner)
    void breakInsert(const QString& file, int line1, const QString& cond,
                     std::function<void(QString)> done);
    void breakDelete(const QString& number);
    // Yürütme
    void execRun();
    void execContinue();
    void execNext();
    void execStep();
    void execFinish();
    void interrupt();
    // İnceleme
    void stackFrames(std::function<void(QList<DebugFrame>)> done);
    void stackVariables(int frame, std::function<void(QList<DebugVar>)> done);
    void evaluate(const QString& expr, std::function<void(QString)> done);
    // Ham komut (konsol)
    int command(const QString& mi, std::function<void(QVariantMap)> done);

signals:
    void ready();
    void stopped(const QString& reason, const DebugFrame& frame); // breakpoint-step, end-stepping-range, exited...
    void exited(int code);
    void output(const QString& text);     // hedef program çıktısı (@)
    void consoleMsg(const QString& text); // gdb konsol akışı (~)
    void driverError(const QString& msg);

private slots:
    void onReadyRead();
    void onFinished(int code);

private:
    void send(const QString& mi, std::function<void(QVariantMap)> done);
    void handleRecord(const MiRecord& r);

    QProcess m_proc;
    QByteArray m_buf;
    int m_nextToken = 1;
    bool m_ready = false;
    bool m_debugging = false;
    QMap<int, std::function<void(QVariantMap)>> m_handlers;
};
