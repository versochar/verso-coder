// Stage 30 testleri: birleştirme + ssh-config + pano sabitleme + zincir sırası.
// Çıkış kodu 0 = hepsi geçti.
#include <QCoreApplication>
#include <QMap>
#include <cstdio>

#include "../src/core/ClipboardRing.h"
#include "../src/core/MergeParse.h"
#include "../src/core/SshConfig.h"
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

static void testMerge() {
    const QString t = "a\n<<<<<<< HEAD\nx = 1\n=======\nx = 2\n>>>>>>> dal\nb\n";
    const auto hs = MergeParse::find(t);
    CHECK(hs.size() == 1);
    CHECK(hs[0].startLine == 2 && hs[0].endLine == 6);
    CHECK(hs[0].ours == (QStringList{"x = 1"}));
    CHECK(hs[0].theirs == (QStringList{"x = 2"}));
    CHECK(MergeParse::resolve(hs[0], 0) == (QStringList{"x = 1"}));
    CHECK(MergeParse::resolve(hs[0], 1) == (QStringList{"x = 2"}));
    CHECK(MergeParse::resolve(hs[0], 2) == (QStringList{"x = 1", "x = 2"}));
    CHECK(MergeParse::applyAll(t, {1}) == "a\nx = 2\nb\n");
    CHECK(MergeParse::find("temiz dosya\n").isEmpty());
    // Tabanlı (diff3) işaret
    const QString t3 = "<<<<<<< A\na\n||||||| base\nb\n=======\nc\n>>>>>>> B\n";
    const auto h3 = MergeParse::find(t3);
    CHECK(h3.size() == 1 && h3[0].base == (QStringList{"b"}));
    CHECK(MergeParse::applyAll(t3, {0}) == "a\n");
}

static void testSsh() {
    const QString cfg = "# yorum\nHost sunucu\n  HostName 10.0.0.1\n  User ali\n"
                        "  Port 2222\n  IdentityFile ~/.ssh/id_x\n\n"
                        "Host *.joker\n  User x\n\nHost sade\n";
    const auto es = SshConfig::parse(cfg);
    CHECK(es.size() == 2);
    CHECK(es[0].host == "sunucu" && es[0].hostName == "10.0.0.1");
    CHECK(es[0].user == "ali" && es[0].port == 2222);
    CHECK(es[0].identityFile == "~/.ssh/id_x");
    CHECK(es[1].host == "sade" && es[1].hostName == "sade" && es[1].port == 22);
    const ConnectionProfile p = SshConfig::toProfile(es[0]);
    CHECK(p.name == "sunucu" && p.host == "10.0.0.1" && p.user == "ali");
    CHECK(p.keyPath.endsWith(".ssh/id_x"));
}

static void testRingPin() {
    ClipboardRing r;
    r.push("a");
    r.push("b");
    r.pin("a");
    const QStringList items = r.items();
    CHECK(items.size() == 2 && items[0] == "a" && items[1] == "b");
    r.unpin("a");
    CHECK(!r.pinned().contains("a"));
}

static void testChainOrder() {
    QMap<QString, QStringList> deps;
    deps["derle"] = {};
    deps["test"] = {"derle"};
    deps["paket"] = {"test"};
    QString err;
    const QList<QString> o = TaskChain::order(deps, err);
    CHECK(err.isEmpty() && o.size() == 3 && o.last() == "paket");
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    testMerge();
    testSsh();
    testRingPin();
    testChainOrder();
    fprintf(stderr, "STAGE30: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
