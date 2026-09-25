#include "AgentTools.h"
#include "PatchQueue.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QTextStream>

AgentTools::AgentTools(const QString& root) : m_root(root) {}

bool AgentTools::isInsideRoot(const QString& absPath) const {
    if (m_root.isEmpty()) return false;
    QString root = QDir(m_root).absolutePath();
    QString p = QDir::cleanPath(absPath);
    if (p == root) return true;
    return p.startsWith(root + "/");
}

QString AgentTools::absoluteInRoot(const QString& rel) const {
    if (QDir::isAbsolutePath(rel)) return QDir::cleanPath(rel);
    return QDir::cleanPath(QDir(m_root).absoluteFilePath(rel));
}

bool AgentTools::approve(const ToolCall& c) {
    if (!m_approver) return true; // onaylayıcı yoksa (test) izin ver
    return m_approver(c);
}

QJsonArray AgentTools::toolSchemas() const {
    auto schema = [](const QString& name, const QString& desc, const QJsonObject& props,
                     const QJsonArray& required) {
        QJsonObject o;
        o["name"] = name;
        o["description"] = desc;
        QJsonObject params;
        params["type"] = "object";
        params["properties"] = props;
        params["required"] = required;
        o["parameters"] = params;
        return o;
    };
    QJsonArray arr;
    arr.append(schema("read_file", "Proje içindeki bir dosyayı satır numaralarıyla oku.",
                      QJsonObject{{"path", QJsonObject{{"type", "string"}}},
                                  {"start", QJsonObject{{"type", "integer"}}},
                                  {"count", QJsonObject{{"type", "integer"}}}},
                      {"path"}));
    arr.append(schema("write_file", "Bir dosyayı tümüyle yeni içerikle değiştir (onay gerekir).",
                      QJsonObject{{"path", QJsonObject{{"type", "string"}}},
                                  {"content", QJsonObject{{"type", "string"}}}},
                      {"path", "content"}));
    arr.append(schema("list_dir", "Bir dizindeki dosya/klasörleri listele.",
                      QJsonObject{{"path", QJsonObject{{"type", "string"}}}}, {}));
    arr.append(schema("search", "Projede düz metin/regex ara.",
                      QJsonObject{{"pattern", QJsonObject{{"type", "string"}}},
                                  {"glob", QJsonObject{{"type", "string"}}}},
                      {"pattern"}));
    arr.append(schema("run_command", "Kök dizinde kabuk komutu çalıştır (onay gerekir).",
                      QJsonObject{{"command", QJsonObject{{"type", "string"}}}}, {"command"}));
    arr.append(schema("get_problems", "Editörün bildirdiği derleme/lint sorunlarını getir.",
                      QJsonObject{}, {}));
    return arr;
}

QString AgentTools::systemPromptAddendum() const {
    QString s;
    s += "\n\n[ARAÇ KULLANIMI]\n";
    s += "Görevi tamamlamak için aşağıdaki araçları çağırabilirsin. Bir araç çağırmak için "
         "yanıtına aynen şu bloğu yaz:\n";
    s += "<tool_call>\n{\"name\":\"read_file\",\"arguments\":{\"path\":\"src/main.cpp\"}}\n</tool_call>\n";
    s += "Birden çok çağrı için ayrı bloklar yazabilirsin. Gözlemler sana geri verilir. "
         "Araç gerekmiyorsa nihai yanıtı normal metin olarak ver.\n";
    s += "Araçlar:\n";
    s += "- read_file {path, start?, count?}\n";
    s += "- write_file {path, content}  (onay ister; görev tamamlanınca tam dosya içeriği ver)\n";
    s += "- list_dir {path?}\n";
    s += "- search {pattern, glob?}\n";
    s += "- run_command {command}  (onay ister" + QString(m_allowCommand ? "" : ", şu an kapalı") + ")\n";
    s += "- get_problems {}\n";
    if (m_writeMode == Queue)
        s += "NOT: write_file çağrıları önce kullanıcı onay kuyruğuna alınır.\n";
    return s;
}

