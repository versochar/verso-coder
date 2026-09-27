#include "AgentTools.h"
#include "PathGuard.h"
#include <QElapsedTimer>
#include <QSettings>
#include <QRegularExpression>
#include <QStandardPaths>
#include "PatchQueue.h"
#include "ProjectHealth.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QTextStream>
#include <algorithm>

AgentTools::AgentTools(const QString& root) : m_root(root), m_guard(root) {}

bool AgentTools::safePath(const QString& input, QString& outAbs, QString& why) const {
    why.clear();
    if (m_guard.resolve(input, outAbs, &why)) return true;
    if (why.isEmpty()) why = QStringLiteral("kök dışı yol");
    return false;
}

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
    // --- Stage 34 araçları ---
    arr.append(schema("read_range", "Dosyanın belirli satır aralığını oku (start/end dahil).",
                      QJsonObject{{"path", QJsonObject{{"type", "string"}}},
                                  {"start", QJsonObject{{"type", "integer"}}},
                                  {"end", QJsonObject{{"type", "integer"}}}},
                      {"path"}));
    arr.append(schema("grep_lines", "Desen eşleşen satırları bağlam satırlarıyla getir.",
                      QJsonObject{{"pattern", QJsonObject{{"type", "string"}}},
                                  {"glob", QJsonObject{{"type", "string"}}},
                                  {"context", QJsonObject{{"type", "integer"}}}},
                      {"pattern"}));
    arr.append(schema("find_symbol", "Sınıf/fonksiyon/sembol tanım ve kullanımlarını bul.",
                      QJsonObject{{"name", QJsonObject{{"type", "string"}}}},
                      {"name"}));
    arr.append(schema("git_status", "Git çalışma ağacı durumunu oku.",
                      QJsonObject{}, {}));
    arr.append(schema("git_diff", "Çalışma ağacı ya da staged değişiklikleri oku.",
                      QJsonObject{{"staged", QJsonObject{{"type", "boolean"}}}}, {}));
    arr.append(schema("run_tests",
                      QString("Proje test komutunu çalıştır") +
                          (m_policy.toolAllowed("run_tests") ? " (onay ister)" : " (şu an kapalı)") + ".",
                      QJsonObject{{"command", QJsonObject{{"type", "string"}}}}, {}));
    arr.append(schema("health_scan", "Proje sağlık taraması çalıştır (skor + öneriler).",
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
    // --- Stage 34 araçları ---
    s += "- read_range {path, start, end}\n";
    s += "- grep_lines {pattern, glob?, context?}\n";
    s += "- find_symbol {name}\n";
    s += "- git_status {}   (yalnız okuma)\n";
    s += "- git_diff {staged?}   (yalnız okuma)\n";
    s += "- run_tests {command?}" +
         QString(m_policy.toolAllowed("run_tests") ? "  (onay ister)\n" : "  (şu an kapalı)\n");
    s += "- health_scan {}   (proje sağlık skoru)\n";
    if (m_policy.autonomous)
        s += "OTONOM MOD: yalnızca okuma araçlarını kullan; hiçbir dosyayı değiştirme, "
             "komut çalıştırma; sonunda kısa bir rapor ver.\n";
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
    QString abs, why;
    if (!safePath(path, abs, why)) {
        // Stage 38: neden açıklanır (sembolik bağlantı kaçışı dâhil)
        r.denied = true;
        r.output = "HATA: erişim reddedildi (" + why + "): " + path;
        return r;
    }
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
    QString abs, why;
    if (!safePath(path, abs, why)) {
        r.denied = true;
        r.output = "HATA: yazma reddedildi (" + why + "): " + path;
        return r;
    }
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
        // Stage 39: eşik aşılınca uyar — kullanıcı tek "evet" ile geçmemeli
        if (queueNeedsReview())
            r.output += QString("\nUYARI: kuyrukta %1 dosya var (eşik %2) — "
                                "gözden geçirmede tek tek işaretleyin.")
                            .arg(m_queue->count())
                            .arg(m_queueThreshold);
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
    QString abs, why;
    if (path.isEmpty()) {
        abs = m_guard.canonicalRoot();
    } else if (!safePath(path, abs, why)) {
        r.denied = true;
        r.output = "HATA: listeleme reddedildi (" + why + "): " + path;
        return r;
    }
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

QProcessEnvironment AgentTools::sanitizedEnvironment() {
    // Stage 38: komuta API anahtarlarını/sırları TAŞIMA.
    // (Ajan komutu çalıştırırken yanlışlıkla `env > dosya` yazabilirdi.)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QStringList prefixes = {"VERSO_AI", "OPENAI", "ANTHROPIC", "GEMINI", "GOOGLE_API",
                                 "NVIDIA", "UNOROUTER", "GROQ", "DEEPSEEK", "MISTRAL",
                                 "XAI_", "TOGETHER", "AZURE", "HF_", "HUGGING"};
    const QStringList exact = {"AWS_SECRET_ACCESS_KEY", "AWS_ACCESS_KEY_ID", "GITHUB_TOKEN",
                               "GH_TOKEN", "GITLAB_TOKEN", "SSH_AUTH_SOCK", "NPM_TOKEN"};
    for (const QString& k : env.keys()) {
        const QString up = k.toUpper();
        for (const QString& pfx : prefixes)
            if (up.startsWith(pfx) && (up == pfx || !up[pfx.size()].isLetterOrNumber())) {
                env.remove(k);
                break;
            }
        if (exact.contains(up)) env.remove(k);
    }
    env.insert("PATH", env.value("PATH", "/usr/bin:/bin"));
    return env;
}

QString AgentTools::readOnlyWrapper() {
    if (!QStandardPaths::findExecutable("bwrap").isEmpty()) return QStringLiteral("bwrap");
    if (!QStandardPaths::findExecutable("unshare").isEmpty()) return QStringLiteral("unshare");
    return {};
}

bool AgentTools::readOnlyAvailable() { return !readOnlyWrapper().isEmpty(); }

QString AgentTools::pathWarning(const QString& command) {
    // Mutlak yol içeren komutlar kök dışına çıkabilir; onayda görünmeli.
    static const QRegularExpression reAbs(QStringLiteral("(^|[\\s'\"=(|;&])/(?!/)[^\\s'\"();|&]+"));
    QStringList hits;
    auto it = reAbs.globalMatch(command);
    while (it.hasNext()) {
        // captured(0) = ayırıcı + yol; baştaki ayırıcıyı at
        QString m = it.next().captured(0);
        if (m.startsWith(QLatin1Char('/'))) m.remove(0, 1);
        if (m.size() > 2 && !hits.contains(m)) hits << m;
        if (hits.size() >= 4) break;
    }
    QStringList warnings;
    if (!hits.isEmpty())
        warnings << QStringLiteral("Mutlak yol içeriyor: %1").arg(hits.join(", "));
    // Stage 39: komut zincirleme/girdi ikamesi/yönlendirme kalıpları. "&&"/"|"
    // tek başına zararsızdır (derleme komutlarında normaldir); yalnız gerçekten
    // tehlikeli olanlar bildirilir: komut ikamesi, kabuğa borulama, köke yazma.
    // Backtick her zaman bildirilir (modern kabukta gereksiz, gözden kaçar);
    // $(...) için zararsız izin listesi vardır (derleme komutlarında standart).
    static const QRegularExpression reBacktick(QStringLiteral("`[^`]*`"));
    static const QRegularExpression reDollar(QStringLiteral("\\$\\(([^)]*)\\)"));
    // Stage 40: izin listesi ayarlardan gelir. static ÖNBELLEK YOK: ilk
    // çağrıda donsaydı kullanıcının ayar değişikliği hiç okunmazdı.
    const QStringList benignSubst = benignSubstitutions();
    bool riskySubst = command.contains(reBacktick);
    if (!riskySubst) {
        auto subIt = reDollar.globalMatch(command);
        while (subIt.hasNext()) {
            const QString inner = subIt.next().captured(1).trimmed();
            const QString first =
                inner.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).value(0);
            if (!benignSubst.contains(first, Qt::CaseInsensitive)
                && !benignSubst.contains(inner, Qt::CaseInsensitive)) {
                riskySubst = true;
                break;
            }
        }
    }
    if (riskySubst)
        warnings << QStringLiteral("Komut ikamesi içeriyor ($(...)/backtick): çalışmadan önce açılmış halini denetleyin");
    static const QRegularExpression rePipeSh(
        QStringLiteral("\\|\\s*(?:sh|bash|zsh|dash|python3?|perl|ruby)\\b"));
    if (command.contains(rePipeSh))
        warnings << QStringLiteral("Çıktı doğrudan kabuğa/yorumlayıcıya borulanıyor: indir-çalıştır kalıbı");
    static const QRegularExpression reRootWrite(
        QStringLiteral("(^|[\\s;&|])(?:sudo\\s+)?(tee\\s+)?/(?:etc|usr|bin|sbin|boot|proc|sys|dev)/[^\\s]*"));
    if (command.contains(reRootWrite))
        warnings << QStringLiteral("Sistem dizinine yazma içeriyor: kök dışına çıkıyor");
    if (warnings.isEmpty()) return {};
    return warnings.join("\n");
}

