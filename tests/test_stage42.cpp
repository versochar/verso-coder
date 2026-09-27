// Stage 42 testleri: LSP istek zaman aşımı, sahte sunucuya karşı protokol,
// URI dönüşümleri, yetenek sorgusu ve (clangd varsa) canlı hover.
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QJsonObject>
#include <QSettings>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QTimer>
#include <cstdio>

#include "../src/core/LspClient.h"

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

// Sahte LSP sunucusu: initialize + hover yanıtlar, "asla/yanıt" yutar,
// bozuk başlıklı iletiyi yoksayar (istemci çökmemeli).
static const char* kFakeServer = R"PY(
import sys, json

def read_msg():
    headers = {}
    while True:
        line = sys.stdin.readline()
        if not line:
            return None
        line = line.strip()
        if not line:
            break
        if ':' in line:
            k, v = line.split(':', 1)
            headers[k.strip().lower()] = v.strip()
    n = int(headers.get('content-length', 0))
    if n <= 0:
        return 'BOZUK'
    return sys.stdin.read(n)

def send(obj):
    body = json.dumps(obj).encode()
    sys.stdout.write('Content-Length: %d\r\n\r\n' % len(body))
    sys.stdout.write(body.decode())
    sys.stdout.flush()

while True:
    raw = read_msg()
    if raw is None:
        break
    if raw == 'BOZUK':
        continue
    try:
        msg = json.loads(raw)
    except Exception:
        continue
    mid = msg.get('id')
    method = msg.get('method', '')
    if method == 'initialize':
        send({"jsonrpc": "2.0", "id": mid, "result": {
            "capabilities": {"hoverProvider": True, "renameProvider": False}}})
    elif method == 'initialized':
        pass
    elif method == 'textDocument/hover':
        send({"jsonrpc": "2.0", "id": mid, "result": {
            "contents": {"kind": "markdown", "value": "SAHTE-HOVER"}}})
    elif method == 'shutdown':
        send({"jsonrpc": "2.0", "id": mid, "result": None})
        break
    # 'asla/yanit' ve diğerleri: bilerek yanıtsız (zaman aşımı testi)
)PY";

static QString writeFakeServer(const QString& dir) {
    QFile f(QDir(dir).filePath("fake_lsp.py"));
    CHECK(f.open(QIODevice::WriteOnly));
    f.write(kFakeServer);
    f.close();
    return f.fileName();
}

static bool waitFor(QEventLoop& loop, int ms) {
    QTimer t;
    t.setSingleShot(true);
    QObject::connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
    t.start(ms);
    loop.exec();
    return t.isActive(); // süre dolmadan çıktıysa true
}

// --- 1: URI dönüşümleri + yetenek ---
static void testUriAndCaps() {
    CHECK(LspClient::pathToUri("/a/b/c.cpp") == "file:///a/b/c.cpp");
    CHECK(LspClient::uriToPath("file:///a/b/c.cpp") == "/a/b/c.cpp");
    CHECK(LspClient::uriToPath("file:///a/b%20c.cpp") == "/a/b c.cpp");
    CHECK(LspClient::pathToUri("").isEmpty()); // boş yol tanımsız
    LspClient c;
    CHECK(c.hasCap("hoverProvider")); // yetenek bilinmiyorsa iyimser
    CHECK(c.pendingCount() == 0);
    CHECK(c.requestTimeoutMs() == 30000);
    c.setRequestTimeoutMs(500);
    CHECK(c.requestTimeoutMs() == 500);
    c.setRequestTimeoutMs(-5);
    CHECK(c.requestTimeoutMs() == 0);
}

