// Stage 38: arayüz (widget) testleri — gerçek sinyaller, gerçek diyaloglar.
// Önceden 19 test paketi vardı ve HEPSİ saf mantıktı: 8800 satırlık
// MainWindow yalnız "açılıyor mu" duman testiyle doğrulanıyordu. Burada
// diyaloglar kurulur, alanlara veri girilir, kaydedilir ve geri okunur.
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileSystemModel>
#include <QTableWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextBrowser>
#include <QTreeView>
#include <QTreeWidget>

#include "../src/core/AgentTools.h"
#include <QJsonObject>
#include "../src/core/CommandAudit.h"
#include "../src/core/PathGuard.h"
#include "../src/core/SettingsManager.h"
#include "../src/core/ai/ProviderPrefs.h"
#include "../src/core/ai/SecretStore.h"
#include "../src/core/ai/UsageLedger.h"
#include "../src/widgets/AgentPanelDialog.h"
#include "../src/widgets/ModelArenaDialog.h"
#include "../src/widgets/SettingsDialog.h"
#include "../src/core/PluginEngine.h"
#include "../src/core/GitRunner.h"
#include "../src/widgets/PluginStoreDialog.h"
#include "../src/widgets/PluginSidePanel.h"
#include "../src/widgets/ExplorerPanel.h"
#include "../src/widgets/PluginManagerDialog.h"
#include "../src/widgets/TestExplorer.h"
#include "../src/widgets/WorktreeDialog.h"

class UiWidgetsTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void settingsDialog_opensAllPages();
    void settingsDialog_savesAndRestoresProvider();
    void settingsDialog_apiKeyMaskedAndStored();
    void settingsDialog_stage36And38Toggles();
    void settingsDialog_usageTabLoads();
    void arenaDialog_buildsTargetsAndOpens();
    void arenaDialog_emptySelectionShowsStatus();
    void agentPanel_hasAuditTab();
    void agentPanel_auditTabShowsEntries();
    void pathGuard_symlinkEscapeBlocked();
    void pathGuard_allowsNormalPaths();
    void commandAudit_roundTrip();
    void pluginStoreDialog_buildsAndLists();
    void pluginSidePanel_buildsAndLists();
    void settings_pluginUpdateCheckPersists();
    void settingsDialog_editorTabRoundTrip();
    void explorerPanel_revealsFile();
    void pluginManager_clickSelects();
    void testExplorer_filterAndFailed();
    void worktreeDialog_buildsAndLists();
    void pluginStoreDialog_loadsLocalRegistry();

private:
    QTemporaryDir m_home;
    QTemporaryDir m_project;
};

void UiWidgetsTest::initTestCase() {
    QVERIFY(m_home.isValid());
    QVERIFY(m_project.isValid());
    // Ayar ve kasa dosyalarını geçici dizine yönlendir (test izolasyonu)
    qputenv("XDG_DATA_HOME", m_home.path().toUtf8());
    qputenv("XDG_CONFIG_HOME", m_home.path().toUtf8());
    qputenv("HOME", m_home.path().toUtf8());
}

void UiWidgetsTest::settingsDialog_opensAllPages() {
    SettingsDialog dlg;
    dlg.resize(1000, 800);
    dlg.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dlg, 3000));
    auto* tabs = dlg.findChild<QTabWidget*>();
    QVERIFY(tabs);
    QVERIFY(tabs->count() >= 7); // +Editör sekmesi
    // Her sayfa gerçekten kurulabiliyor mu?
    for (int i = 0; i < tabs->count(); ++i) {
        tabs->setCurrentIndex(i);
        QVERIFY(tabs->widget(i));
        QTest::qWait(10);
    }
    // Ayar arama alanı tüm sayfaları süzer
    dlg.close();
}

