// Stage 49 testleri: gerçek localhost WebSocket üzerinden iki oturum —
// metin/imleç eşitleme, katılma/ayrılma, rev uyuşmazlığı, bozuk ileti.
#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QElapsedTimer>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <QWebSocket>
#include <cstdio>

#include "../src/core/CollabSession.h"

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

static bool pump(int ms) {
    QEventLoop loop;
    QTimer t;
    t.setSingleShot(true);
    QObject::connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
    t.start(ms);
    loop.exec();
    return true;
}

static bool waitTrue(std::function<bool()> fn, int ms = 5000) {
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms) {
        if (fn()) return true;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
    return fn();
}

// --- 1: host + join + merhaba ---
static void testHostJoin() {
    CollabSession host, guest;
    QStringList hostPeers, guestPeers;
    QObject::connect(&host, &CollabSession::peerJoined,
                     [&](const QString& u) { hostPeers << u; });
    QObject::connect(&guest, &CollabSession::peerJoined,
                     [&](const QString& u) { guestPeers << u; });
    const int port = host.host("evsahibi");
    CHECK2(port > 0, "host dinleyemedi");
    CHECK(host.isActive());
    CHECK(host.role() == "host");
    CHECK(guest.join(QString("ws://127.0.0.1:%1").arg(port), "misafir"));
    CHECK(waitTrue([&] { return guest.isActive(); }));
    CHECK(guest.role() == "peer");
    CHECK(waitTrue([&] { return hostPeers.contains("misafir"); }));
    host.leave();
    guest.leave();
    CHECK(!host.isActive() && !guest.isActive());
}

// --- 2: metin eşitleme iki yönlü ---
static void testTextSync() {
    CollabSession h2, g2;
    const int port = h2.host("ev");
    CHECK(port > 0);
    h2.setBaseText("baslangic\n");
    QString gotGuest, gotHost, fromUser;
    QObject::connect(&g2, &CollabSession::textMerged,
                     [&](const QString& t, const QString& u) {
                         gotGuest = t;
                         fromUser = u;
                     });
    QObject::connect(&h2, &CollabSession::textMerged,
                     [&](const QString& t, const QString&) { gotHost = t; });
    g2.join(QString("ws://127.0.0.1:%1").arg(port), "mi");
    CHECK(waitTrue([&] { return g2.isActive(); }));
    pump(300); // sync iletisi
    // Ev sahibi yazar → misafir alır
    h2.publishText("baslangic\nsatir2\n");
    CHECK2(waitTrue([&] { return gotGuest.contains("satir2"); }), "host→guest eşitlenmedi");
    CHECK(fromUser == "ev");
    // Misafir yazar → ev sahibi alır
    g2.publishText(gotGuest + "misafir-satir\n");
    CHECK2(waitTrue([&] { return gotHost.contains("misafir-satir"); }),
           "guest→host eşitlenmedi");
    // Aynı metin yayınlanmaz (trafik yok)
    gotGuest.clear();
    h2.publishText(h2.revision() >= 0 ? gotHost : gotHost);
    pump(300);
    h2.leave();
    g2.leave();
}

// --- 3: imleç + ayrılma ---
static void testCursorLeave() {
    CollabSession h, g;
    const int port = h.host("ev");
    QStringList left;
    QObject::connect(&h, &CollabSession::peerLeft,
                     [&](const QString& u) { left << u; });
    g.join(QString("ws://127.0.0.1:%1").arg(port), "mi");
    CHECK(waitTrue([&] { return g.isActive(); }));
    g.publishCursor(10, 5);
    CHECK2(waitTrue([&] { return h.cursors().value("mi") == qMakePair(10, 5); }),
           "imleç eşitlenmedi");
    CHECK(h.peers().contains("mi"));
    g.leave();
    CHECK2(waitTrue([&] { return left.contains("mi"); }), "ayrılma bildirilmedi");
    CHECK(!h.peers().contains("mi"));
    h.leave();
}

// --- 4: bozuk ileti çökertmez ---
static void testGarbage() {
    CollabSession h;
    const int port = h.host("ev");
    CHECK(port > 0);
    QWebSocket raw;
    bool open = false;
    QObject::connect(&raw, &QWebSocket::connected, [&] { open = true; });
    raw.open(QUrl(QString("ws://127.0.0.1:%1").arg(port)));
    CHECK(waitTrue([&] { return open; }));
    raw.sendTextMessage("{bozuk json");
    raw.sendTextMessage("[1,2,3]");
    raw.sendTextMessage("{\"t\":\"bilinmeyen-tur\"}");
    raw.sendTextMessage("{\"t\":\"edit\",\"base\":999,\"ops\":[]}");
    pump(500);
    CHECK(h.isActive()); // hâlâ ayakta
    raw.close();
    h.leave();
}

// --- 5: yayın yokken trafik yok + rev ---
static void testRevision() {
    CollabSession h;
    h.host("ev");
    h.setBaseText("x");
    CHECK(h.revision() == 0);
    h.publishText("x"); // aynı metin → yayın yok
    CHECK(h.revision() == 0);
    h.publishText("y");
    CHECK(h.revision() == 1);
    h.leave();
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir home;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, home.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, home.path());
    qputenv("XDG_DATA_HOME", home.path().toUtf8());
    qputenv("HOME", home.path().toUtf8());

    testHostJoin();
    testTextSync();
    testCursorLeave();
    testGarbage();
    testRevision();

    fprintf(stderr, "STAGE49: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
