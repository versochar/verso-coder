// CollabCrypt testleri: ChaCha20 bilinen-yanıt (node+openssl oraclesi),
// gidip-gelme, yanlış anahtar/oynama reddi, anahtar türetme.
#include <QCoreApplication>
#include <QRandomGenerator>
#include <cstdio>
#include <functional>

#include "../src/core/CollabCrypt.h"

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

static QByteArray hex(const char* s) { return QByteArray::fromHex(s); }

static void testKat() {
    // RFC 8439 parametreleri; şifrelimetin node+openssl ortak oraclesinden
    const QByteArray key = hex("000102030405060708090a0b0c0d0e0f"
                               "101112131415161718191a1b1c1d1e1f");
    const QByteArray nonce = hex("000000090000004a00000000");
    const QByteArray pt =
        "Ladies and Gentleman of the class of 99: If I could offer you "
        "only one tip for the future, sunscreen would be it.";
    CHECK(pt.size() == 113);
    const QByteArray beklenen = hex(
        "5c90838db44879743e6bfd58c64e05a8a2bc95a913af0e23704acfbaa0b80d3da1a"
        "20b2027a69333348bb12590ab61cdc07ef8f1b17028dcb9f0fa87d770532066f1a3"
        "1857b29f3a91bc8cd6855999a2f533e3f2df41688fc09365ec7f40ec8b95449dfc75"
        "8eabf3d34547fe0d1d26e872");
    const QByteArray ct = CollabCrypt::chacha20(key, nonce, 1, pt);
    CHECK(ct == beklenen);
    // Ters yön de aynı akış (XOR simetrisi)
    CHECK(CollabCrypt::chacha20(key, nonce, 1, ct) == pt);
    // Sayaç 0 farklı akış verir
    CHECK(CollabCrypt::chacha20(key, nonce, 0, pt) != beklenen);
    // Boy/sınır
    CHECK(CollabCrypt::chacha20(key, nonce, 1, {}).isEmpty());
    CHECK(CollabCrypt::chacha20(QByteArray(16, 0), nonce, 1, pt).isEmpty());
    CHECK(CollabCrypt::chacha20(key, QByteArray(4, 0), 1, pt).isEmpty());
}

static void testSeal() {
    const QByteArray key = CollabCrypt::deriveKey("gizli");
    CHECK(key.size() == 32);
    CHECK(CollabCrypt::deriveKey("gizli") == key); // deterministik
    CHECK(CollabCrypt::deriveKey("farklı") != key);
    const QString icerik = QString::fromUtf8("Türkçe メッセージ 🎉");
    QString err;
    const QString zarf = CollabCrypt::seal(key, icerik, &err);
    CHECK(err.isEmpty());
    CHECK(CollabCrypt::looksSealed(zarf));
    CHECK(!zarf.contains("Türkçe")); // düz metin görünmez
    CHECK(CollabCrypt::open(key, zarf, &err) == icerik);
    CHECK(err.isEmpty());
    // Aynı içerik farklı zarf (rastgele IV)
    CHECK(CollabCrypt::seal(key, icerik) != zarf);
    // Uzun metin (çok bloklu)
    QString uzun;
    for (int i = 0; i < 200; ++i) uzun += "satır " + QString::number(i) + " içerik\n";
    CHECK(CollabCrypt::open(key, CollabCrypt::seal(key, uzun)) == uzun);
    // Yanlış anahtar
    const QByteArray kotu = CollabCrypt::deriveKey("yanlış");
    QString err2;
    CHECK(CollabCrypt::open(kotu, zarf, &err2).isEmpty());
    CHECK(!err2.isEmpty());
    // Oynanmış şifrelimetin
    QString oyna = zarf;
    oyna[oyna.size() - 8] = (oyna[oyna.size() - 8] == 'A' ? 'B' : 'A');
    CHECK(CollabCrypt::open(key, oyna).isEmpty());
    // Bozuk zarf
    CHECK(CollabCrypt::open(key, "v1.bozuk").isEmpty());
    CHECK(CollabCrypt::open(key, "{\"t\":\"hello\"}").isEmpty());
    CHECK(!CollabCrypt::looksSealed("{\"t\":\"hello\"}"));
}

