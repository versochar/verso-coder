// Stage 20 testleri: yeni dosya dilleri (saf mantık, QtCore).
// Çıkış kodu 0 = hepsi geçti.
#include <QCoreApplication>
#include <QSet>
#include <QSettings>
#include <cstdio>

#include "../src/core/LanguageSupport.h"

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

static void testLanguageTable() {
    const auto all = LanguageSupport::newFileLanguages();
    CHECK(all.size() == 21); // 20 dil + Düz metin
    QSet<QString> ids;
    for (const auto& d : all) {
        CHECK(!d.id.isEmpty() && !d.title.isEmpty());
        CHECK(!ids.contains(d.id));
        ids << d.id;
        if (d.id != "plain") {
            CHECK(!d.exts.isEmpty());
            CHECK(!d.chip.isEmpty());
        }
    }
    // İstenen 20 dil mevcut mu?
    for (const char* id : {"c", "cpp", "python", "javascript", "typescript",
                           "java", "go", "rust", "ruby", "php", "swift",
                           "kotlin", "csharp", "html", "css", "json", "xml",
                           "markdown", "shell", "sql"})
        CHECK(ids.contains(QString::fromLatin1(id)));
}

static void testExtSkeleton() {
    CHECK(LanguageSupport::defaultExtension("python") == "py");
    CHECK(LanguageSupport::defaultExtension("cpp") == "cpp");
    CHECK(LanguageSupport::defaultExtension("csharp") == "cs");
    CHECK(LanguageSupport::defaultExtension("plain").isEmpty());
    CHECK(LanguageSupport::defaultExtension("yokboyle").isEmpty());
    CHECK(LanguageSupport::skeleton("python").contains("__main__"));
    CHECK(LanguageSupport::skeleton("html").contains("DOCTYPE"));
    CHECK(LanguageSupport::skeleton("plain").isEmpty());
    CHECK(LanguageSupport::findLang("go") != nullptr);
    CHECK(LanguageSupport::findLang("yokboyle") == nullptr);
}

static void testUntitledTitle() {
    CHECK(LanguageSupport::untitledTitle(3, "python") == "Adsız-3 • py");
    CHECK(LanguageSupport::untitledTitle(1, "plain") == "Adsız-1");
    CHECK(LanguageSupport::untitledTitle(2, "yokboyle") == "Adsız-2");
}

static void testRecents() {
    QSettings q("Verso", "VersoCoder");
    const QStringList backup = q.value("newfile/recent").toStringList();
    q.remove("newfile/recent");
    LanguageSupport::pushRecentLang("python");
    LanguageSupport::pushRecentLang("go");
    LanguageSupport::pushRecentLang("python"); // başa taşınır, kopya yok
    QStringList r = LanguageSupport::recentLangs();
    CHECK(r.size() == 2 && r[0] == "python" && r[1] == "go");
    LanguageSupport::pushRecentLang("rust");
    LanguageSupport::pushRecentLang("c");
    r = LanguageSupport::recentLangs();
    CHECK(r.size() == 3 && !r.contains("go")); // en çok 3, eskisi düşer
    if (backup.isEmpty()) q.remove("newfile/recent");
    else q.setValue("newfile/recent", backup);
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    testLanguageTable();
    testExtSkeleton();
    testUntitledTitle();
    testRecents();
    fprintf(stderr, "STAGE20: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
