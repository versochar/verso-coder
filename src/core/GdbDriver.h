#pragma once
#include "MiParser.h"
#include <QMap>
#include <QObject>
#include <QPair>
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
    // Stage 26: koşul/hit-count/sonradan koşul
    void breakCondition(const QString& number, const QString& cond);
    void breakAfter(const QString& number, int count);
    void breakFunction(const QString& func, std::function<void(QString)> done);
    void breakWatch(const QString& expr, const QString& access,
                    std::function<void(QString)> done); // access: "" | "r" | "rw"
    // Yürütme
    void execRun();
    void execContinue();
    void execNext();
    void execStep();
    void execFinish();
    void execUntil(const QString& file, int line); // imlece kadar çalıştır
    static QString untilCommand(const QString& file, int line);
    void execStepInstruction(); // Stage 26: disas adımı
    void execNextInstruction();
    void interrupt();
    // İnceleme
    void stackFrames(std::function<void(QList<DebugFrame>)> done);
    void stackVariables(int frame, std::function<void(QList<DebugVar>)> done);
    void evaluate(const QString& expr, std::function<void(QString)> done);
    // Stage 26: izleme (varobj), yazmaç, bellek, disas, thread
    void watchCreate(const QString& expr, std::function<void(QString)> done); // name döner
    void watchUpdateAll(std::function<void(QVariantList)> done); // -var-update --all
    void watchEvaluate(const QString& name, std::function<void(QString)> done);
    void watchDelete(const QString& name);
    void registers(std::function<void(QList<QPair<QString, QString>>)> done);
    void memoryRead(const QString& addr, int count, std::function<void(QString)> done);
    void disassemble(const QString& file, int line1, int count,
                     std::function<void(QList<QMap<QString, QString>>)> done);
    struct ThreadInfo { QString id; QString target; QString name; };
    void threadList(std::function<void(QList<ThreadInfo>)> done);
    void threadSelect(const QString& id);
    // Stage 26: attach / core / kaynak eşleme
    void attach(int pid, std::function<void(bool)> done);
    void openCore(const QString& program, const QString& coreFile,
                  std::function<void(bool)> done);
    void substitutePath(const QString& from, const QString& to);
    void setVariable(const QString& expr, const QString& value,
                     std::function<void(bool)> done);
    // MI alıntılama (saf, test edilebilir)
    static QString miQuote(const QString& s);
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
