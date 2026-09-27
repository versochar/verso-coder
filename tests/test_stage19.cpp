// Stage 19 testleri: TaskChain + CollabMerge + PluginEngine + LanguageSupport +
// ThreadMonitor + GitVersionManager + CollabSession (canlı localhost).
// QtTest yok: düz main + sayaç. Çıkış kodu 0 = hepsi geçti.
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFuture>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTimer>
#include <QtConcurrent>
#include <QtDebug>
#include <cstdio>

#include "../src/core/CollabMerge.h"
#include "../src/core/CollabSession.h"
#include "../src/core/GitVersionManager.h"
#include "../src/core/LanguageSupport.h"
#include "../src/core/PluginEngine.h"
#include "../src/core/TaskChain.h"
#include "../src/core/ThreadMonitor.h"

static int g_pass = 0, g_fail = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (cond) {                                                            \
            ++g_pass;                                                          \
        } else {                                                               \
            ++g_fail;                                                          \
            fprintf(stderr, "FAIL %d %s\n", __LINE__, #cond);                  \
            fflush(stderr);                                                    \
        }                                                                      \
    } while (0)

static void testTaskChainOrder() {
    QString err;
    QMap<QString, QStringList> deps;
    deps["build"] = {};
    deps["test"] = {"build"};
    deps["deploy"] = {"test"};
    const QList<QString> o = TaskChain::order(deps, err);
    CHECK(err.isEmpty());
    CHECK(o == (QList<QString>{"build", "test", "deploy"}));

    // Döngü
    QMap<QString, QStringList> cyc;
    cyc["a"] = {"b"};
    cyc["b"] = {"a"};
    const QList<QString> o2 = TaskChain::order(cyc, err);
    CHECK(o2.isEmpty());
    CHECK(!err.isEmpty());

    // Elmas
    QMap<QString, QStringList> dia;
    dia["d"] = {"b", "c"};
    dia["b"] = {"a"};
    dia["c"] = {"a"};
    dia["a"] = {};
    const QList<QString> o3 = TaskChain::order(dia, err);
    CHECK(err.isEmpty());
    CHECK(o3.size() == 4 && o3.first() == "a" && o3.last() == "d");
}

static void testTaskChainExpand() {
    QMap<QString, QString> inputs;
    inputs["ad"] = "main";
    const QString e = TaskChain::expand("g++ ${file} -o ${input:ad} @ ${workspaceFolder}",
                                        "/root", "/root/a.cpp", inputs);
    CHECK(e == "g++ /root/a.cpp -o main @ /root");
    CHECK(TaskChain::expand("${fileBasename} ${fileDirname}", "/r", "/r/x/y.cpp", {}) ==
          "y.cpp /r/x");
}

static void testTaskChainInputs() {
    QJsonArray arr;
    arr.append(QJsonObject{{"id", "ad"}, {"default", "main"}});
    arr.append(QJsonObject{{"id", "bayrak"}, {"default", "-O2"}});
    const auto m = TaskChain::parseInputs(arr);
    CHECK(m.value("ad") == "main" && m.value("bayrak") == "-O2");
}

static void testTaskChainProblems() {
    const QString out = "src/a.cpp:10:5: error: tanımsız değişken\n"
                        "notaproblem line\n"
                        "main.py:3:1: warning: unused\n";
    const auto issues = TaskChain::matchProblems(out, "$gcc", "/root");
    CHECK(issues.size() == 2);
    CHECK(issues[0].file == "/root/src/a.cpp" && issues[0].line1 == 10 &&
          issues[0].col1 == 5);
    CHECK(TaskChain::matchProblems(out, "$null", "/root").isEmpty());

    QJsonArray tasks;
    tasks.append(QJsonObject{{"label", "t"}, {"dependsOn", "build"}});
    tasks.append(QJsonObject{{"label", "d"},
                             {"dependsOn", QJsonArray{"t", "build"}}});
    const auto dm = TaskChain::depsFrom(tasks);
    CHECK(dm.value("t") == QStringList{"build"});
    CHECK(dm.value("d") == (QStringList{"t", "build"}));
}

