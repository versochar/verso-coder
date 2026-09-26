// Stage 27 testleri: sunucu tablosu + venv + URI turu.
// Çıkış kodu 0 = hepsi geçti.
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <cstdio>

#include "../src/core/LspClient.h"
#include "../src/core/LspServers.h"

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

static void testTable() {
    const LspServerDef* cpp = LspServers::forSuffix("cpp");
    CHECK(cpp && cpp->lang == "cpp" && cpp->program == "clangd");
    CHECK(LspServers::forSuffix("hpp") == cpp);
    const LspServerDef* py = LspServers::forSuffix("py");
    CHECK(py && py->lang == "python");
    CHECK(LspServers::forSuffix("rs")->program == "rust-analyzer");
    CHECK(LspServers::forSuffix("go")->program == "gopls");
    CHECK(LspServers::forSuffix("xyz") == nullptr);
    CHECK(!LspServers::table().isEmpty());
}

static void testVenv() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    // venv yoksa PATH'e düşer (boş olabilir — çökme yok)
    const QString noVenv = LspServers::pythonExe(dir.path());
    Q_UNUSED(noVenv);
    CHECK(LspServers::pythonVenvs(dir.path()).isEmpty());
    // sahte venv
    QDir().mkpath(dir.filePath(".venv/bin"));
    QFile f(dir.filePath(".venv/bin/pylsp"));
    CHECK(f.open(QIODevice::WriteOnly));
    f.write("#!/bin/sh\n");
    f.close();
    CHECK(LspServers::pythonVenvs(dir.path()).size() == 1);
    CHECK(LspServers::pythonExe(dir.path()).endsWith(".venv/bin/pylsp"));
}

static void testUri() {
    const QString p = "/tmp/a b/c.cpp";
    const QString u = LspClient::pathToUri(p);
    CHECK(u.startsWith("file://"));
    CHECK(LspClient::uriToPath(u) == p); // boşluk kodlamalı tur
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    testTable();
    testVenv();
    testUri();
    fprintf(stderr, "STAGE27: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