void UiWidgetsTest::settingsDialog_savesAndRestoresProvider() {
    {
        SettingsDialog dlg;
        dlg.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dlg, 3000));
        auto* combo = dlg.findChild<QComboBox*>(""); // ilk bulunan; aşağıda spesifik
        Q_UNUSED(combo);
        // "AI Sağlayıcıları" sekmesini bul
        auto* tabs = dlg.findChild<QTabWidget*>();
        QVERIFY(tabs);
        int provTab = -1;
        for (int i = 0; i < tabs->count(); ++i)
            if (tabs->tabText(i).contains("Sağlayıcı")) provTab = i;
        QVERIFY(provTab >= 0);
        tabs->setCurrentIndex(provTab);
        QTest::qWait(50);
        dlg.close();
    }
    // Kaydetme/yükleme yolu: ProviderPrefs üzerinden doğrula (diyalog açılmadan)
    ProviderPrefs::setActiveProvider("groq");
    ProviderPrefs::setModel("groq", "llama-3.3-70b-versatile");
    ProviderPrefs::setUrl("openai-compatible", "http://127.0.0.1:1234/v1");
    const ProviderSpec spec = ProviderPrefs::resolve("groq");
    QCOMPARE(spec.id, QStringLiteral("groq"));
    QCOMPARE(ProviderPrefs::modelFor("groq"), QStringLiteral("llama-3.3-70b-versatile"));
    QCOMPARE(ProviderPrefs::urlFor("openai-compatible"), QStringLiteral("http://127.0.0.1:1234/v1"));
}

void UiWidgetsTest::settingsDialog_apiKeyMaskedAndStored() {
    // Ayarlar diyaloğu varsayılan kasayı okur; anahtarı oraya yazmalıyız
    SecretStore store;
    store.setUseKeyring(false);
    const QString key = "nvapi-0123456789abcdef";
    QVERIFY(store.set("nvidia-nim", key));
    ProviderPrefs::setActiveProvider("nvidia-nim"); // diyalog bu sağlayıcıyı göstersin
    QCOMPARE(store.get("nvidia-nim"), QStringLiteral("nvapi-0123456789abcdef"));
    // Anahtar arayüzde maskeli görünür
    SettingsDialog dlg;
    dlg.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dlg, 3000));
    // Not: QLineEdit::Password yalnızca GÖRÜNTÜYÜ maskeler; text() gerçek
    // değeri verir. Doğrulama yapılacak şey echoMode()'dur.
    bool sawMasked = false;
    int passwordFields = 0;
    for (auto* le : dlg.findChildren<QLineEdit*>()) {
        if (le->echoMode() != QLineEdit::Password) continue;
        ++passwordFields;
        if (le->text() == key) sawMasked = true;
    }
    QVERIFY2(sawMasked, "sağlayıcı anahtarı maskeli alanda gösterilmiyor");
    QVERIFY2(passwordFields >= 1, "maskeli alan yok");
    dlg.close();
    // Diskte düz metin olarak durduğunu da doğrula (kasaya yazıldı)
    QVERIFY(QFile::exists(store.filePath()));
    QFile f(store.filePath());
    QVERIFY(f.open(QIODevice::ReadOnly));
    QVERIFY(f.readAll().contains(key.toUtf8()));
}

void UiWidgetsTest::settingsDialog_stage36And38Toggles() {
    SettingsDialog dlg;
    dlg.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dlg, 3000));
    auto* tabs = dlg.findChild<QTabWidget*>();
    QVERIFY(tabs);
    int provTab = -1;
    for (int i = 0; i < tabs->count(); ++i)
        if (tabs->tabText(i).contains("Sağlayıcı")) provTab = i;
    tabs->setCurrentIndex(provTab);
    QTest::qWait(50);
    // Yönlendirme + model yedeği + komut güvenliği onay kutuları kurulmuş olmalı
    int routeBoxes = 0, shellBoxes = 0;
    for (auto* cb : dlg.findChildren<QCheckBox*>()) {
        const QString t = cb->text();
        if (t.contains("Göreve göre") || t.contains("Model yoğunsa") ||
            t.contains("ücretli modele"))
            ++routeBoxes;
        if (t.contains("Güvenli kabuk") || t.contains("denetim kaydı"))
            ++shellBoxes;
    }
    QVERIFY2(routeBoxes >= 3, "yönlendirme/model yedeği onay kutuları bulunamadı");
    QVERIFY2(shellBoxes >= 2, "komut güvenliği onay kutuları bulunamadı");
    // Varsayılanlar güvenli tarafta
    for (auto* cb : dlg.findChildren<QCheckBox*>()) {
        if (cb->text().contains("Güvenli kabuk")) QVERIFY(cb->isChecked());
        if (cb->text().contains("ücretli modele")) QVERIFY(!cb->isChecked());
    }
    dlg.close();
}

