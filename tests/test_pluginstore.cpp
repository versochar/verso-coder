// Eklenti mağazası testleri: kayıt dizini çözümleme, index ayrıştırma,
// kimlik doğrulama, kurulum + sürüm okuma. Ağ yok: file:// sahte kayıt.
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>

#include "../src/core/AboutInfo.h"
#include "../src/core/PluginStore.h"

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

static void testRegistryBase() {
    qputenv("VERSO_PLUGIN_REGISTRY", "/tmp/verso-test-reg");
    CHECK(PluginStore::registryBase() == "/tmp/verso-test-reg");
    CHECK(PluginStore::indexUrl() == "/tmp/verso-test-reg/index.json");
    qunsetenv("VERSO_PLUGIN_REGISTRY");
    CHECK(PluginStore::registryBase() == PluginStore::defaultRegistry());
    CHECK(PluginStore::defaultRegistry().startsWith("https://"));
}

static void testParseIndex() {
    QString err;
    const QByteArray good = R"({"registry":1,"plugins":[
        {"id":"kelime","name":"Kelime Sayacı","version":"1.0.0",
         "description":"Açık dosyayı sayar","author":"versochar",
         "permissions":["fs.read","ui"],"file":"plugins/kelime/plugin.js",
         "minApp":"1.1.0"},
        {"id":"echo","version":"0.9","file":"plugins/echo/plugin.js"},
        {"id":"KÖTÜ!","version":"1","file":"x"},
        {"id":"","version":"1","file":"x"},
        {"id":"sürümsüz","file":"x"}
    ]})";
    const auto list = PluginStore::parseIndex(good, &err);
    CHECK(err.isEmpty());
    CHECK(list.size() == 2); // 3 geçersiz girdi atlandı
    CHECK(list[0].id == "kelime");
    CHECK(list[0].permissions == QStringList({"fs.read", "ui"}));
    CHECK(list[0].minApp == "1.1.0");
    CHECK(list[0].description == "Açık dosyayı sayar");
    CHECK(list[1].name == "echo"); // ad yoksa kimlik
    CHECK(list[1].permissions.isEmpty());
    CHECK(PluginStore::pluginUrl(list[0]).endsWith("plugins/kelime/plugin.js"));

    const auto empty = PluginStore::parseIndex("bozuk {", &err);
    CHECK(empty.isEmpty());
    CHECK(!err.isEmpty());
    const auto noarr = PluginStore::parseIndex(R"({"registry":1})", &err);
    CHECK(noarr.isEmpty());
    CHECK(!err.isEmpty());
}

static void testValidId() {
    CHECK(PluginStore::isValidId("kelime"));
    CHECK(PluginStore::isValidId("hizli-not2"));
    CHECK(!PluginStore::isValidId(""));
    CHECK(!PluginStore::isValidId("Büyük"));
    CHECK(!PluginStore::isValidId("boş luk"));
    CHECK(!PluginStore::isValidId("nokta.var"));
    CHECK(!PluginStore::isValidId("../kacis"));
}

static void testInstallAndVersion() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    QSettings("Verso", "VersoCoder").remove("plugin/enabled");
    // Önceden bir eklenti varmış gibi yap (tohum koruma denetimi)
    QFile old(dir.filePath("eski.js"));
    CHECK(old.open(QIODevice::WriteOnly));
    old.write("// @version 0.1\n");
    old.close();

    PluginStore::Entry e;
    e.id = "kelime";
    e.name = "Kelime Sayacı";
    e.version = "1.0.0";
    const QString src = "// @name Kelime Sayacı\n// @version 1.0.0\nverso.log(1);\n";
    QString err;
    CHECK(PluginStore::install(e, src, dir.path(), &err));
    CHECK(err.isEmpty());
    CHECK(QFile::exists(dir.filePath("kelime.js")));
    CHECK(PluginStore::installedVersion(dir.path(), "kelime") == "1.0.0");
    CHECK(PluginStore::installedVersion(dir.path(), "yok").isEmpty());
    // Liste yoktu: eski + yeni ikisi de etkin olmalı
    const QStringList en =
        QSettings("Verso", "VersoCoder").value("plugin/enabled").toStringList();
    CHECK(en.contains("kelime"));
    CHECK(en.contains("eski"));

    PluginStore::Entry bad;
    bad.id = "../kacis";
    CHECK(!PluginStore::install(bad, src, dir.path(), &err));
    CHECK(!err.isEmpty());
    CHECK(!QFile::exists(dir.filePath("kacis.js")));
    CHECK(!PluginStore::install(e, "   ", dir.path(), &err)); // boş kaynak
    QSettings("Verso", "VersoCoder").remove("plugin/enabled");
}

static void testFetchLocalRegistry() {
    QTemporaryDir reg;
    CHECK(reg.isValid());
    QDir(reg.path()).mkpath("plugins/kelime");
    QFile idx(reg.filePath("index.json"));
    CHECK(idx.open(QIODevice::WriteOnly));
    idx.write(R"({"registry":1,"plugins":[
        {"id":"kelime","name":"Kelime Sayacı","version":"1.0.0",
         "description":"d","author":"v","permissions":["ui"],
         "file":"plugins/kelime/plugin.js","minApp":""}
    ]})");
    idx.close();
    QFile js(reg.filePath("plugins/kelime/plugin.js"));
    CHECK(js.open(QIODevice::WriteOnly));
    js.write("// @name Kelime Sayacı\n// @version 1.0.0\n");
    js.close();

    qputenv("VERSO_PLUGIN_REGISTRY",
            ("file://" + reg.path()).toUtf8());
    QString err;
    const auto list = PluginStore::fetchIndex(&err);
    CHECK(err.isEmpty());
    CHECK(list.size() == 1);
    CHECK(list[0].id == "kelime");
    const QString src =
        QString::fromUtf8(PluginStore::fetchUrl(PluginStore::pluginUrl(list[0]), &err));
    CHECK(err.isEmpty());
    CHECK(src.contains("@version 1.0.0"));
    // Olmayan dosya: hata dönmeli
    PluginStore::Entry missing = list[0];
    missing.file = "plugins/yok/plugin.js";
    CHECK(PluginStore::fetchUrl(PluginStore::pluginUrl(missing), &err).isEmpty());
    CHECK(!err.isEmpty());
    qunsetenv("VERSO_PLUGIN_REGISTRY");
}

static void testUpdateCheck() {
    // Mağaza güncelleme kararı AboutInfo karşılaştırmasıyla verilir
    CHECK(AboutInfo::isNewer("1.0.1", "1.0.0"));
    CHECK(!AboutInfo::isNewer("1.0.0", "1.0.0"));
    CHECK(!AboutInfo::isNewer("0.9", "1.0.0"));
    CHECK(AboutInfo::isNewer("1.1.0", "1.0.4"));
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    testRegistryBase();
    testParseIndex();
    testValidId();
    testInstallAndVersion();
    testFetchLocalRegistry();
    testUpdateCheck();
    fprintf(stderr, "PLUGINSTORE: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
