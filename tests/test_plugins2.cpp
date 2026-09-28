// Mağaza eklentileri testleri 2: 20 yeni eklenti (geliştirici + dil + web).
// Komutlar bağımsız değişkenle çağrılır; ağ ikilisi (rest, gitignore)
// yükleme + kayıtla sınırlıdır.
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
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

static PluginEngine* g_eng = nullptr;
static QStringList g_cmds;

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
    const QString srcDir = QFileInfo(QString::fromUtf8(__FILE__))
                               .dir()
                               .absoluteFilePath("../resources/plugins");

    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QStringList ids = {
        "logavcisi", "todo",  "importlar", "modeluret", "bosluktemiz",
        "karmasiklik", "rest", "regex",   "gitignore", "snippet",
        "pydoc",     "pyfstring", "cheader", "cppmodern", "javags",
        "html",      "cssrenk",   "jsvar",   "jsarrow",   "webizle"};
    for (const QString& id : ids)
        CHECK(QFile::copy(srcDir + "/" + id + ".js", dir.filePath(id + ".js")));

    PluginEngine eng;
    g_eng = &eng;
    eng.setWorkspaceRoot(dir.path());
    const QString dosya = dir.filePath("cikti.txt");
    eng.setCurrentFileProvider([dosya]() { return dosya; });
    QObject::connect(&eng, &PluginEngine::commandRegistered, &eng,
                     [](const QString& id, const QString&) { g_cmds << id; });
    eng.loadAll(dir.path());
    CHECK(eng.plugins().size() == 20);
    for (const QString& id : ids) yuklendi(id);

    // Log avcısı
    CHECK(cagir("plugin.logavcisi.tara", "x=1\nconsole.log(x)\nprint(y)\n") ==
          "2 artık satır");
    CHECK(cagir("plugin.logavcisi.temizle", "x=1\nconsole.log(x)\nprint(y)\n") ==
          "2 satır");

    // TODO
    CHECK(cagir("plugin.todo.tara", "x=1 # TODO: düzelt\n# FIXME yok\n") ==
          "2 yapılacak");

    // Import
    CHECK(cagir("plugin.importlar.duzenle", "import sys\nimport os\nx=1\n") ==
          "1 blok");
    CHECK(cagir("plugin.importlar.duzenle", "import os\nimport sys\n") == "0 blok");

    // Model üretici
    const QString py = cagir("plugin.modeluret.uret", "py:{\"ad\": \"x\", \"yas\": 30}");
    CHECK(py.contains("@dataclass") && py.contains("ad: str") &&
          py.contains("yas: int"));
    const QString cpp = cagir("plugin.modeluret.uret", "cpp:{\"a\": true}");
    CHECK(cpp.contains("struct Model") && cpp.contains("bool a"));
    CHECK(cagir("plugin.modeluret.uret", "ts:{\"a\": 1}").contains("interface Model"));

    // Boşluk
    CHECK(cagir("plugin.bosluktemiz.temizle", "a  \n\n\nb\n") == "1 satır");

    // Karmaşıklık
    CHECK(cagir("plugin.karmasiklik.olc", "def f():\n return 1\n").contains("temiz"));
    CHECK(cagir("plugin.karmasiklik.olc",
                "def f():\n if a:\n if b:\n if c:\n if d:\n if e:\n if f:\n return\n")
              .contains("riskli: f(7)"));

    // REST + gitignore: ağ yok
    CHECK(g_cmds.contains("plugin.rest.istek"));
    CHECK(g_cmds.contains("plugin.gitignore.uret"));

    // Regex
    CHECK(cagir("plugin.regex.dene", "a(B+)c\naBBBc").contains("1. [0-5] 'aBBBc' (BBB)"));
    CHECK(cagir("plugin.regex.dene", "([\"\nx").startsWith("hata")); // geçersiz kalıp

    // Snippet
    CHECK(cagir("plugin.snippet.kaydet", "merhaba:::selam") == "kaydedildi: merhaba");
    CHECK(cagir("plugin.snippet.liste").contains("merhaba"));
    CHECK(cagir("plugin.snippet.ekle", "merhaba") == "eklendi: merhaba");
    QFile f(dosya);
    CHECK(f.open(QIODevice::ReadOnly));
    CHECK(QString::fromUtf8(f.readAll()).contains("selam"));
    f.close();

    // Python docstring
    CHECK(cagir("plugin.pydoc.tara", "def topla(a, b):\n return a+b\n") ==
          "1 docstring'siz");
    const QString doc = cagir("plugin.pydoc.ekle", "def topla(a, b):\n return a+b\n");
    CHECK(doc == "1 eklendi");

    // Python f-string
    CHECK(cagir("plugin.pyfstring.cevir", "x = \"%s\" % ad\n") == "1 satır");
    CHECK(cagir("plugin.pyfstring.cevir", "x = \"{}\".format(ad)\n") == "1 satır");
    CHECK(cagir("plugin.pyfstring.cevir", "x = f\"{ad}\"\n") == "0 satır");

    // C header guard
    CHECK(cagir("plugin.cheader.ekle", "int x;\n") == "KORUMA_H");
    CHECK(cagir("plugin.cheader.ekle", "#ifndef X_H\nint x;\n") == "zaten var");

    // C++ modern
    const QString mod =
        cagir("plugin.cppmodern.modernlestir", "int *p = NULL;\ntypedef int T;\n");
    CHECK(mod.contains("1 nullptr") && mod.contains("1 using"));

    // Java getter/setter
    CHECK(cagir("plugin.javags.uret", "class A {\nprivate int yas;\nprivate String ad;\n}\n") ==
          "2 alan");

    // HTML
    CHECK(cagir("plugin.html.dogrula", "<div>\n<p>selam\n") == "2 hata");
    CHECK(cagir("plugin.html.dogrula", "<div><p>x</p></div>") == "geçerli");
    CHECK(cagir("plugin.html.iskelet", "Deneme").contains("<title>Deneme</title>"));

    // CSS
    CHECK(cagir("plugin.cssrenk.degisken",
                "a { color: #FF0000; background: #ff0000; }\n") == "1 renk");

    // JS var + arrow
    CHECK(cagir("plugin.jsvar.donustur", "var a = 1;\nvar b = 2;\nb = 3;\n") ==
          "1 const · 1 let");
    CHECK(cagir("plugin.jsarrow.donustur",
                "var f = function (x) { return x * 2; };\n") == "1 işlev");

    // Web önizleme
    CHECK(cagir("plugin.webizle.goster", "<p>Selam</p>").contains("<p>Selam</p>"));
    // Sağlayıcı kurulu: görünüm açık dosyayı (snippet çıktısı) işler
    CHECK(eng.renderView("plugin.webizle.view.gorunum").contains("selam"));

    // quickPick ayrıştırma: JS dizisi tek seçeneğe yapışmamalı (regresyon)
    {
        QJSEngine js;
        CHECK(VersoApi::pickItems(js.evaluate("[\"a\",\"b\",\"c\"]")) ==
              QStringList({"a", "b", "c"}));
        CHECK(VersoApi::pickItems(js.evaluate("\"x\\ny\"")) ==
              QStringList({"x", "y"}));
        CHECK(VersoApi::pickItems(js.evaluate("[\"\"]")).isEmpty());
    }

    for (const auto& p : eng.plugins()) CHECK(!p.quarantined);

    QSettings("Verso", "VersoCoder").remove("plugin/enabled");
    QSettings("Verso", "VersoCoder").remove("plugin/quarantine");
    fprintf(stderr, "PLUGINS2: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
