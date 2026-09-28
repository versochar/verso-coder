#include "GdbDriver.h"

GdbDriver::GdbDriver(QObject* parent) : QObject(parent) {
    connect(&m_proc, &QProcess::readyReadStandardOutput, this, &GdbDriver::onReadyRead);
    connect(&m_proc, &QProcess::readyReadStandardError, this, [this]() {
        m_proc.readAllStandardError(); // gdb açılış gevezeliği
    });
    connect(&m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &GdbDriver::onFinished);
}

GdbDriver::~GdbDriver() { quit(); }

bool GdbDriver::start(const QString& gdbPath) {
    quit();
    m_ready = false;
    m_debugging = false;
    m_buf.clear();
    m_handlers.clear();
    m_proc.start(gdbPath, {"--interpreter=mi2", "-q"});
    if (!m_proc.waitForStarted(5000)) {
        emit driverError("GDB başlatılamadı: " + gdbPath);
        return false;
    }
    return true;
}

void GdbDriver::quit() {
    if (m_proc.state() != QProcess::NotRunning) {
        if (m_ready) m_proc.write("-gdb-exit\n");
        m_proc.terminate();
        if (!m_proc.waitForFinished(1500)) m_proc.kill();
    }
    m_ready = false;
    m_debugging = false;
    m_handlers.clear();
}

bool GdbDriver::isRunning() const {
    return m_proc.state() != QProcess::NotRunning;
}

void GdbDriver::send(const QString& mi, std::function<void(QVariantMap)> done) {
    if (!isRunning()) return;
    int tok = m_nextToken++;
    if (done) m_handlers[tok] = done;
    m_proc.write(QString("%1%2\n").arg(tok).arg(mi).toUtf8());
}

int GdbDriver::command(const QString& mi, std::function<void(QVariantMap)> done) {
    if (!isRunning()) return -1;
    int tok = m_nextToken++;
    if (done) m_handlers[tok] = done;
    m_proc.write(QString("%1%2\n").arg(tok).arg(mi).toUtf8());
    return tok;
}

void GdbDriver::launch(const QString& program, const QStringList& args, const QString& cwd,
                       bool stopAtEntry, std::function<void(bool)> done) {
    auto fail = [this, done](const QString& e) {
        emit driverError(e);
        if (done) done(false);
    };
    if (!isRunning() && !start("gdb")) { fail("GDB yok."); return; }
    // gdb hazır olana kadar bekle (started sinyali) — komutları sırayla gönder
    auto go = [this, program, args, cwd, stopAtEntry, done]() {
        if (!cwd.isEmpty())
            send("-environment-cd " + cwd, nullptr);
        send("-file-exec-and-symbols " + program, [this, program, args, stopAtEntry, done](QVariantMap r) {
            if (r.value("_class").toString() == "error") {
                emit driverError("Program yüklenemedi: " + r.value("msg").toString());
                if (done) done(false);
                return;
            }
            if (!args.isEmpty())
                send("-exec-arguments " + args.join(' '), nullptr);
            m_debugging = true;
            if (stopAtEntry) {
                send("-break-insert -t main", [this, done](QVariantMap) {
                    execRun();
                    if (done) done(true);
                });
            } else {
                execRun();
                if (done) done(true);
            }
        });
    };
    if (m_ready) go();
    else connect(this, &GdbDriver::ready, this, go, Qt::SingleShotConnection);
}

void GdbDriver::breakInsert(const QString& file, int line1, const QString& cond,
                            std::function<void(QString)> done) {
    QString cmd = QString("-break-insert \"%1:%2\"").arg(file).arg(line1);
    if (!cond.trimmed().isEmpty()) cmd += " -c \"" + cond.trimmed() + "\"";
    send(cmd, [done](QVariantMap r) {
        QString num = r.value("bkpt").toMap().value("number").toString();
        if (done) done(num);
    });
}

void GdbDriver::breakDelete(const QString& number) {
    if (!number.isEmpty()) send("-break-delete " + number, nullptr);
}

void GdbDriver::execRun() { send("-exec-run", nullptr); }
void GdbDriver::execContinue() { send("-exec-continue", nullptr); }
void GdbDriver::execNext() { send("-exec-next", nullptr); }
void GdbDriver::execStep() { send("-exec-step", nullptr); }
void GdbDriver::execFinish() { send("-exec-finish", nullptr); }
QString GdbDriver::untilCommand(const QString& file, int line) {
    return QString("-exec-until %1")
        .arg(miQuote(file + ":" + QString::number(qMax(1, line))));
}
void GdbDriver::execUntil(const QString& file, int line) {
    send(untilCommand(file, line), nullptr);
}
void GdbDriver::interrupt() { send("-exec-interrupt", nullptr); }

