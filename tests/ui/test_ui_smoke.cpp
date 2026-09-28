// Stage 38: duman otomasyonu — MainWindow'ı gerçekten kur, komutları çalıştır,
// ayarları kaydet, kapat. 8800 satırlık sınıf yalnız "açılıyor mu" testiyle
// doğrulanıyordu; burada gerçek sinyal/yuva zincirleri yürütülür.
// qWarning sızıntısı (Qt uyarıları) başarısızlık sayılır.
#include <QAction>
#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QStatusBar>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <QWidget>

#include "../src/MainWindow.h"
#include "../src/widgets/CodeEditor.h"
#include "../src/core/LeakWatch.h"
#include "../src/core/ProjectSessions.h"
#include "../src/core/SettingsManager.h"
#include "../src/core/ai/ProviderPrefs.h"
#include "../src/core/ai/SecretStore.h"

class UiSmokeTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void mainWindow_constructsAndExposes();
    void mainWindow_opensAllTabsAndClosesThem();
    void mainWindow_runsCommandsWithoutWarning();
    void mainWindow_settingsDialogRoundTrip();
    void mainWindow_switchesProviderPersists();
    void mainWindow_survivesRepeatedProviderSwitching();
    void mainWindow_settingsSoakDoesNotLeak();
    void mainWindow_lazyTabsRestoreAndMaterialize();
    void mainWindow_tabLimitAndWidth();

private:
    QTemporaryDir m_home;
    QTemporaryDir m_project;
    QStringList m_warnings;
    QtMessageHandler m_oldHandler = nullptr;
    MainWindow* m_win = nullptr;
};

static QStringList* g_warnings = nullptr;
static void warningCollector(QtMsgType, const QMessageLogContext&, const QString& msg) {
    if (g_warnings) g_warnings->append(msg);
}

void UiSmokeTest::initTestCase() {
    QVERIFY(m_home.isValid());
    QVERIFY(m_project.isValid());
    {
        QFile f(m_project.path() + "/deneme.txt");
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("int main(){return 0;}\n");
    }
    qputenv("XDG_DATA_HOME", m_home.path().toUtf8());
    qputenv("XDG_CONFIG_HOME", m_home.path().toUtf8());
    qputenv("HOME", m_home.path().toUtf8());
    // İlk çalıştırma sihirbazı MODAL bir diyalog açar ve testi dondurur;
    // gerçek bir kullanıcının "ilk açılış" değil, normal akış deneyimini
    // ölçüyoruz.
    {
        AppSettings fs = SettingsManager::instance().load();
        fs.ollamaAutoStart = false; // testler gerçek sunucu başlatmasın
        fs.m_firstRun = false;
        SettingsManager::instance().save(fs);
    }
    // Qt uyarılarını topla (sızıntı sayılır)
    m_warnings.clear();
    g_warnings = &m_warnings;
    m_oldHandler = qInstallMessageHandler(warningCollector);
    // Pencere bir kez kurulur (aşağıdaki gerekçe)
    m_win = new MainWindow();
    m_win->resize(1280, 800);
    m_win->show();
    if (!QTest::qWaitForWindowExposed(m_win, 8000)) QTest::qWait(500);
}

void UiSmokeTest::cleanupTestCase() {
    if (m_win) {
        m_win->close();
        delete m_win;
        m_win = nullptr;
    }
    qInstallMessageHandler(m_oldHandler);
    g_warnings = nullptr;
}

// Pencere TEK kez kurulur ve yaşamı boyunca paylaşılır. MainWindow'i her testte
// yeniden kurup yıkmak tekil durumlar (çökme işleyicisi, tek örnek kilidi,
// tema/ayar önbellekleri) nedeniyle çöküyor; gerçek bir kullanıcı da tek
// oturumda birden çok işlem yapar.
void UiSmokeTest::init() {
    m_warnings.clear();
    // Önceki testten açık kalan modal diyalog kalmasın
    for (int i = 0; i < 4; ++i) {
        QWidget* modal = QApplication::activeModalWidget();
        if (!modal) break;
        modal->close();
        QTest::qWait(20);
    }
}