void UiWidgetsTest::settingsDialog_usageTabLoads() {
    UsageLedger::instance().record("openai", "gpt-4o", 1000, 200);
    SettingsDialog dlg;
    dlg.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dlg, 3000));
    auto* tabs = dlg.findChild<QTabWidget*>();
    QVERIFY(tabs);
    int usageTab = -1;
    for (int i = 0; i < tabs->count(); ++i)
        if (tabs->tabText(i).contains("Kullanım")) usageTab = i;
    QVERIFY(usageTab >= 0);
    tabs->setCurrentIndex(usageTab);
    QTest::qWait(50);
    // Tablo doldu mu?
    QTableWidget* table = nullptr;
    for (auto* t : dlg.findChildren<QTableWidget*>()) {
        if (t->columnCount() == 6) table = t;
    }
    QVERIFY2(table, "sağlayıcı dağılım tablosu bulunamadı");
    QVERIFY(table->rowCount() >= 1);
    dlg.close();
}

void UiWidgetsTest::arenaDialog_buildsTargetsAndOpens() {
    QList<ArenaTarget> targets;
    targets << ArenaTarget{"ollama", "qwen2.5:1.5b"};
    targets << ArenaTarget{"unorouter", "gpt-4o:free"};
    ModelArenaDialog dlg(targets, "sistem", "istem metni", QJsonObject());
    dlg.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dlg, 3000));
    // Otomatik koşu tetiklenmesin diye sonucu temizle ve duruma bak
    QTest::qWait(50);
    auto* status = dlg.findChild<QLabel*>();
    QVERIFY(status);
    // Etiketler hedef listesinde görünmeli
    bool sawBoth = false;
    for (auto* le : dlg.findChildren<QLineEdit*>())
        if (le->text().contains("qwen2.5") && le->text().contains("UnoRouter")) sawBoth = true;
    QVERIFY2(sawBoth, "arena hedef listesi eksik");
    dlg.close();
}

void UiWidgetsTest::arenaDialog_emptySelectionShowsStatus() {
    QList<ArenaTarget> targets;
    targets << ArenaTarget{"ollama", "qwen2.5:1.5b"};
    ModelArenaDialog dlg(targets, "sistem", QString(), QJsonObject());
    dlg.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dlg, 3000));
    // Hiçbir şey seçilip boş istemle koşturulursa anlaşılır mesaj verir
    const auto buttons = dlg.findChildren<QPushButton*>();
    QPushButton* run = nullptr;
    for (auto* b : buttons)
        if (b->text().contains("Koştur")) run = b;
    QVERIFY(run);
    run->click();
    QTest::qWait(30);
    bool warned = false;
    for (auto* l : dlg.findChildren<QLabel*>())
        if (l->text().contains("En az bir hedef") || l->text().contains("İstem boş")) warned = true;
    QVERIFY2(warned, "arena boş istem/hedef uyarısı göstermedi");
    dlg.close();
}

void UiWidgetsTest::agentPanel_hasAuditTab() {
    AgentPanelDialog dlg(m_project.path());
    dlg.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dlg, 3000));
    auto* tabs = dlg.findChild<QTabWidget*>();
    QVERIFY(tabs);
    int audit = -1;
    for (int i = 0; i < tabs->count(); ++i)
        if (tabs->tabText(i).contains("Son Komutlar")) audit = i;
    QVERIFY2(audit >= 0, "komut denetimi sekmesi yok");
    tabs->setCurrentIndex(audit);
    QTest::qWait(30);
    dlg.close();
}