void GdbDriver::stackFrames(std::function<void(QList<DebugFrame>)> done) {
    send("-stack-list-frames", [done](QVariantMap r) {
        QList<DebugFrame> out;
        const QVariantList stack = r.value("stack").toList();
        for (const QVariant& v : stack) {
            const QVariantMap fm = v.toMap().value("frame").toMap();
            if (fm.isEmpty() && v.toMap().contains("func")) {
                // düz frame objesi
            }
            const QVariantMap f = fm.isEmpty() ? v.toMap() : fm;
            if (!f.contains("func") && !f.contains("file")) continue;
            DebugFrame d;
            d.level = f.value("level").toInt();
            d.func = f.value("func").toString();
            d.file = MiParser::frameFile(f);
            d.line = MiParser::frameLine(f);
            d.addr = f.value("addr").toString();
            out << d;
        }
        if (done) done(out);
    });
}

void GdbDriver::stackVariables(int frame, std::function<void(QList<DebugVar>)> done) {
    // gdb17: --frame tek başına geçersiz, --thread gerekli
    send(QString("-stack-list-variables --thread 1 --frame %1 --all-values").arg(frame),
         [done](QVariantMap r) {
             QList<DebugVar> out;
             for (const QVariant& v : r.value("variables").toList()) {
                 const QVariantMap m = v.toMap();
                 DebugVar d;
                 d.name = m.value("name").toString();
                 d.value = m.value("value").toString();
                 d.type = m.value("type").toString();
                 if (d.name.isEmpty()) continue;
                 out << d;
             }
             if (done) done(out);
         });
}

void GdbDriver::evaluate(const QString& expr, std::function<void(QString)> done) {
    QString e = expr;
    e.replace('"', "\\\"");
    send("-data-evaluate-expression \"" + e + "\"", [done](QVariantMap r) {
             if (done) done(r.value("value").toString());
         });
}

// Stage 26: MI alıntılama — tersbölü + çift tırnak kaçar
QString GdbDriver::miQuote(const QString& s) {
    QString e = s;
    e.replace('\\', "\\\\");
    e.replace('"', "\\\"");
    return '"' + e + '"';
}

void GdbDriver::breakCondition(const QString& number, const QString& cond) {
    if (!number.isEmpty()) command("-break-condition " + number + " " + miQuote(cond), nullptr);
}

void GdbDriver::breakAfter(const QString& number, int count) {
    if (!number.isEmpty() && count > 0)
        command(QString("-break-after %1 %2").arg(number).arg(count), nullptr);
}

void GdbDriver::breakFunction(const QString& func, std::function<void(QString)> done) {
    command("-break-insert " + miQuote(func.trimmed()), [done](QVariantMap r) {
        if (done) done(r.value("bkpt").toMap().value("number").toString());
    });
}

void GdbDriver::breakWatch(const QString& expr, const QString& access,
                            std::function<void(QString)> done) {
    QString cmd = "-break-watch";
    if (access == "r") cmd += " -r";
    else if (access == "rw" || access == "a") cmd += " -a";
    cmd += " " + miQuote(expr.trimmed());
    command(cmd, [done](QVariantMap r) {
        if (done) done(r.value("wpt").toMap().value("number").toString());
    });
}

void GdbDriver::execStepInstruction() { command("-exec-step-instruction", nullptr); }
void GdbDriver::execNextInstruction() { command("-exec-next-instruction", nullptr); }

void GdbDriver::watchCreate(const QString& expr, std::function<void(QString)> done) {
    static int seq = 0;
    const QString name = QString("vverso%1").arg(++seq);
    command("-var-create " + name + " * " + miQuote(expr.trimmed()),
            [done, name](QVariantMap r) {
                if (done) done(r.value("_class").toString() == "error" ? QString() : name);
            });
}

void GdbDriver::watchUpdateAll(std::function<void(QVariantList)> done) {
    command("-var-update --all *", [done](QVariantMap r) {
        if (done) done(r.value("changelist").toList());
    });
}

void GdbDriver::watchEvaluate(const QString& name, std::function<void(QString)> done) {
    command("-var-evaluate-expression " + name, [done](QVariantMap r) {
        if (done) done(r.value("value").toString());
    });
}

void GdbDriver::watchDelete(const QString& name) {
    if (!name.isEmpty()) command("-var-delete " + name, nullptr);
}

