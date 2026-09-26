// Stage 29 testleri: eklenti v2 (yükleme, izin, olay, durum, görünüm, karantina).
// Çıkış kodu 0 = hepsi geçti.
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>

#include "../src/core/PluginEngine.h"

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

static void cleanSettings() {
    QSettings q("Verso", "VersoCoder");
    q.remove("plugin");
}

static QString writePlugin(const QString& dir, const QString& id, const QString& src) {
    QFile f(dir + "/" + id + ".js");
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) f.write(src.toUtf8());
    return f.fileName();
}

static void testLoadV2() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    writePlugin(dir.path(), "v2demo",
                "// @name Demo\n// @version 2.0\n// @permission ui, fs.read, events\n"
                "verso.onEvent(\"save\", function(p) { verso.log(\"saved:\" + p); });\n"
                "verso.registerCommand(\"kos\", \"Koş\", function(a) { return \"ok:\" + a; });\n"
                "verso.registerView(\"panel\", \"Panel\", function() { return \"<b>hi</b>\"; });\n"
                "verso.setGlobalState(\"k\", \"v\");\n");
    PluginEngine eng;
    QStringList logs;
    QObject::connect(&eng, &PluginEngine::pluginLog, &eng,
                     [&](const QString& id, const QString& m) { logs << id + ":" + m; });
    const auto pls = eng.loadAll(dir.path());
    CHECK(pls.size() == 1 && pls[0].loaded);
    CHECK(pls[0].name == "Demo" && pls[0].version == "2.0");
    CHECK(pls[0].permissions.contains("ui") && pls[0].permissions.contains("events"));
    CHECK(eng.callCommand("plugin.v2demo.kos", "x").toString() == "ok:x");
    CHECK(eng.renderView("plugin.v2demo.view.panel") == "<b>hi</b>");
    eng.fireEvent("save", "/a.cpp");
    bool seen = false;
    for (const QString& l : logs)
        if (l.contains("saved:/a.cpp")) seen = true;
    CHECK(seen);
}

static void testDeniedAndOverride() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    writePlugin(dir.path(), "sneak", "var x = verso.readFile(\"/etc/hostname\");\n");
    PluginEngine eng;
    QStringList denied;
    QObject::connect(&eng, &PluginEngine::permissionDenied, &eng,
                     [&](const QString& id, const QString& p) {
                         denied << id + ":" + p;
                     });
    eng.loadAll(dir.path());
    CHECK(denied.size() == 1 && denied[0] == "sneak:fs.read");
    // Yönetici tüm izinleri verse bile bildirim yok, okuma çalışır
    denied.clear();
    eng.setPermOverride("sneak", {"fs.read"});
    eng.loadAll(dir.path());
    CHECK(denied.isEmpty());
}

static void testQuarantine() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    writePlugin(dir.path(), "bozuk", "throw new Error(\"patla\");\n");
    PluginEngine eng;
    for (int i = 0; i < 3; ++i) eng.loadAll(dir.path());
    eng.loadAll(dir.path()); // 4. yükleme: karantina listesinde görülür
    const auto pls = eng.plugins();
    CHECK(pls.size() == 1 && pls[0].quarantined);
    CHECK(!eng.isLoaded("bozuk"));
    eng.clearQuarantine("bozuk");
    eng.loadAll(dir.path());
    // Hâlâ hatalı ama sayaç sıfırlandı → karantinada değil (1 hata)
    CHECK(!eng.plugins().isEmpty() && !eng.plugins()[0].quarantined);
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    cleanSettings();
    testLoadV2();
    testDeniedAndOverride();
    testQuarantine();
    cleanSettings();
    fprintf(stderr, "STAGE29: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