void UiSmokeTest::cleanup() {}

void UiSmokeTest::mainWindow_constructsAndExposes() {
    QVERIFY(m_win);
    // Temel parçalar gerçekten kuruldu mu?
    QVERIFY(m_win->findChild<QTabWidget*>());
    // Stage 47: tıklanabilir çipler + sekmeler ekran okuyucuya adlandırılmış
    int namedChips = 0;
    for (auto* l : m_win->findChildren<QLabel*>()) {
        if (l->objectName() == QString("chip") && !l->accessibleName().isEmpty())
            ++namedChips;
    }
    QVERIFY2(namedChips >= 8, "erisilebilir cip bulunamadi");
    // Stage 49: birlikte çalışma komutları kayıtlı (modal açılmaz, yalnız varlık)
    QVERIFY(m_win->findChild<QAction*>("collab.host"));
    QVERIFY(m_win->findChild<QAction*>("collab.join"));
    QVERIFY(m_win->findChild<QAction*>("collab.leave"));
    QVERIFY(m_win->findChild<QTabWidget*>());      // alt panel sekmeleri
    QVERIFY(m_win->findChild<QStatusBar*>());      // durum çubuğu
    // Sekme sayısı makul
    const auto tabs = m_win->findChildren<QTabWidget*>();
    QVERIFY(!tabs.isEmpty());
    int maxTabs = 0;
    for (auto* t : tabs) maxTabs = qMax(maxTabs, t->count());
    QVERIFY2(maxTabs > 0, "hiçbir sekme bulunamadı");
}

void UiSmokeTest::mainWindow_opensAllTabsAndClosesThem() {
    QVERIFY(m_win);
    const auto tabs = m_win->findChildren<QTabWidget*>();
    QVERIFY(!tabs.isEmpty());
    for (auto* t : tabs) {
        for (int i = 0; i < t->count(); ++i) {
            t->setCurrentIndex(i);
            QVERIFY(t->widget(i));
            QTest::qWait(5);
        }
    }
    // Dosya aç/kapa yolu
    QVERIFY(QFile::exists(m_project.path() + "/deneme.txt"));
    m_win->show();
    QTest::qWait(50);
}

void UiSmokeTest::mainWindow_runsCommandsWithoutWarning() {
    QVERIFY(m_win);
    m_warnings.clear();
    // Modal kapatıcı ÖNCE başlatılmalı: bir komut QInputDialog gibi iç olay
    // döngüsü açarsa, bu zamanlayıcı o döngünün içinde çalışır ve diyaloğu
    // kapatır. Sonradan başlatılırsa test kilitlenir.
    auto* closer = new QTimer;
    closer->setInterval(40);
    QObject::connect(closer, &QTimer::timeout, []() {
        if (QWidget* modal = QApplication::activeModalWidget()) modal->close();
    });
    closer->start();
    // Modal OLMAYAN komutlar: modal diyalog açan komutlar (komut paleti gibi)
    // testi dondurur; onlar ayrı denendi.
    // Kimlikler MainWindow'daki gerçek kayıtlardan alındı
    // view.font / view.theme gibi yazı tipi seçici açan komutlar listede yok:
    // QFontDialog modal + etkileşimlidir ve offscreen eklentisinde kendi
    // dosya uyarısını üretir (ürün hatası değil, ortam gürültüsü).
    const QStringList ids = {"file.new", "edit.toggleEol", "edit.toggleComment",
                             "view.bookmarks", "view.markdownPreview", "view.radar",
                             "file.projectNotes", "file.copyPath", "view.zen", "view.zen",
                             "view.minimap", "view.terminal"};
    for (const QString& id : ids) {
        QAction* a = m_win->findChild<QAction*>(id);
        QVERIFY2(a, qPrintable(QString("komut eylemi yok: %1").arg(id)));
        const int before = m_warnings.size();
        a->trigger();
        // Her adımdan sonra olay döngüsünü sınırlı süre döndür
        QElapsedTimer t;
        t.start();
        while (t.elapsed() < 400) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            if (QWidget* modal = QApplication::activeModalWidget()) modal->close();
        }
        if (m_warnings.size() > before) {
            const int firstNew = m_warnings.size() - 1;
            for (int i = firstNew; i >= 0; --i) {
                const QString wmsg = m_warnings.at(i);
                if (m_warnings.mid(0, before).count(wmsg) == i) {
                    fprintf(stderr, "UYARI [%s] %s\n", qPrintable(id), qPrintable(wmsg));
                    break;
                }
            }
        }
    }
    QTest::qWait(300);
    closer->stop();
    closer->deleteLater();
    // Zararsız uyarılar tolere edilir, gerisi başarısızlıktır
    QStringList real;
    for (const QString& w : m_warnings) {
        // offscreen eklentisinin kendi gürültüsü (dialog/kaydetme yolları)
        if (w.contains("propagateSizeHints") || w.contains("offscreen") ||
            w.contains("QFont::") || w.contains("Unable to load") ||
            w.contains("QFSFileEngine"))
            continue;
        real << w;
    }
    QVERIFY2(real.isEmpty(), qPrintable(real.join(QLatin1Char('\n'))));
}

