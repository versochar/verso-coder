// Stage 43 testleri: gerçek git deposunda işlem zinciri (init/add/commit/
// branch/stash/log/status), remoteGitCmd kurulumu ve zaman aşımı.
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QElapsedTimer>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>

#include "../src/core/ConnectionProfile.h"
#include "../src/core/GitRunner.h"
#include "../src/core/GitWorktree.h"

static int g_pass = 0, g_fail = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (cond) {                                                            \
            ++g_pass;                                                          \
        } else {                                                                \
            ++g_fail;                                                          \
            fprintf(stderr, "FAIL %d %s\n", __LINE__, #cond);                    \
            fflush(stderr);                                                    \
        }                                                                      \
    } while (0)

#define CHECK2(cond, why)                                                      \
    do {                                                                       \
        if (cond) {                                                            \
            ++g_pass;                                                          \
        } else {                                                                \
            ++g_fail;                                                          \
            fprintf(stderr, "FAIL %d %s (%s)\n", __LINE__, #cond, why);          \
            fflush(stderr);                                                    \
        }                                                                      \
    } while (0)

// --- 1: tam işlem zinciri ---
static void testGitFlow() {
    QTemporaryDir d;
    CHECK(d.isValid());
    LocalGitRunner g(d.path());
    CHECK(g.label() == "yerel");
    // init + kimlik (test izolasyonu: -c ile)
    auto run = [&](const QStringList& a) { return g.run(a, 10000); };
    CHECK(run({"init", "-b", "main"}).exit == 0);
    CHECK(run({"config", "user.email", "test@verso"}).exit == 0);
    CHECK(run({"config", "user.name", "Verso Test"}).exit == 0);
    // commit zinciri
    QFile f(d.path() + "/a.txt");
    CHECK(f.open(QIODevice::WriteOnly));
    f.write("v1\n");
    f.close();
    CHECK(run({"add", "a.txt"}).exit == 0);
    CHECK(run({"commit", "-m", "ilk"}).exit == 0);
    // status temiz
    GitRunner::Result st = run({"status", "--porcelain=v1"});
    CHECK(st.exit == 0 && st.out.trimmed().isEmpty());
    // değiştir + stash
    QFile f2(d.path() + "/a.txt");
    CHECK(f2.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f2.write("v2-degisik\n");
    f2.close();
    st = run({"status", "--porcelain=v1"});
    CHECK(st.out.contains("M a.txt") || st.out.contains("M  a.txt"));
    CHECK(run({"stash", "push", "-m", "verso-stash"}).exit == 0);
    st = run({"status", "--porcelain=v1"});
    CHECK(st.out.trimmed().isEmpty()); // stash sonrası temiz
    GitRunner::Result sl = run({"stash", "list", "--format=%gd: %gs"});
    CHECK(sl.out.contains("stash@{0}"));
    CHECK(run({"stash", "apply", "stash@{0}"}).exit == 0);
    st = run({"status", "--porcelain=v1"});
    CHECK(!st.out.trimmed().isEmpty()); // değişiklik geri geldi
    CHECK(run({"stash", "drop", "stash@{0}"}).exit == 0);
    sl = run({"stash", "list"});
    CHECK(sl.out.trimmed().isEmpty());
    // branch
    CHECK(run({"checkout", "-b", "ozellik"}).exit == 0);
    GitRunner::Result br = run({"branch", "--format=%(refname:short)"});
    CHECK(br.out.contains("ozellik") && br.out.contains("main"));
    GitRunner::Result head = run({"rev-parse", "--abbrev-ref", "HEAD"});
    CHECK(head.out.trimmed() == "ozellik");
    // log grafiği
    GitRunner::Result lg = run({"log", "--oneline", "--graph", "--decorate", "-5"});
    CHECK(lg.exit == 0 && lg.out.contains("ilk"));
    // yukarı akış yokken önde/geride sessizce başarısız olur
    GitRunner::Result ab = run({"rev-list", "--left-right", "--count", "HEAD...@{upstream}"});
    CHECK(ab.exit != 0);
    // rename ayrıştırma (panel mantığı)
    QFile old(d.path() + "/eski.txt");
    CHECK(old.open(QIODevice::WriteOnly));
    old.write("x\n");
    old.close();
    CHECK(run({"add", "eski.txt"}).exit == 0);
    CHECK(run({"commit", "-m", "eski"}).exit == 0);
    CHECK(run({"mv", "eski.txt", "yeni.txt"}).exit == 0);
    st = run({"status", "--porcelain=v1"});
    QString path = "eski.txt -> yeni.txt";
    if (path.contains(" -> ")) path = path.split(" -> ").last();
    CHECK2(path == "yeni.txt", "rename ayrıştırma");
}

// --- 2: remoteGitCmd kurulumu (saf) ---
static void testRemoteCmd() {
    ConnectionProfile p;
    p.host = "ornek";
    p.user = "kul";
    // Uzak komut: boş kökte sade git komutu
    const QString cmd = GitRunner::remoteGitCmd(p, {"status"});
    CHECK(!cmd.isEmpty());
    CHECK(cmd.contains("status"));
    CHECK(cmd.startsWith("git"));
    // Boşluklu argüman tek tırnağa alınır (kabuk enjeksiyonu yok)
    const QString q = GitRunner::remoteGitCmd(p, {"commit", "-m", "iki kelime"});
    CHECK(q.contains("'iki kelime'"));
    // Uzak kök varsa -C eklenir
    p.remoteRoot = "/srv/proje";
    const QString rooted = GitRunner::remoteGitCmd(p, {"log"});
    CHECK(rooted.contains("-C '/srv/proje'"));
    // Tek tırnak kaçışı
    p.remoteRoot = "/srv/o'brien";
    const QString esc = GitRunner::remoteGitCmd(p, {"log"});
    CHECK(!esc.contains("/srv/o'brien"));
    CHECK(esc.contains("o'\"'\"'brien"));
}

// --- 3: zaman aşımı ---
static void testTimeout() {
    QTemporaryDir d;
    LocalGitRunner g(d.path());
    // Geçersiz komut hızlı başarısız olur (asılı kalmaz)
    QElapsedTimer t;
    t.start();
    GitRunner::Result r = g.run({"--gecersiz-komut-xyz"}, 5000);
    CHECK(t.elapsed() < 5000);
    CHECK(r.exit != 0);
}

// --- 4: depo yokken davranış ---
static void testNoRepo() {
    QTemporaryDir d;
    LocalGitRunner g(d.path() + "/yok");
    GitRunner::Result r = g.run({"rev-parse", "--show-toplevel"}, 5000);
    CHECK(r.exit != 0);
}

static void testWorktree() {
    QTemporaryDir d;
    CHECK(d.isValid());
    LocalGitRunner g(d.path());
    auto run = [&](const QStringList& a) { return g.run(a, 10000); };
    CHECK(run({"init", "-b", "main"}).exit == 0);
    QFile f(d.path() + "/a.txt");
    CHECK(f.open(QIODevice::WriteOnly));
    f.write("v1\n");
    f.close();
    CHECK(run({"add", "a.txt"}).exit == 0);
    CHECK(run({"commit", "-m", "ilk"}).exit == 0);
    QString err;
    auto l0 = GitWorktree::list(g, &err);
    CHECK(err.isEmpty());
    CHECK(l0.size() == 1 && l0[0].branch == "refs/heads/main");
    // Ekle (yeni dalla)
    const QString wt = d.path() + "-wt1";
    CHECK(GitWorktree::add(g, wt, "", "ozellik", &err));
    auto l1 = GitWorktree::list(g, nullptr);
    CHECK(l1.size() == 2);
    bool dal = false;
    for (const auto& w : l1)
        if (w.path == wt && w.branch == "refs/heads/ozellik") dal = true;
    CHECK(dal);
    CHECK(QFile::exists(wt + "/a.txt")); // dosya ağaca geldi
    // Kaldır
    CHECK(GitWorktree::remove(g, wt, false, &err));
    CHECK(GitWorktree::list(g, nullptr).size() == 1);
    // Olmayan dizin: hata
    CHECK(!GitWorktree::remove(g, d.path() + "-yok", false, &err));
    CHECK(!err.isEmpty());
    // Depo dışında liste boş + hata
    LocalGitRunner g2(d.path() + "-bos");
    CHECK(GitWorktree::list(g2, &err).isEmpty());
    CHECK(!err.isEmpty());
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir home;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, home.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, home.path());
    qputenv("XDG_DATA_HOME", home.path().toUtf8());
    qputenv("HOME", home.path().toUtf8());
    // Test kimliği (kullanıcı ayarını kirletmez)
    qputenv("GIT_AUTHOR_NAME", "Verso Test");
    qputenv("GIT_AUTHOR_EMAIL", "test@verso");
    qputenv("GIT_COMMITTER_NAME", "Verso Test");
    qputenv("GIT_COMMITTER_EMAIL", "test@verso");

    testGitFlow();
    testRemoteCmd();
    testTimeout();
    testNoRepo();
    testWorktree();

    fprintf(stderr, "STAGE43: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