void GdbDriver::registers(std::function<void(QList<QPair<QString, QString>>)> done) {
    command("-data-list-register-names", [this, done](QVariantMap rn) {
        const QVariantList names = rn.value("register-names").toList();
        command("-data-list-register-values x", [done, names](QVariantMap rv) {
            QList<QPair<QString, QString>> out;
            const QVariantList vals = rv.value("register-values").toList();
            for (const QVariant& v : vals) {
                const QVariantMap m = v.toMap();
                const int n = m.value("number").toInt();
                const QString nm = (n >= 0 && n < names.size()) ? names[n].toString()
                                                                : QString("r%1").arg(n);
                out << qMakePair(nm, m.value("value").toString());
            }
            if (done) done(out);
        });
    });
}

void GdbDriver::memoryRead(const QString& addr, int count,
                            std::function<void(QString)> done) {
    command(QString("-data-read-memory-bytes %1 %2").arg(addr.trimmed()).arg(qMax(1, count)),
            [done](QVariantMap r) {
                QString out;
                for (const QVariant& v : r.value("memory").toList()) {
                    const QVariantMap m = v.toMap();
                    out += m.value("address").toString() + ": ";
                    QStringList bytes;
                    for (const QVariant& b : m.value("data").toList())
                        bytes << b.toString();
                    out += bytes.join(" ") + "\n";
                    QString ascii;
                    for (const QVariant& b : m.value("data").toList()) {
                        bool ok = false;
                        const int byte = QString(b.toString()).toInt(&ok, 0);
                        ascii += (ok && byte >= 32 && byte < 127) ? QChar(byte) : '.';
                    }
                    out += "  [" + ascii + "]\n";
                }
                if (done) done(out.trimmed());
            });
}

void GdbDriver::disassemble(const QString& file, int line1, int count,
                             std::function<void(QList<QMap<QString, QString>>)> done) {
    command(QString("-data-disassemble -f %1 -l %2 -n %3 -- 0")
                .arg(miQuote(file))
                .arg(qMax(1, line1))
                .arg(qMax(1, count)),
            [done](QVariantMap r) {
                QList<QMap<QString, QString>> out;
                for (const QVariant& v : r.value("asm_insns").toList()) {
                    const QVariantMap m = v.toMap();
                    QMap<QString, QString> row;
                    row["address"] = m.value("address").toString();
                    row["func"] = m.value("func-name").toString();
                    row["offset"] = m.value("offset").toString();
                    row["inst"] = m.value("inst").toString();
                    out << row;
                }
                if (done) done(out);
            });
}

void GdbDriver::threadList(std::function<void(QList<ThreadInfo>)> done) {
    command("-thread-info", [done](QVariantMap r) {
        QList<ThreadInfo> out;
        for (const QVariant& v : r.value("threads").toList()) {
            const QVariantMap m = v.toMap();
            ThreadInfo t;
            t.id = m.value("id").toString();
            t.target = m.value("target-id").toString();
            t.name = m.value("name").toString();
            if (t.name.isEmpty()) t.name = m.value("details").toString();
            out << t;
        }
        if (done) done(out);
    });
}

void GdbDriver::threadSelect(const QString& id) {
    if (!id.isEmpty()) command("-thread-select " + id, nullptr);
}

void GdbDriver::attach(int pid, std::function<void(bool)> done) {
    if (pid <= 0) {
        if (done) done(false);
        return;
    }
    command(QString("-target-attach %1").arg(pid), [done](QVariantMap r) {
        if (done) done(r.value("_class").toString() != "error");
    });
}

void GdbDriver::openCore(const QString& program, const QString& coreFile,
                          std::function<void(bool)> done) {
    command("-file-exec-and-symbols " + miQuote(program), [this, coreFile, done](QVariantMap r) {
        if (r.value("_class").toString() == "error") {
            if (done) done(false);
            return;
        }
        command("-target-select core " + miQuote(coreFile), [done](QVariantMap r2) {
            if (done) done(r2.value("_class").toString() != "error");
        });
    });
}

void GdbDriver::substitutePath(const QString& from, const QString& to) {
    if (!from.trimmed().isEmpty())
        command("-gdb-set substitute-path " + miQuote(from.trimmed()) + " " + miQuote(to),
                nullptr);
}

void GdbDriver::setVariable(const QString& expr, const QString& value,
                             std::function<void(bool)> done) {
    command("-gdb-set variable " + expr.trimmed() + " = " + value.trimmed(),
            [done](QVariantMap r) {
                if (done) done(r.value("_class").toString() != "error");
            });
}