static void testCollabMerge() {
    CHECK(CollabMerge::validType("edit") && CollabMerge::validType("sync"));
    CHECK(!CollabMerge::validType("hack"));

    // diff: ekleme + silme yok
    CHECK(CollabMerge::diff("abc", "abc").isEmpty());
    const auto ops1 = CollabMerge::diff("abc", "aXbc");
    CHECK(ops1.size() == 1 && ops1[0].kind == "ins" && ops1[0].pos == 1 &&
          ops1[0].text == "X");
    const auto ops2 = CollabMerge::diff("aXbc", "abc");
    CHECK(ops2.size() == 1 && ops2[0].kind == "del" && ops2[0].len == 1);

    // roundtrip
    const QJsonObject edit = CollabMerge::makeEdit("u", 3, ops1);
    CHECK(CollabMerge::parseOps(edit).size() == 1);

    // apply: uzak ekleme yerel metne uygulanır
    int rev = 0;
    const QString merged = CollabMerge::apply("abc", ops1, {}, rev);
    CHECK(merged == "aXbc" && rev == 1);

    // transform: yerel ekleme sonrası uzak ekleme kayar
    const QList<CollabOp> local = {{"ins", 0, "!", 0}};
    const CollabOp t = CollabMerge::transform({"ins", 1, "X", 0}, local);
    CHECK(t.pos == 2);
}

static void testPluginEngine() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    QFile f(dir.filePath("echo.js"));
    CHECK(f.open(QIODevice::WriteOnly));
    f.write("// @permission fs.read\n"
            "verso.registerCommand(\"echo\", \"Echo\", function(arg) { return \"hi:\" + arg; });\n"
            "verso.log(\"yüklendi\");\n");
    f.close();

    PluginEngine eng;
    QStringList logs;
    QObject::connect(&eng, &PluginEngine::commandRegistered, &eng,
                     [&](const QString& id, const QString&) { logs << id; });
    const auto pls = eng.loadAll(dir.path());
    CHECK(pls.size() == 1 && pls[0].loaded);
    CHECK(pls[0].permissions.contains("fs.read"));
    CHECK(logs.size() == 1);
    const QJSValue r = eng.callCommand(logs[0], "x");
    CHECK(!r.isError() && r.toString() == "hi:x");
    CHECK(eng.isLoaded("echo"));
    CHECK(eng.unload("echo"));
    CHECK(!eng.isLoaded("echo"));

    // İzin yoksa okuma engellenir + sinyal
    QTemporaryDir dir2;
    QFile f2(dir2.filePath("sneaky.js"));
    CHECK(f2.open(QIODevice::WriteOnly));
    f2.write("var x = verso.readFile(\"/etc/hostname\");\n"
             "verso.registerCommand(\"noop\", \"N\", function() { return x === \"\" ? \"denied\" : \"open\"; });\n");
    f2.close();
    QStringList denied;
    QObject::connect(&eng, &PluginEngine::permissionDenied, &eng,
                     [&](const QString& id, const QString& perm) {
                         denied << id + ":" + perm;
                     });
    eng.loadAll(dir2.path());
    CHECK(denied.size() == 1 && denied[0] == "sneaky:fs.read");
    const QJSValue r2 = eng.callCommand("plugin.sneaky.noop");
    CHECK(r2.toString() == "denied");

    // fs.write izniyle yazma
    QTemporaryDir dir3;
    QFile f3(dir3.filePath("writer.js"));
    CHECK(f3.open(QIODevice::WriteOnly));
    f3.write("// @permission fs.write\n"
             "verso.registerCommand(\"w\", \"W\", function(p) { return verso.writeFile(p, \"ok\"); });\n");
    f3.close();
    // Stage 41: eklenti yazma çalışma alanına kapsamlıdır — kök kurulur
    eng.setWorkspaceRoot(dir3.path());
    eng.loadAll(dir3.path());
    const QJSValue r3 = eng.callCommand("plugin.writer.w", "out.txt");
    CHECK(r3.toBool() == true);
    QFile check(dir3.filePath("out.txt"));
    CHECK(check.open(QIODevice::ReadOnly) &&
          QString::fromUtf8(check.readAll()) == "ok");
}