QList<ToolCall> AgentTools::parseCalls(const QString& text) {
    QList<ToolCall> out;
    auto addJson = [&out](const QString& js) {
        QJsonDocument d = QJsonDocument::fromJson(js.toUtf8());
        auto addObj = [&out](const QJsonObject& o) {
            ToolCall c;
            c.name = o.value("name").toString(o.value("tool").toString());
            c.args = o.value("arguments").toObject();
            if (c.args.isEmpty()) c.args = o.value("args").toObject();
            if (c.args.isEmpty() && o.contains("parameters")) c.args = o.value("parameters").toObject();
            if (c.args.isEmpty()) { // düz argümanlar
                for (const QString& k : o.keys())
                    if (k != "name" && k != "tool") c.args[k] = o.value(k);
            }
            if (!c.name.isEmpty()) out << c;
        };
        if (d.isArray())
            for (const QJsonValue& v : d.array()) addObj(v.toObject());
        else if (d.isObject())
            addObj(d.object());
    };

    QRegularExpression rxCall("<tool_call>\\s*(.*?)\\s*</tool_call>",
        QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption);
    for (auto it = rxCall.globalMatch(text); it.hasNext();) addJson(it.next().captured(1).trimmed());

    QRegularExpression rxTag("<tool\\s+name=[\"']([^\"']+)[\"']\\s*(?:/>|>(.*?)</tool>)",
        QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption);
    for (auto it = rxTag.globalMatch(text); it.hasNext();) {
        auto m = it.next();
        ToolCall c;
        c.name = m.captured(1).trimmed();
        QString body = m.captured(2).trimmed();
        if (!body.isEmpty()) {
            QJsonDocument d = QJsonDocument::fromJson(body.toUtf8());
            if (d.isObject()) c.args = d.object();
        }
        if (!c.name.isEmpty()) out << c;
    }

    QRegularExpression rxFence("```(?:tool|json:tool)\\s*(.*?)```",
        QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption);
    for (auto it = rxFence.globalMatch(text); it.hasNext();) addJson(it.next().captured(1).trimmed());
    return out;
}

ToolResult AgentTools::readFile(const QString& path, int startLine, int count) {
    ToolResult r;
    QString abs = absoluteInRoot(path);
    if (!isInsideRoot(abs)) { r.output = "HATA: kök dizin dışına erişim reddedildi: " + path; return r; }
    QFile f(abs);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) { r.output = "HATA: açılamadı: " + path; return r; }
    const QString all = QString::fromUtf8(f.read(2 * 1024 * 1024));
    const QStringList lines = all.split('\n');
    int from = startLine > 0 ? startLine - 1 : 0;
    int to = (count > 0) ? qMin(lines.size(), from + count) : lines.size();
    QString out;
    QTextStream ts(&out);
    for (int i = from; i < to; ++i)
        ts << QString("%1| %2\n").arg(i + 1, 5).arg(lines[i]);
    r.ok = true;
    r.output = out.left(24000);
    return r;
}

ToolResult AgentTools::writeFile(const QString& path, const QString& content) {
    ToolResult r;
    QString abs = absoluteInRoot(path);
    if (!isInsideRoot(abs)) { r.output = "HATA: kök dizin dışına yazma reddedildi: " + path; return r; }
    ToolCall c; c.name = "write_file"; c.args = QJsonObject{{"path", path}, {"content", content}};
    if (!approve(c)) { r.denied = true; r.output = "Kullanıcı yazmayı reddetti: " + path; return r; }

    if (m_writeMode == Queue && m_queue) {
        QueuedEdit e;
        e.path = abs;
        QFile old(abs);
        if (old.open(QIODevice::ReadOnly | QIODevice::Text)) e.oldText = QString::fromUtf8(old.readAll());
        e.newText = content;
        e.wholeFile = true;
        m_queue->add(e);
        r.ok = true;
        r.output = "Onay kuyruğuna alındı (yazılmadı): " + path;
        return r;
    }
    QDir().mkpath(QFileInfo(abs).absolutePath());
    QFile f(abs);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) { r.output = "HATA: yazılamadı: " + path; return r; }
    f.write(content.toUtf8());
    r.ok = true;
    r.output = QString("Yazıldı: %1 (%2 bayt)").arg(path).arg(content.toUtf8().size());
    return r;
}

