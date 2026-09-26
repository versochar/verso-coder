// Stage 24 testleri: semboller + metrik + kopya + blame + todo (saf mantık).
// Çıkış kodu 0 = hepsi geçti.
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <cstdio>

#include "../src/core/CodeMetrics.h"
#include "../src/core/DuplicateFinder.h"
#include "../src/core/GitBlame.h"
#include "../src/core/WorkspaceSymbols.h"
#include "../src/widgets/TodoPanel.h"

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

static void testSymbols() {
    const QString py = "class Foo:\n    def bar(self):\n        pass\n\ndef top():\n    pass\n";
    const auto ss = WorkspaceSymbols::scanFile("a.py", py);
    CHECK(ss.size() == 3);
    CHECK(ss[0].kind == "class" && ss[0].name == "Foo" && ss[0].line == 1);
    CHECK(ss[1].name == "bar" && ss[1].line == 2);
    const QString cpp = "int add(int a, int b) {\nreturn a + b;\n}\n";
    const auto cs = WorkspaceSymbols::scanFile("a.cpp", cpp);
    CHECK(cs.size() == 1 && cs[0].name == "add");
    QMap<QString, QString> all;
    all["a.py"] = py;
    all["a.cpp"] = cpp;
    CHECK(WorkspaceSymbols::scanFiles(all).size() == 4);
    CHECK(WorkspaceSymbols::query(WorkspaceSymbols::scanFiles(all), "ad").size() == 1);
    CHECK(WorkspaceSymbols::refCount("add(1);\nadd(2);\n", "add", -1) == 2);
    CHECK(WorkspaceSymbols::refCount("int add() {}\nadd();\n", "add", 1) == 1);
}

static void testMetrics() {
    const QString t = "int f() {\n// yorum\n\nif (a && b) {\nreturn 1;\n}\n}\n";
    const CodeMetrics m = CodeMetrics::analyze("a.cpp", t, 1);
    CHECK(m.lines == 8 && m.functions == 1);
    CHECK(m.codeLines == 5); // yorum + boş satır + sondaki boşluk hariç
    CHECK(m.branches == 2 && m.complexity() == 3);
}

static void testDupes() {
    QMap<QString, QString> files;
    const QString blk = "int x = 1;\nint y = 2;\nint z = x + y;\nreturn z;\nfoo();\nbar();\n";
    files["a.cpp"] = blk + "int unique_a = 1;\n";
    files["b.cpp"] = "int unique_b = 2;\n" + blk;
    const auto gs = DuplicateFinder::find(files, 6);
    CHECK(gs.size() >= 1 && gs[0].files.size() == 2 && gs[0].lines == 6);
    QMap<QString, QString> solo;
    solo["a.cpp"] = "int a = 1;\n";
    CHECK(DuplicateFinder::find(solo, 6).isEmpty());
}

static void testBlame() {
    const QString por =
        "abc1234 1 1 1\nauthor Ali\n"
        "author-time 1700000000\nauthor-tz +0300\n"
        "\tint x = 1;\n"
        "def4567 2 2 1\nauthor Zeynep\n"
        "author-time 1700100000\nauthor-tz +0300\n"
        "\tint y = 2;\n";
    const auto bl = GitBlame::parse(por);
    CHECK(bl.size() == 2 && bl[0].line == 1 && bl[0].author == "Ali");
    CHECK(bl[1].authorTime == 1700100000LL);
    CHECK(GitBlame::ageDays(bl[0], 1700000000LL + 86400) == 1);
    CHECK(GitBlame::ageDays(bl[0], 1700000000LL) == 0);
    const double h = GitBlame::heat(bl[0], 1700000000LL + 86400 * 365, 365);
    CHECK(h > 0.99 && h <= 1.0);
}

static void testTodo() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    QFile f(dir.filePath("a.cpp"));
    CHECK(f.open(QIODevice::WriteOnly));
    f.write("// TODO @veli !p1: kritik iş\n// FIXME: normal iş\n// NOTE !p3 @zeynep: düşük\n");
    f.close();
    const auto hits = TodoPanel::scanSync(dir.path(), "TODO|FIXME|HACK|XXX|BUG|NOTE", 100);
    CHECK(hits.size() == 3);
    CHECK(hits[0].assignee == "veli" && hits[0].priority == 1);
    CHECK(hits[1].priority == 0);
    CHECK(hits[2].assignee == "zeynep" && hits[2].priority == 3);
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    testSymbols();
    testMetrics();
    testDupes();
    testBlame();
    testTodo();
    fprintf(stderr, "STAGE24: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