static void testLanguageSupport() {
    LanguageSupport ls;
    CHECK(ls.detectLanguage("a.cpp") == "cpp");
    CHECK(ls.detectLanguage("x.PY") == "python");
    CHECK(ls.detectLanguage("main.ts") == "typescript");
    CHECK(ls.detectLanguage("bilinmeyen.xyz") == "unknown");

    QTemporaryDir dir;
    QFile f1(dir.filePath("a.cpp"));
    CHECK(f1.open(QIODevice::WriteOnly));
    f1.close();
    QFile f2(dir.filePath("b.py"));
    CHECK(f2.open(QIODevice::WriteOnly));
    f2.close();
    const QStringList langs = ls.scanDirectoryLanguages(dir.path());
    CHECK(langs.size() == 1 && (langs[0] == "cpp" || langs[0] == "python"));
}

static void testThreadMonitor() {
    ThreadMonitor mon;
    QEventLoop loop;
    bool done = false;
    QObject::connect(&mon, &ThreadMonitor::finished, &mon,
                     [&](const QString&) { done = true; });
    mon.watch<QString>("job", QtConcurrent::run([] { return QString("x"); }));
    // Gerçek iş bitene kadar bekle (en çok 5 sn)
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    QObject::connect(&mon, &ThreadMonitor::finished, &loop, &QEventLoop::quit);
    if (!done) loop.exec();
    CHECK(done);
    CHECK(mon.status().value("job") == "bitti");
    mon.unwatch("job");
    CHECK(!mon.status().contains("job"));
}

static void testGitVersion() {
    GitVersionManager g;
    QStringList errs;
    QObject::connect(&g, &GitVersionManager::errorOccurred, &g,
                     [&](const QString& m) { errs << m; });
    CHECK(g.currentVersion().isEmpty()); // kök yok
    CHECK(!errs.isEmpty());
    CHECK(g.lastCommitMessage().isEmpty());
}

static bool waitFor(QEventLoop& loop, int ms) {
    bool timeout = false;
    QTimer::singleShot(ms, &loop, [&]() {
        timeout = true;
        loop.quit();
    });
    loop.exec();
    return !timeout;
}

static void testCollabLive() {
    CollabSession host, peer;
    const int port = host.host("evsahibi", 0);
    CHECK(port > 0);
    host.setBaseText("merhaba");

    bool hostOn = false, peerOn = false;
    QString gotText, gotFrom;
    QEventLoop loop;
    QObject::connect(&host, &CollabSession::activeChanged, &host,
                     [&](bool on) { hostOn = on; });
    QObject::connect(&peer, &CollabSession::activeChanged, &peer, [&](bool on) {
        peerOn = on;
        if (on) loop.quit();
    });
    QObject::connect(&peer, &CollabSession::textMerged, &peer,
                     [&](const QString& t, const QString& from) {
                         gotText = t;
                         gotFrom = from;
                         loop.quit();
                     });
    CHECK(peer.join(QString("ws://127.0.0.1:%1").arg(port), "misafir"));
    waitFor(loop, 5000);
    CHECK(peerOn);

    // İmleç + metin akışı
    bool cursors = false;
    QObject::connect(&host, &CollabSession::cursorsChanged, &host, [&]() {
        cursors = true;
        if (!gotText.isEmpty()) loop.quit();
    });
    peer.publishCursor(2, 4);
    host.publishText("merhaba dünya");
    waitFor(loop, 5000);
    CHECK(cursors);
    CHECK(gotFrom == "evsahibi" || gotText == "merhaba dünya");

    host.leave();
    peer.leave();
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    testTaskChainOrder();
    testTaskChainExpand();
    testTaskChainInputs();
    testTaskChainProblems();
    testCollabMerge();
    testPluginEngine();
    testLanguageSupport();
    testThreadMonitor();
    testGitVersion();
    testCollabLive();
    fprintf(stderr, "STAGE19: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