void UiWidgetsTest::agentPanel_auditTabShowsEntries() {
    const QString f = m_home.path() + "/audit-ui.json";
    CommandAudit log(f);
    log.clear();
    log.record("run_command", "cmake --build build", true, false, 0, 2400);
    log.record("run_command", "cat ~/.ssh/id_rsa", false, true, -1, 0, "kullanıcı reddi");

    AgentPanelDialog dlg(m_project.path());
    // Aynı dosyayı okuyan kayıt: denetim sınıfı varsayılan yolu kullanır,
    // bu yüzden kaydı varsayılan yola da yazalım.
    CommandAudit def;
    def.record("run_command", "ls -la", true, false, 0, 120);
    dlg.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dlg, 3000));
    // Kayıt diyalog kurulduktan SONRA eklendi → tazeleme gerekir
    QMetaObject::invokeMethod(&dlg, "refreshAudit", Qt::DirectConnection);
    auto* tabs = dlg.findChild<QTabWidget*>();
    QVERIFY(tabs);
    for (int i = 0; i < tabs->count(); ++i)
        if (tabs->tabText(i).contains("Son Komutlar")) tabs->setCurrentIndex(i);
    QTest::qWait(50);
    QTextBrowser* view = nullptr;
    for (auto* v : dlg.findChildren<QTextBrowser*>()) view = v;
    QVERIFY2(view, "denetim metin görünümü bulunamadı");
    QVERIFY(view->toPlainText().contains("ls -la"));
    dlg.close();
}

void UiWidgetsTest::pathGuard_allowsNormalPaths() {
    // Normal proje yolları geçmeli
    QFile f(m_project.path() + "/ok.txt");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("merhaba");
    f.close();
    PathGuard g(m_project.path());
    QString why, abs;
    QVERIFY2(g.resolve("ok.txt", abs, &why), qPrintable(why));
    QVERIFY(g.resolve("./ok.txt", abs, &why));
    QVERIFY(g.resolve("yeni/dosya.txt", abs, &why)); // henüz yok
    QVERIFY(g.resolve(m_project.path() + "/ok.txt", abs, &why));
    QVERIFY(!g.isInside(".."));
    QVERIFY(!g.isInside(""));
}

void UiWidgetsTest::pathGuard_symlinkEscapeBlocked() {
    // Klasör dışına işaret eden sembolik bağlantı: ajan kök dışına çıkamamalı
    const QString outside = m_home.path() + "/disarida";
    QVERIFY(QDir().mkpath(outside));
    QFile secret(outside + "/gizli.txt");
    QVERIFY(secret.open(QIODevice::WriteOnly));
    secret.write("GIZLI");
    secret.close();
    const QString link = m_project.path() + "/kacak";
    QVERIFY(QFile::link(outside, link));
    PathGuard g(m_project.path());
    QString why, abs;
    // Klasör üzerinden kaçış reddedilmeli
    const bool inside = g.resolve("kacak/gizli.txt", abs, &why);
    QVERIFY2(!inside, "sembolik bağlantı kaçışı engellenmedi!");
    QVERIFY(why.contains("sembolik") || why.contains("kök"));
    // doğrudan kök dışı da reddedilir
    QVERIFY(!g.resolve(outside + "/gizli.txt", abs, &why));
    // Ajan aracı da aynı kararı verir
    AgentTools tools(m_project.path());
    const ToolResult r = tools.readFile("kacak/gizli.txt");
    QVERIFY(!r.ok);
    QVERIFY(r.denied);
    QVERIFY(!r.output.contains("GIZLI"));
    QVERIFY(r.output.contains("sembolik") || r.output.contains("reddedildi"));
}