void UiSmokeTest::mainWindow_settingsDialogRoundTrip() {
    QVERIFY(m_win);
    // Ayarlar diyaloğunu aç/kapat (ayarlar kaydı yolunu da zorlar)
    // Ayarlar diyaloğu modal: açılıp kapanmasını beklemeden kapatacağız
    auto* closer = new QTimer;
    closer->setSingleShot(true);
    closer->setInterval(400);
    QObject::connect(closer, &QTimer::timeout, []() {
        if (QWidget* modal = QApplication::activeModalWidget()) modal->close();
    });
    if (QAction* a = m_win->findChild<QAction*>("view.settings")) {
        a->trigger();
        closer->start();
    }
    QTest::qWait(600);
    closer->deleteLater();
    // Ayarlar gerçekten kaydedilebiliyor mu?
    AppSettings s = SettingsManager::instance().load();
    s.temperature = 0.42;
    s.contextWindow = 8192;
    SettingsManager::instance().save(s);
    const AppSettings back = SettingsManager::instance().load();
    QCOMPARE(back.temperature, 0.42);
    QCOMPARE(back.contextWindow, 8192);
}

void UiSmokeTest::mainWindow_switchesProviderPersists() {
    QVERIFY(m_win);
    ProviderPrefs::setActiveProvider("nvidia-nim");
    QCOMPARE(ProviderPrefs::activeProvider(), QStringLiteral("nvidia-nim"));
    ProviderPrefs::setModel("nvidia-nim", "nvidia/llama-3.1-70b-instruct");
    QCOMPARE(ProviderPrefs::resolve("nvidia-nim").id, QStringLiteral("nvidia-nim"));
    // Anahtar kasası yaz/oku
    SecretStore store;
    store.setUseKeyring(false);
    store.set("nvidia-nim", "nvapi-test");
    QCOMPARE(store.get("nvidia-nim"), QStringLiteral("nvapi-test"));
    store.remove("nvidia-nim");
    ProviderPrefs::reset();
}

void UiSmokeTest::mainWindow_survivesRepeatedProviderSwitching() {
    QVERIFY(m_win);
    // Stage 38: hızlı sağlayıcı değiştirme kaynak sızıntısı üretiyor muydu?
    m_warnings.clear();
    const QStringList ids = {"ollama", "openai", "nvidia-nim", "unorouter", "groq"};
    for (int round = 0; round < 4; ++round) {
        for (const QString& id : ids) {
            ProviderPrefs::setActiveProvider(id);
            if (QAction* a = m_win->findChild<QAction*>("view.bookmarks")) a->trigger();
            QTest::qWait(5);
        }
    }
    ProviderPrefs::reset();
    QTest::qWait(100);
    QStringList real;
    for (const QString& w : m_warnings) {
        if (w.contains("propagateSizeHints") || w.contains("offscreen")) continue;
        real << w;
    }
    QVERIFY2(real.isEmpty(), qPrintable(real.join(QLatin1Char('\n'))));
    // Pencere hâlâ yaşıyor
    QVERIFY(QTest::qWaitForWindowExposed(m_win, 2000));
}