QStringList AgentTools::blockedPatterns() {
    // Stage 34 güvenlik: yıkıcı/uzak etkili komut kalıpları
    return {
        "rm -rf /", "rm -fr /", "mkfs", "dd if=", ":(){", "shutdown", "reboot", "halt",
        "chown -R /", "chmod -R 777 /", "> /dev/sd", "git push", "git reset --hard",
        "curl ", "wget ", "sudo ", "su -", "nc -l", "ssh ", "scp ", "systemctl ",
        "kill -9 1", "insmod", "modprobe", "> /proc/", "truncate -s 0 /"};
}

bool AgentTools::isCommandBlocked(const QString& command, QString* why) {
    const QString c = command.toLower();
    for (const QString& pat : blockedPatterns())
        if (c.contains(pat)) {
            if (why) *why = QString("tehlikeli kalıp: \"%1\"").arg(pat);
            return true;
        }
    return false;
}

QString AgentTools::guessTestCommand(const QString& root) {
    const QString r = QDir(root).absolutePath();
    if (QFile::exists(r + "/package.json")) return "npm test";
    if (QFile::exists(r + "/pytest.ini") || QFile::exists(r + "/tests/test_" + QFileInfo(r).fileName() + ".py"))
        return "python3 -m pytest -q";
    if (QFile::exists(r + "/Makefile") || QFile::exists(r + "/CMakeLists.txt")) return "ctest --output-on-failure";
    if (QFile::exists(r + "/Cargo.toml")) return "cargo test";
    if (QFile::exists(r + "/go.mod")) return "go test ./...";
    if (QFile::exists(r + "/pom.xml")) return "mvn -q test";
    if (QFile::exists(r + "/build.gradle")) return "./gradlew test";
    return {};
}