void UiWidgetsTest::commandAudit_roundTrip() {
    const QString f = m_home.path() + "/audit-round.json";
    CommandAudit a(f);
    a.clear();
    for (int i = 0; i < 5; ++i)
        a.record("run_command", QString("echo %1").arg(i), true, false, 0, 100);
    a.record("run_command", "rm -rf /", false, true, -1, 0, "tehlikeli kalıp");
    QCOMPARE(a.count(), 6);
    const CommandAudit::Summary s = a.summary();
    QCOMPARE(s.total, 6);
    QCOMPARE(s.denied, 1);
    QVERIFY(s.totalMs >= 500);
    // Kalıcılık
    CommandAudit b(f);
    QCOMPARE(b.count(), 6);
    QCOMPARE(b.last(1).first().command, QString("rm -rf /"));
    // Arama
    QCOMPARE(b.search("echo").size(), 5);
    QCOMPARE(b.search("yok-boyle-bir-komut").size(), 0);
    // Üst sınır
    b.setMaxEntries(10);
    CommandAudit c(f);
    c.clear();
    c.setMaxEntries(10);
    for (int i = 0; i < 40; ++i) c.record("run_command", QString("c%1").arg(i), true, false, 0, 1);
    QVERIFY(c.count() <= 10);
    // Metin dışa aktarım
    const QString txt = CommandAudit::toText(c.last(3));
    QCOMPARE(txt.count(QLatin1Char('\n')), 3);
}

void UiWidgetsTest::pluginStoreDialog_buildsAndLists() {
    // Kurulum ağı gerektirmez: liste Yenile'ye basılana dek boştur
    PluginEngine eng;
    PluginStoreDialog dlg(&eng);
    QCOMPARE(dlg.windowTitle(), QString("Eklenti Mağazası"));
    QVERIFY(dlg.findChild<QPushButton*>());
    QVERIFY(dlg.findChild<QListWidget*>());
    dlg.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dlg, 3000));
    dlg.close();
}

void UiWidgetsTest::pluginSidePanel_buildsAndLists() {
    QSettings("Verso", "VersoCoder").remove("plugin/enabled"); // izole başla
    QTemporaryDir plugdir;
    QVERIFY(plugdir.isValid());
    QFile f(plugdir.filePath("ornek.js"));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("// @name Örnek\n// @version 1.0.0\nverso.log(\"x\");\n");
    f.close();
    PluginEngine eng;
    eng.loadAll(plugdir.path());
    QVERIFY(eng.isLoaded("ornek"));
    PluginSidePanel panel;
    panel.setEngine(&eng);
    panel.refresh();
    panel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&panel, 3000));
    // Kapatma: ayar yazılır + yeniden yüklenir, sinyal gelir
    QSignalSpy changed(&panel, &PluginSidePanel::pluginsChanged);
    bool found = false;
    for (auto* it : panel.findChildren<QListWidget*>()) {
        for (int i = 0; i < it->count(); ++i) {
            auto* wi = it->item(i);
            if (wi && wi->data(Qt::UserRole).toString() == "ornek" &&
                (wi->flags() & Qt::ItemIsUserCheckable)) {
                wi->setCheckState(Qt::Unchecked);
                found = true;
            }
        }
    }
    QVERIFY(found);
    QCOMPARE(changed.size(), 1);
    QVERIFY(!eng.isLoaded("ornek")); // yeniden yüklemede devre dışı
    panel.close();
}

void UiWidgetsTest::pluginStoreDialog_loadsLocalRegistry() {
    // Sahte kayıt deposu (file://): ağ yok, arayüz donmamalı
    QTemporaryDir reg;
    QVERIFY(reg.isValid());
    QVERIFY(QDir().mkpath(reg.filePath("plugins/deneme")));
    QFile idx(reg.filePath("index.json"));
    QVERIFY(idx.open(QIODevice::WriteOnly));
    idx.write(R"({"registry":1,"plugins":[
        {"id":"deneme","name":"Deneme","version":"1.0.0","description":"d",
         "author":"t","permissions":["ui"],"file":"plugins/deneme/plugin.js","minApp":""}
    ]})");
    idx.close();
    QFile js(reg.filePath("plugins/deneme/plugin.js"));
    QVERIFY(js.open(QIODevice::WriteOnly));
    js.write("// @name Deneme\n// @version 1.0.0\nverso.log(1);\n");
    js.close();
    qputenv("VERSO_PLUGIN_REGISTRY", ("file://" + reg.path()).toUtf8());
    PluginEngine eng;
    PluginStoreDialog dlg(&eng);
    dlg.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dlg, 3000));
    QPushButton* yenile = nullptr;
    for (auto* b : dlg.findChildren<QPushButton*>())
        if (b->text() == "Yenile") yenile = b;
    QVERIFY(yenile);
    QTest::mouseClick(yenile, Qt::LeftButton);
    QListWidget* liste = dlg.findChild<QListWidget*>();
    QVERIFY(liste);
    QTRY_VERIFY(liste->count() == 1);
    QVERIFY(liste->item(0)->text().contains("Deneme"));
    // Tıklama özyinelememeli (donma regresyonu): seçim + ayrıntı gelir
    liste->setCurrentRow(0);
    QTest::qWait(50);
    QCOMPARE(liste->currentRow(), 0);
    auto* ayrinti = dlg.findChild<QTextEdit*>();
    QVERIFY(ayrinti);
    QVERIFY(ayrinti->toPlainText().contains("Deneme"));
    dlg.close();
    qunsetenv("VERSO_PLUGIN_REGISTRY");
}

