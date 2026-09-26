// Stage 21 testleri: pano halkası + kısayol çakışması + palet skoru +
// bütçe uyarısı + ileti şablonları (saf mantık).
// Çıkış kodu 0 = hepsi geçti.
#include <QApplication>
#include <QMap>
#include <cstdio>

#include "../src/core/ClipboardRing.h"
#include "../src/core/CommitMsg.h"
#include "../src/core/ContextBudget.h"
#include "../src/core/ShortcutCheck.h"
#include "../src/widgets/CommandPalette.h"

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

static void testRing() {
    ClipboardRing r(3);
    CHECK(r.size() == 0 && r.next().isEmpty());
    r.push("a");
    r.push("b");
    r.push("c");
    CHECK(r.size() == 3);
    CHECK(r.next() == "b"); // halka turu: c -> b
    CHECK(r.next() == "a");
    CHECK(r.next() == "c"); // başa sarar
    r.push("b");            // kopya başa taşınır
    CHECK(r.current() == "b" && r.size() == 3);
    r.push("d");
    r.push("e");
    CHECK(r.size() == 3); // kap aşılmaz
}

static void testConflicts() {
    QMap<QString, QString> m;
    m["a"] = "Ctrl+N";
    m["b"] = "Ctrl+N";
    m["c"] = "";
    m["d"] = "Ctrl+S";
    const QStringList bad = ShortcutCheck::findConflicts(m);
    CHECK(bad.size() == 1 && bad[0] == "Ctrl+N: a, b");
    m["b"] = "Ctrl+M";
    CHECK(ShortcutCheck::findConflicts(m).isEmpty());
}

static void testMatchScore() {
    CHECK(CommandPalette::matchScore("", "x") == 1000);
    CHECK(CommandPalette::matchScore("zzz", "abc") == -1);
    CHECK(CommandPalette::matchScore("Tema", "Tema") == 100000);
    // önek > gövde-içi
    CHECK(CommandPalette::matchScore("te", "Tema") >
          CommandPalette::matchScore("te", "Kote"));
    // kelime başı > gövde-içi
    CHECK(CommandPalette::matchScore("ga", "Tema Galerisi") >
          CommandPalette::matchScore("ga", "Lagavulin"));
}

static void testBudget() {
    CHECK(ContextBudget::exceeds(QString(5000, 'x'), 100));
    CHECK(!ContextBudget::exceeds("kısa", 100));
    const QString w = ContextBudget::warnText(QString(5000, 'x'), 100);
    CHECK(w.contains("1250") && w.contains("100"));
}

static void testTemplates() {
    const QStringList t = CommitMsg::templates();
    CHECK(t.contains("feat: ") && t.contains("fix!: "));
    CHECK(CommitMsg::applyTemplate("feat", "") == "feat: ");
    CHECK(CommitMsg::applyTemplate("fix:", "x") == "fix: x");
}

int main(int argc, char** argv) {
    QApplication app(argc, argv); // CommandPalette (QDialog) bağımlılığı için
    testRing();
    testConflicts();
    testMatchScore();
    testBudget();
    testTemplates();
    fprintf(stderr, "STAGE21: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
