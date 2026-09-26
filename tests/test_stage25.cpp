// Stage 25 testleri: istem değişkenleri + makro + araç/şablon ayrıştırma.
// Çıkış kodu 0 = hepsi geçti.
#include <QCoreApplication>
#include <QSettings>
#include <cstdio>

#include "../src/core/ExternalTools.h"
#include "../src/core/MacroRecorder.h"
#include "../src/core/PromptVars.h"

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

static void testPromptVars() {
    QMap<QString, QString> v;
    v["selection"] = "kod";
    CHECK(PromptVars::expand("açıkla: {{selection}}", v) == "açıkla: kod");
    CHECK(PromptVars::expand("x{{yok}}y", v) == "x{{yok}}y"); // bilinmeyen korunur
    const QString e = PromptVars::expand("{{filename}} ({{lang}}) {{date}}", "s",
                                         "/r/main.py");
    CHECK(e.startsWith("main.py (py) "));
    CHECK(e.size() > 14); // tarih eklendi
}

static void testMacro() {
    MacroRecorder m;
    m.push("file.save"); // kayıt kapalıyken yoksayılır
    CHECK(m.commands().isEmpty());
    m.start();
    m.push("file.save");
    m.push("file.save"); // art arda kopya teklenir
    m.push("macro.play"); // makro komutları kayda girmez
    m.push("nav.palette");
    m.stop();
    m.push("file.save");
    CHECK(m.commands() == (QStringList{"file.save", "nav.palette"}));
    m.save();
    MacroRecorder m2;
    m2.load();
    CHECK(m2.commands() == (QStringList{"file.save", "nav.palette"}));
    m.clear();
    m.save();
    MacroRecorder m3;
    m3.load();
    CHECK(m3.commands().isEmpty());
}

static void testTools() {
    QString err;
    const auto ts = ExternalTools::parse(
        R"({"tools": [{"label": "Test Koş", "command": "pytest -q"}, {"label": "", "command": "x"}]})",
        &err);
    CHECK(err.isEmpty() && ts.size() == 1);
    CHECK(ts[0].label == "Test Koş" && ts[0].command == "pytest -q");
    CHECK(ExternalTools::commandId("Test Koş").startsWith("tool."));
    CHECK(ExternalTools::configPathForRoot("/r").endsWith(".verso/tools.json"));
    const auto bad = ExternalTools::parse("bozuk", &err);
    CHECK(bad.isEmpty() && !err.isEmpty());
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QSettings("Verso", "VersoCoder").remove("macro/last"); // izole başla
    testPromptVars();
    testMacro();
    testTools();
    fprintf(stderr, "STAGE25: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
