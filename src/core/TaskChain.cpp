#include "TaskChain.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <functional>

QList<QString> TaskChain::order(const QMap<QString, QStringList>& deps,
                                QString& error) {
    error.clear();
    QList<QString> out;
    QSet<QString> done, visiting;
    std::function<bool(const QString&)> visit = [&](const QString& n) -> bool {
        if (done.contains(n)) return true;
        if (visiting.contains(n)) {
            error = "Döngüsel bağımlılık: " + n;
            return false;
        }
        visiting.insert(n);
        for (const QString& d : deps.value(n))
            if (!visit(d)) return false;
        visiting.remove(n);
        done.insert(n);
        out << n;
        return true;
    };
    // Belirleyici sıra: alfabetik kökler
    QStringList keys = deps.keys();
    keys.sort();
    for (const QString& k : keys)
        if (!visit(k)) return {};
    // deps'te anahtar olmayan bağımlılıklar da sıraya girer (visit ekler)
    return out;
}

QList<TaskChain::TaskInput> TaskChain::parseTaskInputs(const QJsonArray& arr) {
    QList<TaskInput> out;
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        TaskInput t;
        t.id = o.value("id").toString();
        if (t.id.isEmpty()) continue;
        t.type = o.value("type").toString("promptString");
        t.def = o.value("default").toString();
        t.description = o.value("description").toString();
        for (const QJsonValue& op : o.value("options").toArray()) {
            const QString s = op.isObject() ? op.toObject().value("label").toString()
                                            : op.toString();
            if (!s.isEmpty()) t.options << s;
        }
        out << t;
    }
    return out;
}

QMap<QString, QString> TaskChain::parseInputs(const QJsonArray& arr) {
    QMap<QString, QString> out;
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        const QString id = o.value("id").toString();
        if (!id.isEmpty()) out[id] = o.value("default").toString();
    }
    return out;
}

QString TaskChain::expand(const QString& command, const QString& root,
                          const QString& file, const QMap<QString, QString>& inputs) {
    QString out = command;
    out.replace("${workspaceFolder}", root);
    out.replace("${file}", file);
    out.replace("${fileBasename}", QFileInfo(file).fileName());
    out.replace("${fileDirname}", QFileInfo(file).absolutePath());
    for (auto it = inputs.begin(); it != inputs.end(); ++it)
        out.replace("${input:" + it.key() + "}", it.value());
    return out;
}

QList<TaskChain::ChainIssue> TaskChain::matchProblems(const QString& output,
                                                      const QString& matcher,
                                                      const QString& root) {
    QList<ChainIssue> out;
    if (matcher.trimmed().isEmpty() || matcher == "$null") return out;
    // $gcc / $tsc / $python: dosya:satır(:sütun) yakalama
    static QRegularExpression re(
        R"(^(.+\.(?:cpp|c|cc|h|hpp|py|ts|js|rs|go))[:\(](\d+)(?::|\s*,\s*|\()(\d+)?[^:]*:\s*(?:error|warning)?\s*:?\s*(.*)$)");
    for (const QString& raw : output.split('\n')) {
        const QString line = raw.trimmed();
        if (line.isEmpty()) continue;
        auto m = re.match(line);
        if (!m.hasMatch()) continue;
        ChainIssue is;
        is.file = m.captured(1);
        if (QFileInfo(is.file).isRelative() && !root.isEmpty())
            is.file = QDir(root).absoluteFilePath(is.file);
        is.line1 = qMax(1, m.captured(2).toInt());
        is.col1 = m.captured(3).isEmpty() ? 1 : qMax(1, m.captured(3).toInt());
        is.message = m.captured(5).trimmed().left(300);
        out << is;
    }
    return out;
}

QMap<QString, QStringList> TaskChain::depsFrom(const QJsonArray& tasks) {
    QMap<QString, QStringList> out;
    for (const QJsonValue& v : tasks) {
        const QJsonObject o = v.toObject();
        const QString label = o.value("label").toString();
        if (label.isEmpty()) continue;
        QStringList deps;
        const QJsonValue dv = o.value("dependsOn");
        if (dv.isString()) deps << dv.toString();
        else if (dv.isArray())
            for (const QJsonValue& d : dv.toArray()) deps << d.toString();
        out[label] = deps;
    }
    return out;
}