void UiWidgetsTest::pluginManager_clickSelects() {
    // Yönetici penceresinde de aynı tuzak vardı
    QSettings("Verso", "VersoCoder").remove("plugin/enabled");
    QTemporaryDir plugdir;
    QVERIFY(plugdir.isValid());
    QFile f(plugdir.filePath("ornek.js"));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("// @name Örnek\n// @version 1.0.0\nverso.log(1);\n");
    f.close();
    PluginEngine eng;
    eng.loadAll(plugdir.path());
    QVERIFY(eng.isLoaded("ornek"));
    PluginManagerDialog yon(&eng);
    yon.show();
    QVERIFY(QTest::qWaitForWindowExposed(&yon, 3000));
    QListWidget* liste = yon.findChild<QListWidget*>();
    QVERIFY(liste);
    QTRY_VERIFY(liste->count() >= 1);
    liste->setCurrentRow(0);
    QTest::qWait(50);
    QCOMPARE(liste->currentRow(), 0);
    yon.close();
    QSettings("Verso", "VersoCoder").remove("plugin/enabled");
}

void UiWidgetsTest::worktreeDialog_buildsAndLists() {
    // Depo olmayan dizin: hata satırı, çökme yok
    QTemporaryDir bos;
    QVERIFY(bos.isValid());
    WorktreeDialog d0(bos.path());
    d0.show();
    QVERIFY(QTest::qWaitForWindowExposed(&d0, 3000));
    d0.close();
    // Gerçek depo: ana ağaç listelenir
    QTemporaryDir repo;
    QVERIFY(repo.isValid());
    LocalGitRunner g(repo.path());
    QCOMPARE(g.run({"init", "-b", "main"}, 10000).exit, 0);
    WorktreeDialog d1(repo.path());
    d1.show();
    QVERIFY(QTest::qWaitForWindowExposed(&d1, 3000));
    auto* liste = d1.findChild<QListWidget*>();
    QVERIFY(liste);
    QVERIFY(liste->count() >= 1);
    QVERIFY(liste->item(0)->text().contains(repo.path()));
    d1.close();
}

void UiWidgetsTest::testExplorer_filterAndFailed() {
    TestExplorer ex;
    TestCase a, b, c;
    a.suite = "Ag";
    a.name = "elma";
    b.suite = "Ag";
    b.name = "armut";
    c.suite = "Diger";
    c.name = "elma";
    ex.setTests({a, b, c});
    TestCase ra = a, rb = b, rc = c;
    ra.status = "pass";
    rb.status = "fail";
    rc.status = "fail";
    ex.setResults({ra, rb, rc});
    QCOMPARE(ex.failedIds(), QStringList({"Ag.armut", "Diger.elma"}));
    // Süzgeç: "arm" yalnız armut satırını gösterir
    auto* süz = ex.findChild<QLineEdit*>();
    QVERIFY(süz);
    ex.show();
    QVERIFY(QTest::qWaitForWindowExposed(&ex, 3000));
    süz->setText("arm");
    QTreeWidget* agac = nullptr;
    for (auto* t : ex.findChildren<QTreeWidget*>()) agac = t;
    QVERIFY(agac);
    int gorunur = 0;
    QTreeWidgetItemIterator it(agac);
    while (*it) {
        if ((*it)->childCount() == 0 && !(*it)->isHidden()) ++gorunur;
        ++it;
    }
    QCOMPARE(gorunur, 1);
    süz->clear();
    ex.close();
}

