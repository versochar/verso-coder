// Mağaza eklentileri testleri: 10 eklenti yüklenir, komutlar başsız çağrılır.
// Ağ gerektiren ikisi (çeviri, hava) yalnız yükleme + kayıt denetimi yapar
// (ctest'te dış ağ yok). Kaynaklar: resources/plugins/*.js
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>

#include "../src/core/PluginEngine.h"

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

static QString g_srcDir;
static PluginEngine* g_eng = nullptr;
static QStringList g_cmds;
static QStringList g_themes;
static QStringList g_terms;

static const PluginEngine::Plugin* bul(const QString& id) {
    for (const auto& p : g_eng->plugins())
        if (p.id == id) return &p;
    return nullptr;
}

static void yuklendi(const QString& id) {
    const auto* p = bul(id);
    if (!p || !p->loaded)
        fprintf(stderr, "PLUGIN %s: %s\n", qUtf8Printable(id),
                qUtf8Printable(p ? p->error : QString("bulunamadı")));
    CHECK(p && p->loaded);
}

static QString cagir(const QString& cmd, const QString& arg = QString()) {
    return g_eng->callCommand(cmd, arg).toString();
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QSettings("Verso", "VersoCoder").remove("plugin/enabled");
    QSettings("Verso", "VersoCoder").remove("plugin/quarantine");
    g_srcDir = QFileInfo(QString::fromUtf8(__FILE__))
                   .dir()
                   .absoluteFilePath("../resources/plugins");

    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QStringList ids = {"md",         "json",  "csv",  "kodsay", "lorem",
                             "ceviri",     "hava",  "githizli", "notlar", "pastel"};
    for (const QString& id : ids)
        CHECK(QFile::copy(g_srcDir + "/" + id + ".js", dir.filePath(id + ".js")));
    CHECK(QFile::exists(dir.filePath("pastel.js")));

    PluginEngine eng;
    g_eng = &eng;
    eng.setWorkspaceRoot(dir.path()); // notlar durumu izole olur
    QObject::connect(&eng, &PluginEngine::commandRegistered, &eng,
                     [](const QString& id, const QString&) { g_cmds << id; });
    QObject::connect(&eng, &PluginEngine::themeRegistered, &eng,
                     [](const QString&, const QString& n, const QString&) {
                         g_themes << n;
                     });
    QObject::connect(&eng, &PluginEngine::terminalRequested, &eng,
                     [](const QString&, const QString& t) { g_terms << t; });
    eng.loadAll(dir.path());
    CHECK(eng.plugins().size() == 10);
    for (const QString& id : ids) yuklendi(id);

    // Markdown
    CHECK(g_cmds.contains("plugin.md.goster"));
    const QString md = cagir("plugin.md.goster", "# Merhaba\n**kalın** ve *yatık*");
    CHECK(md.contains("<h1>Merhaba</h1>"));
    CHECK(md.contains("<strong>kalın</strong>"));
    CHECK(md.contains("<em>yatık</em>"));
    CHECK(eng.renderView("plugin.md.view.gorunum").contains("<html>"));

    // JSON
    CHECK(cagir("plugin.json.dogrula", "{\"a\":1}") == "geçerli");
    CHECK(cagir("plugin.json.dogrula", "{bozuk").startsWith("hata"));
    CHECK(cagir("plugin.json.duzenle", "{\"a\":1}").contains("\"a\": 1"));

    // CSV
    CHECK(g_cmds.contains("plugin.csv.tablo"));
    const QString tab = cagir("plugin.csv.tablo", "ad,yas\nAli,30");
    CHECK(tab.contains("<table"));
    CHECK(tab.contains("<th>ad</th>"));
    CHECK(tab.contains("<td>Ali</td>"));
    CHECK(eng.renderView("plugin.csv.view.gorunum").contains("<html>"));

    // Kod sayacı
    const QString ist = cagir("plugin.kodsay.istatistik", "int a;\n// yorum\n\nx();\n");
    CHECK(ist.contains("5 satır")); // sondaki boş satır dahil
    CHECK(ist.contains("1 yorum"));

    // Lorem
    CHECK(cagir("plugin.lorem.uret", "2").split("\n\n").size() == 2);

    // Çeviri + hava: ağ yok, yalnız kayıt
    CHECK(g_cmds.contains("plugin.ceviri.cevir"));
    CHECK(g_cmds.contains("plugin.hava.durum"));

    // Git hızlı komut
    CHECK(cagir("plugin.githizli.hizli", "git status --short") ==
          "gönderildi: git status --short");
    CHECK(g_terms.contains("git status --short\n"));

    // Notlar (izole workspace durumu)
    CHECK(cagir("plugin.notlar.listele") == "not yok");
    CHECK(cagir("plugin.notlar.ekle", "süt al") == "eklendi");
    CHECK(cagir("plugin.notlar.listele").contains("1. süt al"));
    CHECK(cagir("plugin.notlar.sil", "1") == "silindi");
    CHECK(cagir("plugin.notlar.listele") == "not yok");

    // Pastel temalar
    CHECK(g_themes.size() == 3);
    CHECK(g_themes.contains("Pastel Koyu"));
    CHECK(g_themes.contains("Pastel Açık"));
    CHECK(g_themes.contains("Pastel Kontrast"));

    // Karantina yok
    for (const auto& p : eng.plugins()) CHECK(!p.quarantined);

    QSettings("Verso", "VersoCoder").remove("plugin/enabled");
    QSettings("Verso", "VersoCoder").remove("plugin/quarantine");
    fprintf(stderr, "PLUGINS: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
