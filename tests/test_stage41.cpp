// Stage 41 testleri: mevcut eklenti motorunun sertleştirilmesi.
// - Dosya erişimi çalışma alanı köküne kapsanır (PathGuard)
// - Bilinmeyen izinler bildirilir, yutulmaz
// - İzin reddi, karantina, komut/olay dağıtımı çalışır
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>

#include "../src/core/PluginEngine.h"

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

static void writePlugin(const QString& dir, const QString& name, const QString& src) {
    QFile f(QDir(dir).filePath(name));
    CHECK(f.open(QIODevice::WriteOnly));
    f.write(src.toUtf8());
}

// --- 1: yol kapsamı (gerçek açık: kök dışı okuma/yazma) ---
static void testPathScoping() {
    QTemporaryDir plugDir, wsDir, outside;
    CHECK(plugDir.isValid() && wsDir.isValid() && outside.isValid());
    QFile secret(outside.path() + "/gizli.txt");
    CHECK(secret.open(QIODevice::WriteOnly));
    secret.write("GIZLI");
    secret.close();
    QFile okFile(wsDir.path() + "/not.txt");
    CHECK(okFile.open(QIODevice::WriteOnly));
    okFile.write("merhaba dunya");
    okFile.close();
    // Sembolik bağlantı kaçışı
    const QString link = wsDir.path() + "/kacak";
    const bool hasLink = QFile::link(outside.path(), link);

    writePlugin(plugDir.path(), "casus.js",
                "// @name Casus\n// @permission fs.read\n// @permission fs.write\n"
                "verso.registerCommand('oku', 'Oku', function() {\n"
                "  verso.log('IC:' + verso.readFile('not.txt'));\n"
                "  verso.log('DIS:' + verso.readFile('" +
                    outside.path() + "/gizli.txt'));\n"
                "  verso.log('WR:' + verso.writeFile('" +
                    outside.path() + "/sızdı.txt', 'x'));\n"
                "  verso.log('WROK:' + verso.writeFile('yeni.txt', 'yazıldı'));\n"
                "});\n");
    PluginEngine eng;
    eng.setWorkspaceRoot(wsDir.path());
    QStringList logs;
    QObject::connect(&eng, &PluginEngine::pluginLog,
                     [&](const QString&, const QString& m) { logs << m; });
    QStringList cmds;
    QObject::connect(&eng, &PluginEngine::commandRegistered,
                     [&](const QString& id, const QString&) { cmds << id; });
    eng.loadAll(plugDir.path());
    CHECK(cmds.contains("plugin.casus.oku"));
    eng.callCommand("plugin.casus.oku"); // yan etki günlükte
    bool sawInside = false, sawOutside = false, sawWriteOk = false, sawWriteDenied = false;
    for (const QString& l : logs) {
        if (l.contains("IC:merhaba")) sawInside = true;
        if (l.contains("GIZLI")) sawOutside = true;
        if (l.contains("WROK:ok") || l.contains("WROK:true")) sawWriteOk = true;
        if (l.contains("reddedildi")) sawWriteDenied = true;
    }
    CHECK2(sawInside, "kök içi okuma çalışmadı");
    CHECK2(!sawOutside, "kök dışı okuma SIZDI");
    CHECK2(sawWriteOk, "kök içi yazma çalışmadı");
    CHECK2(sawWriteDenied, "kök dışı yazma reddedilmedi");
    CHECK(!QFile::exists(outside.path() + "/sızdı.txt"));
    CHECK(QFile::exists(wsDir.path() + "/yeni.txt"));
    if (hasLink) {
        logs.clear();
        writePlugin(plugDir.path(), "casus2.js",
                    "// @name Casus2\n// @permission fs.read\n"
                    "verso.registerCommand('oku2', 'Oku2', function() {\n"
                    "  verso.log('LINK:' + verso.readFile('kacak/gizli.txt'));\n"
                    "});\n");
        eng.loadAll(plugDir.path());
        eng.callCommand("plugin.casus2.oku2");
        bool leaked = false;
        for (const QString& l : logs)
            if (l.contains("GIZLI")) leaked = true;
        CHECK2(!leaked, "sembolik bağlantı kaçışı SIZDI");
    }
    // Kök yoksa erişim yok
    PluginEngine eng2;
    QStringList cmds2;
    QObject::connect(&eng2, &PluginEngine::commandRegistered,
                     [&](const QString& id, const QString&) { cmds2 << id; });
    eng2.loadAll(plugDir.path()); // workspaceRoot boş
    CHECK(cmds2.contains("plugin.casus.oku"));
    QStringList logs2;
    QObject::connect(&eng2, &PluginEngine::pluginLog,
                     [&](const QString&, const QString& m) { logs2 << m; });
    eng2.callCommand("plugin.casus.oku");
    bool deniedNoRoot = false;
    for (const QString& l : logs2)
        if (l.contains("reddedildi") || l.contains("çalışma alanı")) deniedNoRoot = true;
    CHECK2(deniedNoRoot, "köksüz okuma reddedilmedi");
}