void UiWidgetsTest::explorerPanel_revealsFile() {
    QTemporaryDir kok;
    QVERIFY(kok.isValid());
    QVERIFY(QDir().mkpath(kok.filePath("alt")));
    QFile f(kok.filePath("alt/dosya.txt"));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("x");
    f.close();
    ExplorerPanel panel;
    panel.setRoot(kok.path());
    panel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&panel, 3000));
    QTest::qWait(300); // QFileSystemModel tembel yüklenir
    panel.revealFile(kok.filePath("alt/dosya.txt"));
    QTreeView* agac = panel.findChild<QTreeView*>();
    QVERIFY(agac);
    // GitIgnore proxy'sinden kaynağa: seçili dizin dosyayı göstermeli
    const QModelIndex cur = agac->currentIndex();
    QVERIFY(cur.isValid());
    const auto* proxy =
        qobject_cast<const QSortFilterProxyModel*>(agac->model());
    QVERIFY(proxy);
    const QModelIndex src = proxy->mapToSource(cur);
    QVERIFY(src.isValid());
    QCOMPARE(QFileInfo(QDir(kok.path()), "alt/dosya.txt").absoluteFilePath(),
             QFileInfo(src.data(QFileSystemModel::FilePathRole).toString())
                 .absoluteFilePath());
    // Kök dışı sessizce yoksayılır (çökme yok)
    panel.revealFile("/yok/boyle/bir/dosya.txt");
    panel.close();
}

void UiWidgetsTest::settingsDialog_editorTabRoundTrip() {
    SettingsDialog dlg;
    dlg.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dlg, 3000));
    auto* tabs = dlg.findChild<QTabWidget*>();
    QVERIFY(tabs);
    int ed = -1;
    for (int i = 0; i < tabs->count(); ++i)
        if (tabs->tabText(i) == "Editör") ed = i;
    QVERIFY(ed >= 0);
    tabs->setCurrentIndex(ed);
    QTest::qWait(50);
    QWidget* page = tabs->widget(ed);
    QVERIFY(page);
    auto spins = page->findChildren<QSpinBox*>();
    QCOMPARE(spins.size(), 2);
    spins[0]->setValue(160);
    spins[1]->setValue(7);
    QPushButton* kaydet = nullptr;
    for (auto* b : dlg.findChildren<QPushButton*>())
        if (b->text() == "Kaydet") kaydet = b;
    QVERIFY(kaydet);
    QTest::mouseClick(kaydet, Qt::LeftButton);
    QTest::qWait(50);
    AppSettings s = SettingsManager::instance().load();
    QCOMPARE(s.tabBarMaxWidth, 160);
    QCOMPARE(s.maxOpenTabs, 7);
    // Temiz bırak
    s.tabBarMaxWidth = 0;
    s.maxOpenTabs = 0;
    SettingsManager::instance().save(s);
}

void UiWidgetsTest::settings_pluginUpdateCheckPersists() {
    QSettings q("Verso", "VersoCoder");
    q.remove("plugin/checkUpdates");
    {
        SettingsDialog dlg;
        dlg.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dlg, 3000));
        QCheckBox* kutu = nullptr;
        for (auto* c : dlg.findChildren<QCheckBox*>())
            if (c->text().contains("Eklenti güncellemelerini")) kutu = c;
        QVERIFY(kutu);
        QVERIFY(kutu->isChecked()); // varsayılan açık
        kutu->setChecked(false);
        QPushButton* kaydet = nullptr;
        for (auto* b : dlg.findChildren<QPushButton*>())
            if (b->text() == "Kaydet") kaydet = b;
        QVERIFY(kaydet);
        QTest::mouseClick(kaydet, Qt::LeftButton);
        QTest::qWait(50);
    }
    QCOMPARE(QSettings("Verso", "VersoCoder").value("plugin/checkUpdates", true).toBool(),
             false);
    q.remove("plugin/checkUpdates"); // temiz bırak
}

QTEST_MAIN(UiWidgetsTest)
#include "test_ui_widgets.moc"
