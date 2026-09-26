// Stage 28 testleri: yer imleri + yorum öneki + görev girdileri + shebang.
// Çıkış kodu 0 = hepsi geçti.
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>

#include "../src/core/BookmarkStore.h"
#include "../src/core/LanguageSupport.h"
#include "../src/core/TaskChain.h"

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

static void testBookmarks() {
    BookmarkStore b;
    b.toggle("/a.cpp", 10);
    b.toggle("/a.cpp", 5);
    b.toggle("/a.cpp", 10); // geri al
    CHECK(b.lines("/a.cpp") == (QList<int>{5}));
    CHECK(b.has("/a.cpp", 5) && !b.has("/a.cpp", 10));
    b.toggle("/a.cpp", 20);
    CHECK(b.next("/a.cpp", 5) == 20 && b.next("/a.cpp", 20) == -1);
    CHECK(b.prev("/a.cpp", 20) == 5 && b.prev("/a.cpp", 5) == -1);
    b.save();
    BookmarkStore b2;
    b2.load();
    CHECK(b2.lines("/a.cpp") == (QList<int>{5, 20}));
    QSettings("Verso", "VersoCoder").remove("bookmarks"); // temizle
}

static void testComment() {
    CHECK(LanguageSupport::commentPrefix("a.cpp") == "//");
    CHECK(LanguageSupport::commentPrefix("x.py") == "#");
    CHECK(LanguageSupport::commentPrefix("q.sql") == "--");
    CHECK(LanguageSupport::commentPrefix("p.html") == "<!--");
    CHECK(LanguageSupport::commentPrefix("python") == "#");
    CHECK(LanguageSupport::commentPrefix("bilinmeyenxyz") == "");
}

static void testInputs() {
    QJsonArray arr;
    arr.append(QJsonObject{{"id", "ad"}, {"type", "promptString"},
                           {"default", "main"}, {"description", "Ad?"}});
    arr.append(QJsonObject{{"id", "mod"},
                           {"type", "pickString"},
                           {"options", QJsonArray{"a", "b"}}});
    arr.append(QJsonObject{{"id", "onay"}, {"type", "confirm"}});
    arr.append(QJsonObject{{"type", "promptString"}}); // idsiz atlanır
    const auto ins = TaskChain::parseTaskInputs(arr);
    CHECK(ins.size() == 3);
    CHECK(ins[0].id == "ad" && ins[0].def == "main" && ins[0].description == "Ad?");
    CHECK(ins[1].type == "pickString" && ins[1].options == (QStringList{"a", "b"}));
    CHECK(ins[2].type == "confirm");
    // Genişletme ${input:} ile çalışır
    QMap<QString, QString> vals;
    vals["ad"] = "X";
    CHECK(TaskChain::expand("g++ -o ${input:ad}", "/r", "", vals) == "g++ -o X");
}

static void testShebang() {
    LanguageSupport ls;
    QTemporaryDir dir;
    CHECK(dir.isValid());
    QFile f1(dir.filePath("kos"));
    CHECK(f1.open(QIODevice::WriteOnly));
    f1.write("#!/usr/bin/env python3\nprint(1)\n");
    f1.close();
    CHECK(ls.detectLanguage(f1.fileName()) == "python");
    QFile f2(dir.filePath("run"));
    CHECK(f2.open(QIODevice::WriteOnly));
    f2.write("#!/bin/bash\necho hi\n");
    f2.close();
    CHECK(ls.detectLanguage(f2.fileName()) == "shell");
    CHECK(ls.detectLanguage(dir.filePath("Makefile")) == "make");
    CHECK(ls.detectLanguage(dir.filePath("Dockerfile")) == "docker");
    CHECK(ls.detectLanguage(dir.filePath("x.py")) == "python"); // regresyon
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    testBookmarks();
    testComment();
    testInputs();
    testShebang();
    fprintf(stderr, "STAGE28: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