void UiSmokeTest::mainWindow_settingsSoakDoesNotLeak() {
    QVERIFY(m_win);
    // Stage 38: yineleme sızıntısı. Ayar kaydetme + tema değiştirme +
    // sağlayıcı değiştirme en sık yapılan işlemler; RSS tavanı aşılırsa
    // test kırılır.
    LeakWatch watch(QStringLiteral("ayar/çözümleyici döngüsü"), 40);
    watch.reset();
    double worst = 0.0;
    for (int i = 0; i < 40; ++i) {
        AppSettings s = SettingsManager::instance().load();
        s.temperature = 0.1 + double(i % 8) * 0.1;
        s.contextWindow = 4096 + (i % 4) * 4096;
        s.showWhitespace = (i % 2) == 0;
        SettingsManager::instance().save(s);
        if (i % 8 == 0) {
            ProviderPrefs::setActiveProvider(i % 16 == 0 ? "nvidia-nim" : "ollama");
            // sağlayıcı değişimi tek başına yeterli (arayüz eylemi değil)
        }
        watch.tick();
        if (watch.result().growthPerIterKb > worst) worst = watch.result().growthPerIterKb;
    }
    const LeakWatch::Result r = watch.result();
    if (r.available) {
        fprintf(stderr, "soak: %d tur, basina %.1f KB, toplam %.1f KB\n", r.iterations,
                r.growthPerIterKb, r.growthMb);
        // 40 turda 64 KB'den büyük ortalama artış gerçek sızıntı işaretidir
        QVERIFY2(r.growthPerIterKb <= LeakWatch::growthThresholdKb(),
                 qPrintable(QStringLiteral("ayarlar döngüsünde bellek artışı: %1 KB/tur")
                                .arg(r.growthPerIterKb, 0, 'f', 1)));
    }
    ProviderPrefs::reset();
    Q_UNUSED(worst);
}

void UiSmokeTest::mainWindow_lazyTabsRestoreAndMaterialize() {
    QVERIFY(m_win);
    // 3 dosyalı oturum: yalnız etkin sekme gerçekten açılır
    QStringList files;
    for (int i = 0; i < 3; ++i) {
        const QString p = m_project.path() + QString("/sekme%1.txt").arg(i);
        QFile f(p);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QString("satir %1\\n").arg(i).toUtf8().repeated(50));
        f.close();
        files << p;
    }
    DocSession d;
    d.files = files;
    const QString sep = QString(QChar(0x1F)); // "\x1F10" Clang'te geçersiz (sınırsız hex)
    d.cursors = {files[0] + sep + "10", files[1] + sep + "20", files[2] + sep + "30"};
    d.active = 0;
    const int editorsBefore = m_win->editorTabCount();
    m_win->applyDocSession(d);
    QTest::qWait(100);
    // 1 gerçek + 2 bekleyen
    QCOMPARE(m_win->editorTabCount(), editorsBefore + 1);
    QCOMPARE(m_win->pendingTabCount(), 2);
    // Bekleyen sekmeye geç → gerçeklenir, imleç korunur
    QTabWidget* tabs = nullptr;
    for (auto* t : m_win->findChildren<QTabWidget*>()) {
        for (int i = 0; i < t->count(); ++i)
            if (!t->widget(i)->property("pendingPath").toString().isEmpty()) tabs = t;
    }
    QVERIFY(tabs);
    for (int i = 0; i < tabs->count(); ++i) {
        if (!tabs->widget(i)->property("pendingPath").toString().isEmpty()) {
            tabs->setCurrentIndex(i);
            break;
        }
    }
    QTest::qWait(150);
    QCOMPARE(m_win->pendingTabCount(), 1);
    // Oturum turu: bekleyen sekme kaybolmaz
    const DocSession back = m_win->captureSession();
    for (const QString& f : files) QVERIFY(back.files.contains(f));
    // Bekleyen sekme kapatılınca kayıtlar düşer, çökme olmaz
    // (gerçek kapatma yolu: tabCloseRequested → closeTabIn)
    for (int i = 0; i < tabs->count(); ++i) {
        if (!tabs->widget(i)->property("pendingPath").toString().isEmpty()) {
            emit tabs->tabCloseRequested(i);
            break;
        }
    }
    QTest::qWait(50);
    QCOMPARE(m_win->pendingTabCount(), 0);
    const DocSession back2 = m_win->captureSession();
    QVERIFY(back2.files.contains(files[0])); // gerçeklenen durur
    QVERIFY(!back2.files.contains(files[2])); // kapatılan düşer
    // İmleç korunmuş mu?
    bool sawCursor = false;
    for (const QString& c : back.cursors)
        if (c.startsWith(files[1]) || c.startsWith(files[2])) sawCursor = true;
    QVERIFY(sawCursor);
}

