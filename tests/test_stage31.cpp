// Stage 31 testleri: bütünlük + geri çekilme + rotasyon + kota + MI fuzz.
// Çıkış kodu 0 = hepsi geçti.
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <QTemporaryDir>
#include <cstdio>

#include "../src/core/BackupManager.h"
#include "../src/core/MiParser.h"
#include "../src/core/Stability.h"

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

static void testCheckedIO() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString p = dir.filePath("s.json");
    QString err;
    CHECK(Stability::writeChecked(p, "{\"a\":1}", &err) && err.isEmpty());
    QByteArray back;
    CHECK(Stability::readChecked(p, back) && back == "{\"a\":1}");
    // Boz: mühür tutmaz
    QFile f(p);
    CHECK(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write("{\"a\":2}");
    f.close();
    CHECK(!Stability::readChecked(p, back));
    CHECK(Stability::shaData("abc").size() == 64);
    CHECK(Stability::shaFile(dir.filePath("yok")).isEmpty());
}

static void testBackoff() {
    Stability::Backoff b;
    CHECK(b.delayFor(0) == 1000);
    CHECK(b.delayFor(1) == 2000);
    CHECK(b.delayFor(2) == 4000);
    CHECK(b.delayFor(100) == 60000); // tavan
    Stability::Backoff c{500, 1500};
    CHECK(c.delayFor(0) == 500 && c.delayFor(5) == 1500);
}

static void testRotate() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString p = dir.filePath("a.log");
    QFile f(p);
    CHECK(f.open(QIODevice::WriteOnly));
    f.write(QByteArray(3000, 'x'));
    f.close();
    CHECK(LogRotate::rotate(p, 3, 1000));
    CHECK(QFile::exists(p + ".1"));
    CHECK(QFile(p + ".1").size() == 3000);
    // Küçük dosya dönmez
    QFile g(dir.filePath("b.log"));
    CHECK(g.open(QIODevice::WriteOnly));
    g.write("az");
    g.close();
    CHECK(LogRotate::rotate(dir.filePath("b.log"), 3, 1000));
    CHECK(!QFile::exists(dir.filePath("b.log.1")));
}

static void testQuota() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    BackupManager bm(dir.path());
    for (int i = 0; i < 5; ++i)
        bm.save(QString("/orijinal/f%1.cpp").arg(i), QString(2000, QChar('a' + i)));
    CHECK(bm.list().size() == 5);
    // Kota: toplam ~10KB → 3KB tavan eskileri siler, en yeniler kalır
    const int removed = bm.pruneByQuota(3000, 0);
    CHECK(removed > 0);
    const auto left = bm.list();
    CHECK(!left.isEmpty() && left.size() < 5);
    // En yeni korunur (liste yeniden eskiye)
    CHECK(left.first().original.endsWith("f4.cpp"));
}

static void testMiFuzz() {
    // Tohumlu fuzz: geçerli satırları boz, çözümleyici çökmemeli
    const QStringList seeds = {
        "3^done,bkpt={number=\"1\",addr=\"0x10\"}",
        "*stopped,reason=\"breakpoint-hit\",frame={addr=\"0x20\",func=\"main\"}",
        "~\"ok\\n\"",
        "4^error,msg=\"yok\"",
        "=thread-created,id=\"1\"",
    };
    QRandomGenerator rng(12345);
    int parsed = 0;
    for (int i = 0; i < 400; ++i) {
        QString s = seeds.at(rng.bounded(seeds.size()));
        // Rastgele mutasyon: sil/ekle/değiştir
        for (int k = 0, n = 1 + rng.bounded(3); k < n; ++k) {
            if (s.isEmpty()) break;
            const int pos = rng.bounded(s.size());
            switch (rng.bounded(3)) {
            case 0: s.remove(pos, 1 + rng.bounded(3)); break;
            case 1: s.insert(pos, QChar('a' + rng.bounded(26))); break;
            default: s[pos] = QChar(33 + rng.bounded(90)); break;
            }
        }
        const MiRecord r = MiParser::parseLine(s); // çökmemeli
        Q_UNUSED(r);
        ++parsed;
    }
    CHECK(parsed == 400);
    // Geçerli satırlar hâlâ doğru
    MiRecord r = MiParser::parseLine("3^done,bkpt={number=\"1\"}");
    CHECK(r.kind == "result" && r.token == "3");
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    testCheckedIO();
    testBackoff();
    testRotate();
    testQuota();
    testMiFuzz();
    fprintf(stderr, "STAGE31: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
