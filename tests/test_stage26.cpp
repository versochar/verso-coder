// Stage 26 testleri: MI ayrıştırma + alıntılama + launch değişkenleri.
// Çıkış kodu 0 = hepsi geçti.
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <cstdio>

#include "../src/core/GdbDriver.h"
#include "../src/core/LaunchConfig.h"
#include "../src/core/MiParser.h"

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

static void testMiParse() {
    MiRecord r = MiParser::parseLine("3^done,bkpt={number=\"1\"}");
    CHECK(r.kind == "result" && r.token == "3" && r.cls == "done");
    CHECK(r.fields.value("bkpt").toMap().value("number").toString() == "1");
    MiRecord s = MiParser::parseLine("*stopped,reason=\"breakpoint-hit\"");
    CHECK(s.kind == "exec" && s.cls == "stopped");
    CHECK(s.fields.value("reason").toString() == "breakpoint-hit");
    MiRecord c = MiParser::parseLine("~\"ok\\n\"");
    CHECK(c.kind == "console" && c.stream == "ok\n");
    MiRecord u = MiParser::parseLine("(gdb)");
    CHECK(u.kind == "prompt" || u.kind == "unknown");
}

static void testMiQuote() {
    CHECK(GdbDriver::miQuote("a b") == "\"a b\"");
    CHECK(GdbDriver::miQuote("x\"y") == "\"x\\\"y\"");
    CHECK(GdbDriver::miQuote("p\\q") == "\"p\\\\q\"");
}

static void testLaunchVars() {
    CHECK(LaunchConfig::expandVars("${workspaceFolder}/b/${fileBasename}", "/r",
                                   "/r/a/main.cpp") == "/r/b/main.cpp");
    CHECK(LaunchConfig::expandVars("${fileDirname}", "/r", "/r/a/x.cpp") == "/r/a");
    CHECK(LaunchConfig::expandVars("sade", "/r") == "sade");
    CHECK(LaunchConfig::knownKeys().contains("program"));
    QTemporaryDir dir;
    CHECK(dir.isValid());
    QDir().mkpath(dir.filePath(".verso"));
    QFile f(dir.filePath(".verso/launch.json"));
    CHECK(f.open(QIODevice::WriteOnly));
    f.write(R"({"program": "build/a", "typoKey": 1})");
    f.close();
    const QStringList unk = LaunchConfig::unknownKeys(dir.path());
    CHECK(unk.size() == 1 && unk[0] == "typoKey");
    LaunchConfig lc = LaunchConfig::load(dir.path());
    CHECK(lc.program == "build/a");
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    testMiParse();
    testMiQuote();
    testLaunchVars();
    fprintf(stderr, "STAGE26: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