// --- 2: sahte sunucu — initialize + hover + bildirim ---
static void testFakeServer() {
    QTemporaryDir d;
    const QString srv = writeFakeServer(d.path());
    LspClient c;
    CHECK(c.start("python3", {srv}, d.path()));
    QEventLoop loop;
    bool ready = false;
    // Hazır olana kadar bekle (initialize el sıkışması)
    QElapsedTimer t;
    t.start();
    while (!c.isReady() && t.elapsed() < 8000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
    ready = c.isReady();
    CHECK2(ready, "sahte sunucu hazır olmadı");
    if (!ready) {
        c.stop();
        return;
    }
    CHECK(!c.serverCaps().isEmpty());
    CHECK(c.hasCap("hoverProvider"));
    CHECK(!c.hasCap("renameProvider"));
    // Hover
    QJsonObject hover;
    bool gotHover = false;
    c.requestHover(d.path() + "/a.cpp", 0, 0, [&](QJsonObject r) {
        hover = r;
        gotHover = true;
        loop.quit();
    });
    CHECK(waitFor(loop, 5000));
    CHECK2(gotHover, "hover yanıtı gelmedi");
    CHECK(hover["contents"].toObject()["value"].toString() == "SAHTE-HOVER");
    CHECK(c.pendingCount() == 0);
    c.stop();
}

// --- 3: zaman aşımı — yanıtsız istek düşer, sinyal gelir ---
static void testTimeout() {
    QTemporaryDir d;
    const QString srv = writeFakeServer(d.path());
    LspClient c;
    c.setRequestTimeoutMs(400);
    CHECK(c.start("python3", {srv}, d.path()));
    QElapsedTimer t;
    t.start();
    while (!c.isReady() && t.elapsed() < 8000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    CHECK(c.isReady());
    QString timedOutMethod;
    int timedOutId = -1;
    QObject::connect(&c, &LspClient::requestTimedOut,
                     [&](const QString& m, int id) {
                         timedOutMethod = m;
                         timedOutId = id;
                     });
    bool handlerCalled = false;
    const int id = c.request("asla/yanit", QJsonObject{}, [&](QJsonObject) {
        handlerCalled = true;
    });
    CHECK(id > 0);
    CHECK(c.pendingCount() == 1);
    QEventLoop loop;
    QTimer::singleShot(1500, &loop, &QEventLoop::quit);
    loop.exec();
    CHECK2(timedOutMethod == "asla/yanit", "zaman aşımı sinyali gelmedi");
    CHECK(timedOutId == id);
    CHECK(!handlerCalled); // düşürülen handler çalışmamalı
    CHECK(c.pendingCount() == 0);
    // Sunucu hâlâ yaşıyor ve yeni istek alabiliyor (ölmedi)
    CHECK(c.isRunning());
    c.stop();
    CHECK(c.pendingCount() == 0);
}

// --- 4: çöken sunucu — stop bekleyenleri temizler ---
static void testCrashCleanup() {
    QTemporaryDir d;
    const QString srv = writeFakeServer(d.path());
    LspClient c;
    c.setRequestTimeoutMs(60000); // uzun: stop temizlemeli, zaman aşımı değil
    CHECK(c.start("python3", {srv}, d.path()));
    QElapsedTimer t;
    t.start();
    while (!c.isReady() && t.elapsed() < 8000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    CHECK(c.isReady());
    bool called = false;
    c.request("asla/yanit", QJsonObject{}, [&](QJsonObject) { called = true; });
    c.request("asla/yanit2", QJsonObject{}, [&](QJsonObject) { called = true; });
    CHECK(c.pendingCount() == 2);
    c.stop();
    CHECK(c.pendingCount() == 0);
    CHECK(!called);
}

// --- 5: canlı clangd hover (varsa; yoksa atlanır) ---
static void testLiveClangd() {
    if (QStandardPaths::findExecutable("clangd").isEmpty()) {
        fprintf(stderr, "NOT: clangd yok, canlı test atlandı\n");
        ++g_pass;
        return;
    }
    QTemporaryDir d;
    QFile f(QDir(d.path()).filePath("a.cpp"));
    CHECK(f.open(QIODevice::WriteOnly));
    f.write("int toplanan = 41;\nint main() { return toplanan; }\n");
    f.close();
    LspClient c;
    c.setRequestTimeoutMs(25000);
    CHECK(c.start("clangd", {"--offset-encoding=utf-8"}, d.path()));
    QElapsedTimer t;
    t.start();
    while (!c.isReady() && t.elapsed() < 20000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    if (!c.isReady()) {
        fprintf(stderr, "NOT: clangd hazır olmadı (20 sn), atlandı\n");
        ++g_pass;
        c.stop();
        return;
    }
    c.didOpen(f.fileName(), "cpp", "int toplanan = 41;\nint main() { return toplanan; }\n");
    QEventLoop loop;
    QString value;
    bool got = false;
    // "toplanan" 1. satır 14. sütun civarı
    c.requestHover(f.fileName(), 1, 20, [&](QJsonObject r) {
        got = true;
        value = r["contents"].toObject()["value"].toString();
        if (value.isEmpty()) value = QString::fromUtf8(
            QJsonDocument(r).toJson(QJsonDocument::Compact));
        loop.quit();
    });
    waitFor(loop, 20000);
    CHECK2(got, "clangd hover yanıtı gelmedi");
    if (got) CHECK2(value.contains("toplanan") || value.contains("int"),
                    "hover içeriği beklenmedik");
    c.stop();
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir home;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, home.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, home.path());
    qputenv("XDG_DATA_HOME", home.path().toUtf8());
    qputenv("HOME", home.path().toUtf8());

    testUriAndCaps();
    testFakeServer();
    testTimeout();
    testCrashCleanup();
    testLiveClangd();

    fprintf(stderr, "STAGE42: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