ToolResult AgentTools::runShell(const QString& command, int timeoutMs, const QString& label,
                                bool needsApproval) {
    ToolResult r;
    QString why;
    if (isCommandBlocked(command, &why)) {
        r.denied = true;
        r.output = "Güvenlik: komut engellendi (" + why + ")";
        if (m_auditOn) m_audit.record(label, command, false, true, -1, 0, why);
        return r;
    }
    if (needsApproval && !sessionAllows(command)) {
        ToolCall c;
        c.name = label;
        // Stage 38: onay diyaloğunda mutlak yol uyarısı görünür
        const QString warn = pathWarning(command);
        c.args = QJsonObject{{"command", command}, {"pathWarning", warn}};
        if (!approve(c)) {
            r.denied = true;
            r.output = "Kullanıcı komutu reddetti.";
            if (m_auditOn) m_audit.record(label, command, false, true, -1, 0,
                                          QStringLiteral("kullanıcı reddi"));
            return r;
        }
        if (m_auditOn && !warn.isEmpty())
            m_audit.record(label + QStringLiteral("(uyarı)"), command, true, false, 0, 0, warn);
    }
    // Stage 38: kabuk kipi. Varsayılan "bash -c": kullanıcının .bashrc dosyası
    // yüklenmez, böylece alias/fonksiyon komutu sessizce değiştirmez.
    QProcess p;
    p.setWorkingDirectory(m_root.isEmpty() ? QDir::currentPath() : m_root);
    p.setProcessEnvironment(sanitizedEnvironment());
    const QStringList shellArgs = m_shellMode == ShellMode::Legacy
                                      ? QStringList{QStringLiteral("-lc"), command}
                                      : QStringList{QStringLiteral("-c"), command};
    QElapsedTimer timer;
    timer.start();
    p.start("bash", shellArgs);
    const bool finished = p.waitForFinished(timeoutMs);
    const qint64 elapsed = timer.elapsed();
    if (!finished) {
        p.kill();
        p.waitForFinished(1000);
        if (m_auditOn)
            m_audit.record(label, command, needsApproval, true, -1, elapsed,
                           QStringLiteral("zaman aşımı"));
        r.denied = true;
        r.output = "HATA: zaman aşımı (" + QString::number(timeoutMs) + " ms)";
        return r;
    }
    QString out = QString::fromUtf8(p.readAllStandardOutput());
    QString err = QString::fromUtf8(p.readAllStandardError());
    r.ok = p.exitCode() == 0;
    r.output = QString("$ %1\n[exit %2]\n%3%4")
                   .arg(command)
                   .arg(p.exitCode())
                   .arg(out.left(12000),
                        err.isEmpty() ? "" : "\n[stderr]\n" + err.left(4000));
    // Stage 38: denetim kaydı (onay durumu, çıkış kodu, süre)
    if (m_auditOn)
        m_audit.record(label, command, needsApproval, false, p.exitCode(), elapsed);
    return r;
}

