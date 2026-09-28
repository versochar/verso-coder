// Stage 48 testleri: sürüm karşılaştırma, besleme ayrıştırma, denetim
// zamanlaması, taşınabilir kip ayrıştırma, çökme dökümü yardımcıları.
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QDateTime>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>
#include <cstdio>

#include "../src/core/CrashHandler.h"
#include "../src/core/StartupArgs.h"
#include "../src/core/UpdateChecker.h"
#include "../src/core/AppUpdater.h"

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

// --- 1: sürüm karşılaştırma ---
static void testCompare() {
    CHECK(UpdateChecker::compareVersions("2.9.0", "3.0.0") < 0);
    CHECK(UpdateChecker::compareVersions("3.0.0", "2.9.0") > 0);
    CHECK(UpdateChecker::compareVersions("2.9.0", "2.9.0") == 0);
    CHECK(UpdateChecker::compareVersions("2.9", "2.9.0") == 0);
    CHECK(UpdateChecker::compareVersions("v3.0.1", "3.0.0") > 0);
    CHECK(UpdateChecker::compareVersions("2.10.0", "2.9.9") > 0); // sayısal, sözlüksel değil
    CHECK(UpdateChecker::compareVersions("", "1.0.0") < 0);
}

// --- 2: besleme ayrıştırma ---
static void testParseFeed() {
    const QByteArray good = R"({"tag_name": "v3.0.1", "html_url": "https://x/y"})";
    UpdateChecker::Result r = UpdateChecker::parseFeed(good, "2.9.0");
    CHECK(r.checked && r.newer);
    CHECK(r.latest == "v3.0.1");
    CHECK(r.url == "https://x/y");
    UpdateChecker::Result same = UpdateChecker::parseFeed(good, "3.0.1");
    CHECK(same.checked && !same.newer);
    UpdateChecker::Result older = UpdateChecker::parseFeed(good, "4.0.0");
    CHECK(!older.newer);
    UpdateChecker::Result bad = UpdateChecker::parseFeed("{bozuk", "2.9.0");
    CHECK(bad.checked && !bad.newer && !bad.error.isEmpty());
    UpdateChecker::Result empty = UpdateChecker::parseFeed("{}", "2.9.0");
    CHECK(!empty.error.isEmpty());
    // AppImage varlığı + yokluğu
    const QByteArray withAsset =
        R"({"tag_name":"v1.4.0","html_url":"https://x/y","assets":[
             {"name":"VersoCoder-v1.4.0-x86_64.AppImage","size":37964280,
              "browser_download_url":"https://dl/x.AppImage"},
             {"name":"VersoCoder-v1.4.0-windows-x86_64.zip","size":1,
              "browser_download_url":"https://dl/x.zip"}]})";
    const UpdateChecker::Result wa = UpdateChecker::parseFeed(withAsset, "1.3.1");
    CHECK(wa.checked && wa.newer);
    CHECK(wa.appImageUrl == "https://dl/x.AppImage");
    CHECK(wa.appImageSize == 37964280);
    const UpdateChecker::Result na = UpdateChecker::parseFeed(good, "2.9.0");
    CHECK(na.appImageUrl.isEmpty()); // varlıksız besleme
    CHECK(na.appImageSize == -1);
    // Canlı dosya üzerinden (file:// yoklama yolu)
    QTemporaryDir d;
    QFile f(d.path() + "/feed.json");
    CHECK(f.open(QIODevice::WriteOnly));
    f.write(good);
    f.close();
    UpdateChecker uc;
    uc.setFeedUrl(QUrl::fromLocalFile(f.fileName()).toString());
    uc.setCurrentVersion("2.9.0");
    uc.setTimeoutMs(5000);
    const UpdateChecker::Result live = uc.checkSync();
    CHECK2(live.checked && live.newer, "file:// yoklama çalışmadı");
}

// --- 3: denetim zamanlaması (her açılış + ayar) ---
static void testSchedule() {
    QSettings st("Verso", "VersoCoder");
    st.remove("update/check");
    st.remove("update/lastCheck");
    CHECK(UpdateChecker::shouldCheck()); // varsayılan açık
    UpdateChecker::markChecked();
    CHECK(UpdateChecker::shouldCheck()); // damga engellemez (her açılış)
    // Kapalıysa hiç
    st.setValue("update/check", false);
    CHECK(!UpdateChecker::shouldCheck());
    st.setValue("update/check", true);
    CHECK(UpdateChecker::shouldCheck());
    st.remove("update/check");
    st.remove("update/lastCheck");
}