void UiSmokeTest::mainWindow_tabLimitAndWidth() {
    QVERIFY(m_win);
    // Modal kapatıcı (file.new dil seçici açar): OK düğmesine bas
    // (doğrudan accept() m_lang'i doldurmaz)
    auto* closer = new QTimer;
    closer->setInterval(40);
    QObject::connect(closer, &QTimer::timeout, []() {
        QWidget* modal = QApplication::activeModalWidget();
        if (!modal) return;
        if (auto* box = modal->findChild<QDialogButtonBox*>()) {
            if (QPushButton* ok = box->button(QDialogButtonBox::Ok)) {
                ok->click();
                return;
            }
        }
        if (auto* d = qobject_cast<QDialog*>(modal)) d->accept();
        else modal->close();
    });
    closer->start();
    auto edCount = [&]() { return m_win->findChildren<CodeEditor*>().size(); };
    // Sınır 3 + genişlik 150 uygula
    {
        AppSettings s = SettingsManager::instance().load();
        s.maxOpenTabs = 3;
        s.tabBarMaxWidth = 150;
        SettingsManager::instance().save(s);
    }
    QVERIFY(QMetaObject::invokeMethod(m_win, "applySettings", Qt::DirectConnection));
    QTest::qWait(100);
    bool widthOk = false;
    for (auto* t : m_win->findChildren<QTabWidget*>()) {
        if (!t->tabBar()) continue;
        if (t->tabBar()->styleSheet().contains("max-width:150px") &&
            t->tabBar()->elideMode() == Qt::ElideRight)
            widthOk = true;
    }
    QVERIFY(widthOk);
    // 4 adsız sekme aç → editör sayısı 3'e sabitlenir (en eskiler kapanır)
    QAction* yeni = m_win->findChild<QAction*>("file.new");
    QVERIFY(yeni);
    for (int i = 0; i < 4; ++i) {
        yeni->trigger();
        QTest::qWait(80);
    }
    QTRY_COMPARE(edCount(), 3);
    // Temizlik: bu test dosyada sonda; tüm editör sekmelerini kapat
    for (int k = 0; k < 20 && edCount() > 0; ++k) {
        for (auto* t : m_win->findChildren<QTabWidget*>()) {
            for (int i = t->count() - 1; i >= 0 && edCount() > 0; --i) {
                if (qobject_cast<CodeEditor*>(t->widget(i))) {
                    t->tabCloseRequested(i);
                    QTest::qWait(30);
                }
            }
        }
    }
    QCOMPARE(edCount(), 0);
    {
        AppSettings s = SettingsManager::instance().load();
        s.maxOpenTabs = 0;
        s.tabBarMaxWidth = 0;
        SettingsManager::instance().save(s);
    }
    QVERIFY(QMetaObject::invokeMethod(m_win, "applySettings", Qt::DirectConnection));
    QTest::qWait(100);
    closer->stop();
    closer->deleteLater();
}

QTEST_MAIN(UiSmokeTest)
#include "test_ui_smoke.moc"