ToolResult AgentTools::listDir(const QString& path) {
    ToolResult r;
    QString abs = path.isEmpty() ? QDir(m_root).absolutePath() : absoluteInRoot(path);
    if (!isInsideRoot(abs)) { r.output = "HATA: kök dizin dışı: " + path; return r; }
    QDir d(abs);
    if (!d.exists()) { r.output = "HATA: dizin yok: " + path; return r; }
    QStringList names;
    for (const QFileInfo& fi : d.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden,
                                               QDir::DirsFirst | QDir::Name))
        names << (fi.isDir() ? fi.fileName() + "/" : fi.fileName());
    r.ok = true;
    r.output = names.join('\n').left(8000);
    return r;
}

ToolResult AgentTools::search(const QString& pattern, const QString& glob, int maxHits) {
    ToolResult r;
    QRegularExpression rx(pattern);
    if (!rx.isValid()) { r.output = "HATA: geçersiz desen: " + pattern; return r; }
    QRegularExpression globRx;
    if (!glob.isEmpty())
        globRx = QRegularExpression(QRegularExpression::wildcardToRegularExpression(glob));
    QStringList hits;
    QDirIterator it(m_root, QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden, QDirIterator::Subdirectories);
    int scanned = 0;
    while (it.hasNext() && hits.size() < maxHits && scanned < 6000) {
        QString p = it.next();
        if (p.contains("/.git/") || p.contains("/build/") || p.contains("/node_modules/")) continue;
        if (!glob.isEmpty() && !globRx.match(QFileInfo(p).fileName()).hasMatch()) continue;
        QFile f(p);
        if (!f.open(QIODevice::ReadOnly)) continue;
        QByteArray raw = f.read(512 * 1024);
        if (raw.contains('\0')) continue;
        ++scanned;
        const QStringList lines = QString::fromUtf8(raw).split('\n');
        for (int i = 0; i < lines.size() && hits.size() < maxHits; ++i)
            if (rx.match(lines[i]).hasMatch())
                hits << QString("%1:%2: %3").arg(QDir(m_root).relativeFilePath(p))
                            .arg(i + 1).arg(lines[i].trimmed().left(160));
    }
    r.ok = true;
    r.output = hits.isEmpty() ? "(eşleşme yok)" : hits.join('\n');
    return r;
}

ToolResult AgentTools::runCommand(const QString& command, int timeoutMs) {
    ToolResult r;
    if (!m_allowCommand) { r.denied = true; r.output = "run_command devre dışı (ayarlardan/onaydan aç)."; return r; }
    ToolCall c; c.name = "run_command"; c.args = QJsonObject{{"command", command}};
    if (!approve(c)) { r.denied = true; r.output = "Kullanıcı komutu reddetti."; return r; }
    QProcess p;
    p.setWorkingDirectory(m_root);
    p.start("bash", {"-lc", command});
    if (!p.waitForFinished(timeoutMs)) {
        p.kill();
        r.output = "HATA: zaman aşımı (" + QString::number(timeoutMs) + " ms)";
        return r;
    }
    QString out = QString::fromUtf8(p.readAllStandardOutput());
    QString err = QString::fromUtf8(p.readAllStandardError());
    r.ok = p.exitCode() == 0;
    r.output = QString("$ %1\n[exit %2]\n%3%4")
                   .arg(command).arg(p.exitCode())
                   .arg(out.left(12000), err.isEmpty() ? "" : "\n[stderr]\n" + err.left(4000));
    return r;
}

ToolResult AgentTools::getProblems() {
    ToolResult r;
    r.ok = true;
    r.output = m_problems ? m_problems() : "(sorun bilgisi yok)";
    if (r.output.isEmpty()) r.output = "(sorun yok)";
    return r;
}

ToolResult AgentTools::execute(const ToolCall& call) {
    const QString n = call.name;
    ToolResult r;
    if (n == "read_file")
        return readFile(call.args.value("path").toString(),
                        call.args.value("start").toInt(),
                        call.args.value("count").toInt(-1));
    if (n == "write_file")
        return writeFile(call.args.value("path").toString(), call.args.value("content").toString());
    if (n == "list_dir")
        return listDir(call.args.value("path").toString());
    if (n == "search")
        return search(call.args.value("pattern").toString(), call.args.value("glob").toString());
    if (n == "run_command")
        return runCommand(call.args.value("command").toString());
    if (n == "get_problems")
        return getProblems();
    r.output = "Bilinmeyen araç: " + n;
    return r;
}