// --- 4: taşınabilir kip ayrıştırma ---
static void testPortable() {
    StartupOptions o = StartupArgs::parse({"--portable"});
    CHECK(o.portable);
    StartupOptions o2 = StartupArgs::parse({"dosya.txt", "--new"});
    CHECK(!o2.portable && o2.isNew);
    CHECK(o2.paths.contains("dosya.txt"));
    CHECK(StartupArgs::helpText().contains("--portable"));
}

// --- 5: çökme dökümü yardımcıları ---
static void testCrashDumps() {
    CHECK(!CrashHandler::dumpName().isEmpty());
    CHECK(CrashHandler::dumpName().startsWith("crash-"));
    CHECK(CrashHandler::dumpName().endsWith(".log"));
    CHECK(CrashHandler::pendingDumps().isEmpty());
    CHECK(CrashHandler::writeDump(CrashHandler::crashDir() + "/crash-test-1.log",
                                  "test nedeni"));
    CHECK(CrashHandler::pendingDumps().contains("crash-test-1.log"));
    CrashHandler::clearDumps();
    CHECK(CrashHandler::pendingDumps().isEmpty());
}

// --- 3b: indirici (file:// üzerinden) ---
static void testUpdater() {
    QTemporaryDir src;
    QTemporaryDir dst;
    CHECK(src.isValid() && dst.isValid());
    QFile f(src.filePath("sürüm.bin"));
    CHECK(f.open(QIODevice::WriteOnly));
    const QByteArray icerik("0123456789ABCDEF");
    CHECK(f.write(icerik) == icerik.size());
    f.close();
    AppUpdater up;
    up.setSourceUrl(QUrl::fromLocalFile(f.fileName()).toString());
    up.setDestDir(dst.path());
    up.setFileName("indirilen.bin");
    QString yol, hata;
    QEventLoop bekle;
    QTimer guard;
    guard.setSingleShot(true);
    QObject::connect(&guard, &QTimer::timeout, &bekle, &QEventLoop::quit);
    QObject::connect(&up, &AppUpdater::finished, &bekle,
                     [&](const QString& p) { yol = p; bekle.quit(); });
    QObject::connect(&up, &AppUpdater::failed, &bekle,
                     [&](const QString& e) { hata = e; bekle.quit(); });
    up.start();
    guard.start(10000);
    bekle.exec();
    CHECK2(hata.isEmpty(), qUtf8Printable(hata));
    CHECK(yol == dst.filePath("indirilen.bin"));
    QFile g(yol);
    CHECK(g.open(QIODevice::ReadOnly));
    CHECK(g.readAll() == icerik);
    g.close();
    // Olmayan kaynak: hata + yarım dosya yok
    AppUpdater up2;
    up2.setSourceUrl(QUrl::fromLocalFile(src.filePath("yok.bin")).toString());
    up2.setDestDir(dst.path());
    up2.setFileName("olmamali.bin");
    QString hata2;
    QEventLoop bekle2;
    QObject::connect(&up2, &AppUpdater::finished, &bekle2, &QEventLoop::quit);
    QObject::connect(&up2, &AppUpdater::failed, &bekle2,
                     [&](const QString& e) { hata2 = e; bekle2.quit(); });
    up2.start();
    QTimer::singleShot(10000, &bekle2, &QEventLoop::quit);
    bekle2.exec();
    CHECK(!hata2.isEmpty());
    CHECK(!QFile::exists(dst.filePath("olmamali.bin")));
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir home;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, home.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, home.path());
    qputenv("XDG_DATA_HOME", home.path().toUtf8());
    qputenv("XDG_CONFIG_HOME", home.path().toUtf8());
    qputenv("HOME", home.path().toUtf8());

    testCompare();
    testParseFeed();
    testSchedule();
    testUpdater();
    testPortable();
    testCrashDumps();

    fprintf(stderr, "STAGE48: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