ToolResult AgentTools::runCommand(const QString& command, int timeoutMs) {
    if (!m_allowCommand) {
        ToolResult r;
        r.denied = true;
        r.output = "run_command devre dışı (ayarlardan/onaydan aç).";
        return r;
    }
    return runShell(command, timeoutMs, "run_command", true);
}

ToolResult AgentTools::runTests(const QString& command, int timeoutMs) {
    // Politika + izin kapısı (otonom modda test koşmak serbest değildir)
    if (!m_policy.toolAllowed("run_tests")) {
        ToolResult r;
        r.denied = true;
        r.output = "Politika: test koşumu kapalı (ayarlardan açılır).";
        return r;
    }
    QString cmd = command.trimmed();
    if (cmd.isEmpty()) cmd = m_testCommand.trimmed();
    if (cmd.isEmpty()) cmd = guessTestCommand(m_root);
    if (cmd.isEmpty()) {
        ToolResult r;
        r.output = "HATA: test komutu bulunamadı; run_tests {command} ile ver.";
        return r;
    }
    ToolResult r = runShell(cmd, timeoutMs, "run_tests", m_policy.needsApproval("run_tests"));
    if (!r.ok && r.output.isEmpty()) r.output = "HATA: test komutu başarısız.";
    return r;
}

ToolResult AgentTools::readRange(const QString& path, int startLine, int endLine) {
    if (endLine < startLine) std::swap(startLine, endLine);
    return readFile(path, startLine, qMax(0, endLine - startLine + 1));
}

ToolResult AgentTools::grepLines(const QString& pattern, const QString& glob, int context,
                                 int maxHits) {
    const ToolResult base = search(pattern, glob, maxHits);
    if (!base.ok || context <= 0) return base;
    // Bağlam satırlarını ekle (yalnız eşleşen dosyalarda)
    QStringList out = base.output.split('\n');
    QStringList rich;
    for (const QString& line : out) {
        const int c1 = line.indexOf(':');
        if (c1 < 0) continue;
        const int c2 = line.indexOf(':', c1 + 1);
        if (c2 < 0) continue;
        const QString file = line.left(c1);
        const int ln = line.mid(c1 + 1, c2 - c1 - 1).toInt();
        QString fAbs, fWhy;
        if (!safePath(file, fAbs, fWhy)) continue; // Stage 38: sembolik bağlantı kaçışı
        QFile f(fAbs);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        const QStringList all = QString::fromUtf8(f.readAll()).split('\n');
        for (int i = qMax(1, ln - context); i <= qMin(all.size(), ln + context); ++i) {
            const QString mark = (i == ln) ? "»" : " ";
            rich << QString("%1%2:%3: %4").arg(mark).arg(file).arg(i).arg(all[i - 1].left(160));
        }
    }
    ToolResult r = base;
    r.output = rich.isEmpty() ? base.output : rich.join('\n');
    return r;
}

ToolResult AgentTools::findSymbol(const QString& name, int maxHits) {
    ToolResult r;
    const QString sym = name.trimmed();
    if (sym.isEmpty()) { r.output = "HATA: sembol adı boş."; return r; }
    const QRegularExpression rx(
        QString("\\b(class|struct|enum( class)?|namespace|union|interface|def|fn|func|function|"
                "void|int|bool|auto|double|float)\\s+%1\\b|\\b%1\\s*\\(|\\b%1\\s*::")
            .arg(QRegularExpression::escape(sym)));
    ToolResult s = search(rx.pattern(), QString(), maxHits);
    if (!s.ok) return s;
    r.ok = true;
    r.output = s.output == "(eşleşme yok)" ? QString("(sembol bulunamadı: %1)").arg(sym) : s.output;
    return r;
}

ToolResult AgentTools::gitStatus() {
    if (!m_gitRepo) { ToolResult r; r.output = "(bu klasör git deposu değil)"; return r; }
    return runShell("git status --porcelain=v1 -b", 15000, "git_status", false);
}

