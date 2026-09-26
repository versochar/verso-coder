#include "AgentRunStore.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

QString AgentRun::statusLabel() const {
    if (ok) return "başarılı";
    if (steps > 0) return "kısmi";
    return "başarısız";
}

QString AgentRun::oneLine() const {
    return QString("%1 · %2 adım · %3 araç · %4 dosya")
        .arg(QDateTime::fromMSecsSinceEpoch(whenMs).toString("dd.MM HH:mm"))
        .arg(steps)
        .arg(toolCalls)
        .arg(changedFiles.size());
}

AgentRunStore::AgentRunStore(const QString& dir) : m_dir(dir) {
    QDir().mkpath(m_dir);
}

QString AgentRunStore::add(const AgentRun& run) {
    AgentRun r = run;
    if (r.id.isEmpty())
        r.id = QString("r%1").arg(QDateTime::currentMSecsSinceEpoch());
    if (r.whenMs == 0) r.whenMs = QDateTime::currentMSecsSinceEpoch();
    r.steps = r.stepLog.isEmpty() ? r.steps : r.stepLog.size();
    int calls = r.toolCalls;
    if (calls == 0)
        for (const AgentRunStep& s : r.stepLog) calls += s.toolNames.size();
    r.toolCalls = calls;

    QJsonArray steps;
    for (const AgentRunStep& s : r.stepLog) {
        steps.append(QJsonObject{{"assistant", s.assistant},
                                 {"tools", QJsonArray::fromStringList(s.toolNames)},
                                 {"obs", QJsonArray::fromStringList(s.observations)}});
    }
    QJsonArray files;
    for (const RunFile& f : r.changedFiles)
        files.append(QJsonObject{{"path", f.path}, {"before", f.before}, {"created", f.created}});

    QJsonObject o{{"id", r.id},
                  {"task", r.task},
                  {"when", double(r.whenMs)},
                  {"elapsed", double(r.elapsedMs)},
                  {"toolCalls", r.toolCalls},
                  {"tokens", r.tokens},
                  {"ok", r.ok},
                  {"final", r.finalText},
                  {"steps", steps},
                  {"files", files}};
    QFile f(m_dir + "/" + r.id + ".json");
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return {};
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    return r.id;
}

QList<AgentRun> AgentRunStore::list(int limit) const {
    QList<AgentRun> out;
    QDir d(m_dir);
    const QStringList files = d.entryList({"*.json"}, QDir::Files, QDir::Time);
    for (int i = 0; i < files.size() && (limit <= 0 || out.size() < limit); ++i) {
        AgentRun r = load(QFileInfo(files[i]).completeBaseName());
        if (!r.id.isEmpty()) out << r;
    }
    return out;
}

AgentRun AgentRunStore::load(const QString& id) const {
    AgentRun r;
    QFile f(m_dir + "/" + id + ".json");
    if (!f.open(QIODevice::ReadOnly)) return r;
    const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
    r.id = o.value("id").toString();
    if (r.id.isEmpty()) return AgentRun{};
    r.task = o.value("task").toString();
    r.whenMs = qint64(o.value("when").toDouble());
    r.elapsedMs = qint64(o.value("elapsed").toDouble());
    r.toolCalls = o.value("toolCalls").toInt();
    r.tokens = o.value("tokens").toInt();
    r.ok = o.value("ok").toBool();
    r.finalText = o.value("final").toString();
    for (const QJsonValue& v : o.value("steps").toArray()) {
        const QJsonObject so = v.toObject();
        AgentRunStep s;
        s.assistant = so.value("assistant").toString();
        for (const QJsonValue& t : so.value("tools").toArray()) s.toolNames << t.toString();
        for (const QJsonValue& ob : so.value("obs").toArray()) s.observations << ob.toString();
        r.stepLog << s;
    }
    for (const QJsonValue& v : o.value("files").toArray()) {
        const QJsonObject fo = v.toObject();
        RunFile f;
        f.path = fo.value("path").toString();
        f.before = fo.value("before").toString();
        f.created = fo.value("created").toBool();
        r.changedFiles.append(f);
    }
    r.steps = r.stepLog.size();
    return r;
}

bool AgentRunStore::remove(const QString& id) {
    const QString path = m_dir + "/" + id + ".json";
    if (!QFile::exists(path)) return false;
    return QFile::remove(path);
}

int AgentRunStore::count() const {
    return QDir(m_dir).entryList({"*.json"}, QDir::Files).size();
}

QString AgentRunStore::revert(const AgentRun& run, int* reverted, QStringList* skipped) {
    int done = 0;
    QStringList skip;
    QStringList log;
    for (const RunFile& rf : run.changedFiles) {
        const QString path = rf.path;
        QFile f(path);
        const bool exists = f.exists();
        if (rf.created) {
            // Ajan bu dosyayı oluşturmuştu: geri almak = silmek
            if (!exists) {
                ++done;
                log << "zaten silinmiş: " + path;
            } else if (QFile::remove(path)) {
                ++done;
                log << "silindi: " + path;
            } else {
                skip << path + " (silinemedi)";
            }
            continue;
        }
        if (!exists) {
            ++done;
            log << "dosya yok: " + path;
            continue;
        }
        if (!f.open(QIODevice::ReadOnly)) {
            skip << path + " (okunamadı)";
            continue;
        }
        const QString now = QString::fromUtf8(f.readAll());
        f.close();
        if (now != rf.before) {
            skip << path + " (sonradan değiştirilmiş)";
            continue;
        }
        QFile w(path);
        if (!w.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            skip << path + " (yazılamadı)";
            continue;
        }
        w.write(rf.before.toUtf8());
        ++done;
        log << "geri alındı: " + path;
    }
    if (reverted) *reverted = done;
    if (skipped) *skipped = skip;
    if (done == 0 && skip.isEmpty()) return "Geri alınacak dosya değişikliği yok.";
    QString out = QString("%1 dosya geri alındı.").arg(done);
    if (!skip.isEmpty())
        out += "\nAtlanan (" + QString::number(skip.size()) + "): " + skip.join(", ");
    if (!log.isEmpty()) out += "\n" + log.join("\n");
    return out;
}

QString AgentRunStore::diffSummary(const AgentRun& run, int maxChars) {
    QString out;
    for (int i = 0; i < run.stepLog.size(); ++i) {
        out += QString("\n— Adım %1 —\n").arg(i + 1);
        if (!run.stepLog[i].toolNames.isEmpty())
            out += "araçlar: " + run.stepLog[i].toolNames.join(", ") + "\n";
        for (const QString& o : run.stepLog[i].observations) {
            const QString line = "  " + o.left(200) + "\n";
            if (out.size() + line.size() > maxChars) return out + "\n… (kısaltıldı)";
            out += line;
        }
    }
    if (!run.finalText.isEmpty())
        out += "\nSonuç: " + run.finalText.left(1000) + "\n";
    if (out.size() > maxChars) out = out.left(maxChars) + "\n… (kısaltıldı)";
    return out;
}