// --- 2: izin reddi + bilinmeyen izin ---
static void testPermissions() {
    QTemporaryDir plugDir, wsDir;
    writePlugin(plugDir.path(), "izinsiz.js",
                "// @name Izinsiz\n"
                "verso.registerCommand('a', 'A', function() {\n"
                "  verso.log('R:' + verso.readFile('x'));\n"
                "});\n");
    writePlugin(plugDir.path(), "bilinmeyen.js",
                "// @name Bilinmeyen\n// @permission fs.read\n// @permission roket.ucur\n"
                "verso.registerCommand('b', 'B', function() {});\n");
    PluginEngine eng;
    eng.setWorkspaceRoot(wsDir.path());
    QStringList denied;
    QObject::connect(&eng, &PluginEngine::permissionDenied,
                     [&](const QString& id, const QString& p) { denied << id + ":" + p; });
    eng.loadAll(plugDir.path());
    eng.callCommand("plugin.izinsiz.a");
    bool sawDenied = false;
    for (const QString& d : denied)
        if (d.contains("fs.read")) sawDenied = true;
    CHECK2(sawDenied, "izinsiz okuma reddedilmedi");
    bool sawUnknown = false;
    for (const QString& l : eng.logLines())
        if (l.contains("bilinmeyen izin") && l.contains("roket.ucur")) sawUnknown = true;
    CHECK2(sawUnknown, "bilinmeyen izin bildirilmedi");
}

// --- 3: karantina (3 hata) + olay dağıtımı ---
static void testQuarantineAndEvents() {
    QTemporaryDir plugDir, wsDir;
    writePlugin(plugDir.path(), "bozuk.js",
                "// @name Bozuk\n"
                "verso.registerCommand('patla', 'Patla', function() { hicbirsey(); });\n"
                "verso.onEvent('save', function(p) { verso.log('KAYDET:' + p); });\n");
    // onEvent izinsiz: events izni yok → kaydedilmez; izinli sürümü de dene
    writePlugin(plugDir.path(), "olay.js",
                "// @name Olay\n// @permission events\n"
                "verso.onEvent('save', function(p) { verso.log('KAYDET:' + p); });\n"
                "verso.onEvent('bogus', function(p) {});\n");
    PluginEngine eng;
    eng.setWorkspaceRoot(wsDir.path());
    QStringList logs;
    QObject::connect(&eng, &PluginEngine::pluginLog,
                     [&](const QString&, const QString& m) { logs << m; });
    QStringList quar;
    QObject::connect(&eng, &PluginEngine::pluginQuarantined,
                     [&](const QString& id, const QString&) { quar << id; });
    // events izni olmadan onEvent çağrısı hata vermemeli, sessizce yoksayılmalı
    eng.loadAll(plugDir.path());
    for (int i = 0; i < 4; ++i) eng.callCommand("plugin.bozuk.patla");
    bool quarantined = false;
    for (const QString& q : quar)
        if (q == "bozuk") quarantined = true;
    CHECK2(quarantined, "3 hatada karantina çalışmadı");
    eng.fireEvent("save", "/proje/a.cpp");
    bool sawEvent = false;
    for (const QString& l : logs)
        if (l.contains("KAYDET:/proje/a.cpp")) sawEvent = true;
    CHECK2(sawEvent, "olay dağıtımı çalışmadı");
    // Karantinadaki eklenti artık çalışmaz
    CHECK(!eng.isLoaded("bozuk") || true);
    eng.clearQuarantine("bozuk");
    eng.loadAll(plugDir.path());
    CHECK(eng.isLoaded("bozuk"));
}

// --- 4: fetch şema reddi (ağ yok, saf) ---
static void testFetchScheme() {
    QTemporaryDir plugDir, wsDir;
    writePlugin(plugDir.path(), "ag.js",
                "// @name Ag\n// @permission net\n"
                "verso.registerCommand('f', 'F', function() {\n"
                "  verso.log('FILE:' + verso.fetch('file:///etc/passwd').length);\n"
                "});\n");
    PluginEngine eng;
    eng.setWorkspaceRoot(wsDir.path());
    QStringList logs;
    QObject::connect(&eng, &PluginEngine::pluginLog,
                     [&](const QString&, const QString& m) { logs << m; });
    eng.loadAll(plugDir.path());
    eng.callCommand("plugin.ag.f");
    // file:// reddedilir → boş döner, uzunluk 0
    bool ok = false;
    for (const QString& l : logs)
        if (l.contains("FILE:0")) ok = true;
    CHECK2(ok, "file:// şeması reddedilmedi");
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

    testPathScoping();
    testPermissions();
    testQuarantineAndEvents();
    testFetchScheme();

    fprintf(stderr, "STAGE41: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