ToolResult AgentTools::gitDiff(bool staged, int maxChars) {
    if (!m_gitRepo) { ToolResult r; r.output = "(bu klasör git deposu değil)"; return r; }
    const QString cmd = staged ? "git diff --staged" : "git diff";
    ToolResult r = runShell(cmd, 20000, "git_diff", false);
    r.output = r.output.left(maxChars);
    return r;
}

ToolResult AgentTools::healthScan() {
    ToolResult r;
    r.ok = true;
    const ProjectHealth h = ProjectHealth::scan(m_root);
    QString out = h.summary() + "\n";
    out += QString("uzun dosya: %1 · dev dosya: %2").arg(h.longFiles).arg(h.hugeFiles);
    if (h.maxFileLines > 0) out += QString("\nen uzun: %1 (%2 satır)").arg(h.maxFilePath).arg(h.maxFileLines);
    for (const QString& n : h.notes) out += "\n- " + n;
    r.output = out;
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
    // Stage 34: politika kapısı — otonom modda yalnız salt-okunur araçlar çalışır.
    // Bilinmeyen araç adları kapıya giremez; aşağıda "Bilinmeyen araç" olarak döner.
    if (AgentPolicy::isReadOnlyTool(n) || AgentPolicy::isMutatingTool(n)) {
        const bool allowed =
            m_policy.autonomous ? m_policy.autonomousAllows(n) : m_policy.toolAllowed(n);
        if (!allowed) {
            r.denied = true;
            r.output = "Politika bu aracı kapattı: " + n +
                       (m_policy.autonomous ? " (otonom mod salt-okunur)" : " (izin yok)");
            return r;
        }
    }
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
    // --- Stage 34 araçları ---
    if (n == "read_range") {
        const QString p = call.args.value("path").toString();
        const int st = call.args.value("start").toInt(1);
        const int en = call.args.value("end").toInt(0);
        return en <= 0 ? readFile(p, st, -1) : readRange(p, st, en);
    }
    if (n == "grep_lines")
        return grepLines(call.args.value("pattern").toString(),
                         call.args.value("glob").toString(), call.args.value("context").toInt(0),
                         call.args.value("maxHits").toInt(60));
    if (n == "find_symbol")
        return findSymbol(call.args.value("name").toString(),
                          call.args.value("maxHits").toInt(40));
    if (n == "git_status")
        return gitStatus();
    if (n == "git_diff")
        return gitDiff(call.args.value("staged").toBool(), call.args.value("maxChars").toInt(12000));
    if (n == "run_tests")
        return runTests(call.args.value("command").toString());
    if (n == "health_scan")
        return healthScan();
    r.output = "Bilinmeyen araç: " + n;
    return r;
}

bool AgentTools::queueNeedsReview() const {
    return m_queue && m_queue->count() > m_queueThreshold;
}

QStringList AgentTools::benignSubstitutions() {
    QStringList out = {"nproc", "pwd", "hostname", "uname", "date", "dirname $0", "basename $0"};
    const QString saved =
        QSettings().value("agent/benignSubst", QString()).toString().trimmed().toLower();
    if (!saved.isEmpty()) {
        out.clear();
        for (const QString& w : saved.split(',', Qt::SkipEmptyParts)) {
            const QString t = w.trimmed();
            if (!t.isEmpty() && !out.contains(t)) out << t;
        }
    }
    return out;
}

QStringList AgentTools::defaultSessionAllowed() {
    return {"cmake", "ctest", "make", "ninja", "git", "ls", "cat", "grep", "find", "echo",
            "pwd", "head", "tail", "wc", "diff", "python3", "pytest", "npm", "node", "go",
            "cargo", "qmake"};
}

bool AgentTools::isSessionAllowable(const QString& command) {
    const QString first = command.trimmed().split(QRegularExpression("\\s+")).value(0);
    if (first.isEmpty() || !defaultSessionAllowed().contains(first)) return false;
    // Yazma/ağ komutları asla oturum iznine giremez
    if (isCommandBlocked(command)) return false;
    if (!pathWarning(command).isEmpty()) return false;
    return true;
}

bool AgentTools::sessionAllows(const QString& command) const {
    if (m_sessionAllowed.isEmpty()) return false;
    const QString first = command.trimmed().split(QRegularExpression("\\s+")).value(0);
    if (!m_sessionAllowed.contains(first)) return false;
    return isSessionAllowable(command);
}