// --- oturum birlikte-çalışması (localhost soketleri) ---
#include "../src/core/CollabSession.h"
#include <QEventLoop>
#include <QTimer>

static bool bekle(const std::function<bool()>& kosul, int ms) {
    QEventLoop loop;
    QTimer guard;
    guard.setSingleShot(true);
    QObject::connect(&guard, &QTimer::timeout, &loop, &QEventLoop::quit);
    QTimer yokla;
    QObject::connect(&yokla, &QTimer::timeout, &loop, [&]() {
        if (kosul()) loop.quit();
    });
    yokla.start(50);
    guard.start(ms);
    loop.exec();
    return kosul();
}

static void testOturum() {
    // 1) Şifresiz uyumluluk (eski davranış)
    CollabSession hostA, peerB;
    const int p1 = hostA.host("a", 0);
    CHECK(p1 > 0);
    CHECK(!hostA.hasKey());
    CHECK(peerB.join(QString("ws://127.0.0.1:%1").arg(p1), "b"));
    CHECK(bekle([&]() { return hostA.isActive() && peerB.isActive(); }, 5000));
    QString aldi;
    QObject::connect(&peerB, &CollabSession::textMerged, &peerB,
                     [&](const QString& t, const QString&) { aldi = t; });
    hostA.publishText("selam");
    CHECK(bekle([&]() { return aldi == "selam"; }, 5000));
    hostA.leave();
    peerB.leave();

    // 2) Anahtarlı oturum uçar
    CollabSession hostK, peerK;
    const int p2 = hostK.host("k", 0);
    CHECK(p2 > 0);
    hostK.setKey("sifre");
    CHECK(hostK.hasKey());
    CHECK(peerK.join(QString("ws://127.0.0.1:%1#sifre").arg(p2), "p"));
    CHECK(bekle([&]() { return hostK.isActive() && peerK.isActive(); }, 5000));
    QString gizli;
    QObject::connect(&peerK, &CollabSession::textMerged, &peerK,
                     [&](const QString& t, const QString&) { gizli = t; });
    hostK.publishText("gizli-metin");
    CHECK(bekle([&]() { return gizli == "gizli-metin"; }, 5000));

    // 3) Yanlış anahtar düşer (bye + hata)
    CollabSession peerW;
    QString hataW;
    QObject::connect(&peerW, &CollabSession::sessionError, &peerW,
                     [&](const QString& e) { hataW = e; });
    bool sizdi = false;
    QObject::connect(&hostK, &CollabSession::textMerged, &hostK,
                     [&](const QString&, const QString& kim) {
                         if (kim == "w") sizdi = true;
                     });
    CHECK(peerW.join(QString("ws://127.0.0.1:%1#yanlis").arg(p2), "w"));
    CHECK(bekle([&]() { return !hataW.isEmpty(); }, 5000));
    CHECK(hataW.contains("nahtar")); // "Anahtar..." ya da "...anahtarı..."
    // Anahtarsız eş de düşer
    CollabSession peerD;
    QString hataD;
    QObject::connect(&peerD, &CollabSession::sessionError, &peerD,
                     [&](const QString& e) { hataD = e; });
    CHECK(peerD.join(QString("ws://127.0.0.1:%1").arg(p2), "d"));
    CHECK(bekle([&]() { return !hataD.isEmpty(); }, 5000));
    CHECK(!sizdi);
    hostK.leave();
    peerK.leave();
    peerW.leave();
    peerD.leave();

    // 4) leave anahtarı düşürür
    CollabSession s;
    s.setKey("x");
    CHECK(s.hasKey());
    s.leave();
    CHECK(!s.hasKey());
    s.setKey("");
    CHECK(!s.hasKey());
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    testKat();
    testSeal();
    testOturum();
    fprintf(stderr, "COLLABCRYPT: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