void GdbDriver::onReadyRead() {
    m_buf += m_proc.readAllStandardOutput();
    while (true) {
        int nl = m_buf.indexOf('\n');
        if (nl < 0) break;
        QByteArray raw = m_buf.left(nl);
        m_buf = m_buf.mid(nl + 1);
        const QString line = QString::fromUtf8(raw).trimmed();
        if (line.isEmpty()) continue;
        handleRecord(MiParser::parseLine(line));
    }
}

void GdbDriver::handleRecord(const MiRecord& r) {
    if (r.kind == "prompt") {
        if (!m_ready) {
            m_ready = true;
            emit ready();
        }
        return;
    }
    if (r.kind == "console") { emit consoleMsg(r.stream); return; }
    if (r.kind == "target") { emit output(r.stream); return; }
    if (r.kind == "log") { return; } // "&..." iç teşhis
    if (r.kind == "result") {
        bool ok = false;
        const int tok = r.token.toInt(&ok);
        if (ok) {
            auto it = m_handlers.find(tok);
            if (it != m_handlers.end()) {
                auto h = *it;
                m_handlers.erase(it);
                QVariantMap m = r.fields;
                m["_class"] = r.cls;
                h(m);
            }
        }
        if (r.cls == "error" && !ok)
            emit driverError("GDB hatası: " + r.fields.value("msg").toString());
        return;
    }
    if (r.kind == "exec" && r.cls == "stopped") {
        const QString reason = r.fields.value("reason").toString();
        DebugFrame f;
        const QVariantMap fm = r.fields.value("frame").toMap();
        if (!fm.isEmpty()) {
            f.func = fm.value("func").toString();
            f.file = MiParser::frameFile(fm);
            f.line = MiParser::frameLine(fm);
        }
        if (reason == "exited-normally" || reason == "exited") {
            m_debugging = false;
            emit exited(0);
        } else if (reason == "exited-signalled") {
            m_debugging = false;
            emit exited(-1);
        } else {
            emit stopped(reason, f);
        }
        return;
    }
    if (r.kind == "notify") {
        if (r.cls == "thread-group-exited") {
            const QString code = r.fields.value("exit-code").toString();
            m_debugging = false;
            emit exited(code.toInt());
        }
        return;
    }
}

void GdbDriver::onFinished(int) {
    const bool wasDbg = m_debugging;
    m_ready = false;
    m_debugging = false;
    if (wasDbg) emit exited(-1);
}

// ---------- Stage 16: uzak hedef (gdbserver) ----------

QString GdbDriver::gdbserverCmd(int port, const QString& program, const QString& args) {
    QString c = QString("gdbserver :%1 %2").arg(port).arg(program);
    if (!args.trimmed().isEmpty()) c += " " + args.trimmed();
    return c;
}

void GdbDriver::targetRemote(const QString& host, int port,
                             std::function<void(bool)> done) {
    auto fail = [this, done](const QString& e) {
        emit driverError(e);
        if (done) done(false);
    };
    if (!isRunning() && !start("gdb")) { fail("GDB yok."); return; }
    auto go = [this, host, port, done]() {
        send(QString("-target-select remote %1:%2").arg(host).arg(port),
             [this, done](QVariantMap r) {
                 if (r.value("_class").toString() == "error") {
                     emit driverError("Uzak hedefe bağlanılamadı: "
                                      + r.value("msg").toString());
                     if (done) done(false);
                     return;
                 }
                 m_debugging = true;
                 if (done) done(true);
             });
    };
    if (m_ready) go();
    else connect(this, &GdbDriver::ready, this, go, Qt::SingleShotConnection);
}

void GdbDriver::launchRemote(const QString& localSymbols, const QString& host, int port,
                             std::function<void(bool)> done) {
    auto fail = [this, done](const QString& e) {
        emit driverError(e);
        if (done) done(false);
    };
    if (!isRunning() && !start("gdb")) { fail("GDB yok."); return; }
    auto go = [this, localSymbols, host, port, done]() {
        auto cont = [this, host, port, done](QVariantMap r) {
            if (r.value("_class").toString() == "error") {
                emit driverError("Sembol yüklenemedi: " + r.value("msg").toString());
                if (done) done(false);
                return;
            }
            targetRemote(host, port, done);
        };
        if (!localSymbols.isEmpty())
            send("-file-exec-and-symbols " + localSymbols, cont);
        else
            targetRemote(host, port, done);
    };
    if (m_ready) go();
    else connect(this, &GdbDriver::ready, this, go, Qt::SingleShotConnection);
}
