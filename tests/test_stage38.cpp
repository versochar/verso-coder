// Stage 38 testleri: yol güvenliği (sembolik bağlantı kaçışı), komut
// sertleştirme (kabuk kipi, ortam temizliği, salt-okunur), komut denetimi,
// sağlayıcı sigortası (circuit breaker), yönlendirme rehberi, önbellek
// temizleme, kesintili RAG ilerlemesi ve sızıntı ölçümü.
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QDateTime>
#include <QSettings>
#include <QTemporaryDir>
#include <cmath>
#include <cstdio>

#include "../src/core/AgentTools.h"
#include "../src/core/CacheCleaner.h"
#include "../src/core/CommandAudit.h"
#include "../src/core/LeakWatch.h"
#include "../src/core/PathGuard.h"
#include "../src/core/RagProgress.h"
#include "../src/core/SetupAdvisor.h"
#include "../src/core/ai/LlmProvider.h"
#include "../src/core/ModelCapabilities.h"
#include "../src/core/ai/ModelPool.h"
#include "../src/core/ai/ProviderHealth.h"
#include "../src/core/ai/ProviderPrefs.h"
#include "../src/core/ai/ProviderPricing.h"
#include "../src/core/ai/SecretStore.h"
#include "../src/core/ai/TaskRouter.h"
#include "../src/core/ai/UsageLedger.h"

static int g_pass = 0, g_fail = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (cond) {                                                          \
            ++g_pass;                                                        \
        } else {                                                              \
            ++g_fail;                                                        \
            fprintf(stderr, "FAIL %d %s\n", __LINE__, #cond);                \
            fflush(stderr);                                                  \
        }                                                                    \
    } while (0)

#define CHECK2(cond, why)                                                     \
    do {                                                                      \
        if (cond) {                                                           \
            ++g_pass;                                                         \
        } else {                                                               \
            ++g_fail;                                                         \
            fprintf(stderr, "FAIL %d %s (%s)\n", __LINE__, #cond,             \
                    qPrintable(QString(why)));                                 \
            fflush(stderr);                                                   \
        }                                                                     \
    } while (0)
#define CHECK_GE(a, b) ((a) >= (b))

// küçük yardımcılar
static QString canononRootOf(const PathGuard& g) { return g.canonicalRoot(); }
static bool resolveOk(const PathGuard& g, const QString& in) {
    QString abs, why;
    return g.resolve(in, abs, &why);
}

// --- 1–2: yol güvenliği ---
static void testPathGuard() {
    QTemporaryDir rootDir, outside;
    CHECK(rootDir.isValid() && outside.isValid());
    // Dışarıda "gizli" bir dosya
    QFile secret(outside.path() + "/gizli.txt");
    CHECK(secret.open(QIODevice::WriteOnly)); // dış dosya
    secret.write("GIZLI-IERIK");
    secret.close();
    // Proje içinde normal dosya
    QFile ok(rootDir.path() + "/ok.txt");
    CHECK(ok.open(QIODevice::WriteOnly));
    ok.write("merhaba");
    ok.close();
    CHECK(QDir().mkpath(rootDir.path() + "/alt"));

    PathGuard g(rootDir.path());
    CHECK(!g.canonicalRoot().isEmpty());

    // --- Kuşkullu girdi ---
    QString why;
    CHECK(PathGuard::isSuspiciousInput("", &why));
    CHECK(PathGuard::isSuspiciousInput(QString(5000, 'x'), &why));
    CHECK(PathGuard::isSuspiciousInput(QString("a") + QChar(0) + "b", &why));
    CHECK(!PathGuard::isSuspiciousInput("a/b/c.cpp"));
    CHECK(!PathGuard::isSuspiciousInput("..\\x")); // windows ayırıcı normal yol
    CHECK(!g.isInside(""));
    CHECK(!g.isInside("   x\ty\nz")); // kontrol karakteri

    // --- Normal yollar geçer ---
    QString abs;
    CHECK2(resolveOk(g, "ok.txt"), why);
    CHECK(resolveOk(g, "./ok.txt"));
    CHECK(resolveOk(g, "alt/yeni-dosya.txt"));   // henüz yok
    CHECK(resolveOk(g, rootDir.path() + "/ok.txt")); // mutlak, kök içi
    CHECK(resolveOk(g, "ok.txt"));
    CHECK(!g.isInside(".."));
    CHECK(!g.isInside("../"));
    CHECK(!g.isInside("alt/../../gizli.txt"));
    CHECK(!g.isInside(outside.path() + "/gizli.txt"));

    // --- Sembolik bağlantı kaçışı engellenir ---
    const QString link = rootDir.path() + "/kacak";
    if (QFile::link(outside.path(), link)) {
        QString w2, a2;
        const bool inside = g.resolve("kacak/gizli.txt", a2, &w2);
        CHECK2(!inside, "sembolik bağlantı kaçışı engellenmedi");
        CHECK(!w2.isEmpty());
        // Klasörün kendisi de reddedilmeli
        CHECK(!g.isInside("kacak"));
        // Araca zinciri de reddetmeli ve İÇERİĞİ SIZDIRMAMALI
        AgentTools tools(rootDir.path());
        const ToolResult r = tools.readFile("kacak/gizli.txt");
        CHECK(!r.ok);
        CHECK(r.denied);
        CHECK(!r.output.contains("GIZLI"));
        const ToolResult w = tools.writeFile("kacak/yeni.txt", "yaz");
        CHECK(!w.ok && w.denied);
    } else {
        fprintf(stderr, "NOT: sembolik bağlantı oluşturulamadı, test atlandı\n");
    }

    // --- Kök içindeki sembolik bağlantı İZİN VERİLİR ---
    QDir().mkpath(rootDir.path() + "/hedef");
    QFile ic(rootDir.path() + "/hedef/i.txt");
    CHECK(ic.open(QIODevice::WriteOnly));
    ic.write("ic");
    ic.close();
    if (QFile::link(rootDir.path() + "/hedef", rootDir.path() + "/icbag")) {
        CHECK2(resolveOk(g, "icbag/i.txt"), why); // kök içi bağlantı sorun değil
    }

    // --- progressiveCanonical yardımcıları ---
    const QString canon = PathGuard::canonicalizeBestEffort(rootDir.path() + "/ok.txt");
    CHECK(canon.endsWith("ok.txt"));
    CHECK(QDir::cleanPath(canon) == canon);
    CHECK(PathGuard::isSameOrInside(canononRootOf(g), canon));
    const QStringList segs = PathGuard::segmentsOf("/a/b/c");
    CHECK(segs.first() == "/");
    CHECK(segs.size() == 4);
    CHECK(!PathGuard::progressiveCanonical("/a/b").isEmpty());
}


// --- 3: komut sertleştirme ---
static void testCommandHardening() {
    // Ortam temizliği: anahtarlar komuta taşınmamalı
    qputenv("VERSO_AI_KEY_OPENAI", "gizli-anahtar");
    qputenv("OPENAI_API_KEY", "gizli-anahtar");
    qputenv("ANTHROPIC_API_KEY", "gizli");
    qputenv("NVIDIA_API_KEY", "gizli");
    qputenv("GITHUB_TOKEN", "gizli");
    qputenv("AWS_SECRET_ACCESS_KEY", "gizli");
    qputenv("GIT_AUTHOR_NAME", "Test Kullanıcı"); // korunmalı
    const QProcessEnvironment env = AgentTools::sanitizedEnvironment();
    CHECK(!env.contains("VERSO_AI_KEY_OPENAI"));
    CHECK(!env.contains("OPENAI_API_KEY"));
    CHECK(!env.contains("ANTHROPIC_API_KEY"));
    CHECK(!env.contains("NVIDIA_API_KEY"));
    CHECK(!env.contains("GITHUB_TOKEN"));
    CHECK(!env.contains("AWS_SECRET_ACCESS_KEY"));
    CHECK(env.contains("GIT_AUTHOR_NAME"));
    CHECK(env.contains("PATH"));
    qunsetenv("VERSO_AI_KEY_OPENAI");
    qunsetenv("OPENAI_API_KEY");
    qunsetenv("ANTHROPIC_API_KEY");
    qunsetenv("NVIDIA_API_KEY");
    qunsetenv("GITHUB_TOKEN");
    qunsetenv("AWS_SECRET_ACCESS_KEY");
    qunsetenv("GIT_AUTHOR_NAME");

    // Salt-okunur kip bildirimi tutarlı
    const bool ro = AgentTools::readOnlyAvailable();
    CHECK(ro == !AgentTools::readOnlyWrapper().isEmpty());
    if (ro) CHECK(AgentTools::readOnlyWrapper() == "bwrap" || AgentTools::readOnlyWrapper() == "unshare");

    // Yol uyarısı: mutlak yol varsa bildirilir
    const QString w1 = AgentTools::pathWarning("cat /etc/passwd");
    CHECK(!w1.isEmpty() && w1.contains("/etc/passwd"));
    CHECK(AgentTools::pathWarning("cmake --build build").isEmpty());
    CHECK(AgentTools::pathWarning("ls -la src/core").isEmpty());
    // Göreli yol başında olmayan mutlak yol da yakalanmalı
    CHECK(!AgentTools::pathWarning("cp x /home/kullanici/f.txt").isEmpty());

    // Kabuk kipi
    QTemporaryDir d;
    AgentTools tools(d.path());
    CHECK(tools.shellMode() == AgentTools::ShellMode::Secure);
    tools.setAllowCommand(true);
    tools.setShellMode(AgentTools::ShellMode::Legacy);
    CHECK(tools.shellMode() == AgentTools::ShellMode::Legacy);
    tools.setShellMode(AgentTools::ShellMode::Secure);
    // Güvenli kip profil YÜKLEMEZ: alias tanımlıysa komut etkilenmez
    {
        const QString oldHome = qEnvironmentVariable("HOME");
        qputenv("HOME", d.path().toUtf8());
        // Giriş kabuğu (bash -l) ~/.profile okur. Not: alias'lar etkileşimsiz
        // kabukta genişlemez; profil etkisini FONKSİYON ile gösteriyoruz.
        QFile rc(d.path() + "/.profile");
        CHECK(rc.open(QIODevice::WriteOnly));
        rc.write("profil_komutu() { echo PROFIL_YUKLENDI; }\n");
        rc.close();
        // Güvenli kip: kullanıcı kabuğu YÜKLENMEZ → profil fonksiyonu yoktur
        // ve komut "bulunamadı" hatası verir. Bu, sertleştirmenin taşıdığı
        // güvenlik özelliğinin ta kendisidir.
        const ToolResult r = tools.runCommand("profil_komutu");
        CHECK2(!r.output.contains("PROFIL_YUKLENDI"),
               "güvenli kipte kullanıcı profili yüklendi (sertleştirme işe yaramıyor)");
        CHECK2(r.output.contains("not found") || r.output.contains("kayıtlı değil")
                   || !r.ok,
               "profil komutu güvenli kipte çalışmamalıydı");
        // Eski kipte profil yüklenir
        tools.setShellMode(AgentTools::ShellMode::Legacy);
        const ToolResult r2 = tools.runCommand("profil_komutu");
        tools.setShellMode(AgentTools::ShellMode::Secure);
        qputenv("HOME", oldHome.toUtf8());
        CHECK2(r2.output.contains("PROFIL_YUKLENDI"),
               "eski kipte profil yüklenmedi (beklenen: kullanıcı kabuğu etkisi)");
    }
    // Ortam komuta taşınmaz
    qputenv("VERSO_AI_KEY_OPENAI", "sizma-testi");
    const ToolResult envRun = tools.runCommand("echo ${VERSO_AI_KEY_OPENAI:-bos}");
    CHECK(!envRun.output.contains("sizma-testi"));
    qunsetenv("VERSO_AI_KEY_OPENAI");

    // Tehlikeli komut engellenir ve DENETLENİR
    const QString auditFile = d.path() + "/audit.json";
    {
        CommandAudit au(auditFile);
        au.clear();
        AgentTools t2(d.path());
        t2.setAllowCommand(true);
        t2.audit().clear();
        const ToolResult blocked = t2.runCommand("sudo rm -rf /");
        CHECK(!blocked.ok && blocked.denied);
        CHECK(blocked.output.contains("Güvenlik"));
        CHECK(CHECK_GE(t2.audit().count(), 1));
        CHECK(t2.audit().entries().last().denied);
        // Kullanıcı reddi de denetlenir
        t2.setApprover([](const ToolCall&) { return false; });
        const ToolResult denied = t2.runCommand("make");
        CHECK(!denied.ok && denied.denied);
        CHECK(CHECK_GE(t2.audit().count(), 2));
        // Onaylanan komut kaydı (çıkış kodu + süre)
        t2.setApprover([](const ToolCall&) { return true; });
        const ToolResult okRun = t2.runCommand("echo denetim");
        CHECK(okRun.ok);
        CHECK(t2.audit().count() >= 3);
        CHECK(t2.audit().entries().last().approved);
        CHECK(t2.audit().entries().last().exitCode == 0);
    }
}


// --- 4: komut denetimi ---
static void testCommandAudit() {
    QTemporaryDir d;
    CommandAudit a(d.path() + "/a.json");
    a.clear();
    CHECK(a.count() == 0);
    a.record("run_command", "ls -la", true, false, 0, 120);
    a.record("run_tests", "ctest", true, false, 0, 4200);
    a.record("run_command", "curl x", false, true, -1, 0, "tehlikeli kalıp");
    CHECK(a.count() == 3);
    const CommandAudit::Summary s = a.summary();
    CHECK(s.total == 3 && s.denied == 1 && s.totalMs == 4320);
    // Arama
    CHECK(a.search("ctest").size() == 1);
    CHECK(a.search("run_command").size() == 2);
    CHECK(a.search("yok").isEmpty());
    // Metin çıktısı okunabilir (toplu eklemeden ÖNCE)
    const QString txt = CommandAudit::toText(a.last(3));
    CHECK(txt.contains("ls -la") && txt.contains("ctest"));
    CHECK(txt.contains("⊘") || txt.contains("✓"));
    // Kalıcılık + yuvarlama
    CommandAudit b(d.path() + "/a.json");
    CHECK(b.count() == 3);
    for (int i = 0; i < 300; ++i) b.record("x", "cmd", true, false, 0, 1);
    CHECK(b.count() == 200); // varsayılan üst sınır
    b.setMaxEntries(20);
    CHECK(b.count() <= 20);
    b.clear();
    CHECK(b.count() == 0);
}

// --- 12: sağlayıcı sigortası ---
static void testCircuitBreaker() {
    QTemporaryDir d;
    ProviderHealth h(d.path() + "/health.json");
    h.reset();
    CHECK(!h.isTripped("test-sigorta"));
    CHECK(h.isUsable("test-sigorta"));
    // 429/503 "yoğun" sayılır
    h.recordFailure("test-sigorta", 100, 429);
    CHECK(!h.isTripped("test-sigorta"));
    h.recordFailure("test-sigorta", 100, 503);
    CHECK(!h.isTripped("test-sigorta"));
    h.recordFailure("test-sigorta", 100, 429);
    CHECK(h.isTripped("test-sigorta"));
    CHECK(!h.isUsable("test-sigorta"));
    CHECK(h.cooldownLeft("test-sigorta") > 0);
    CHECK(h.cooldownLeft("test-sigorta") <= ProviderHealth::cooldownSec());
    // Havuzdan elenir
    const QStringList pool{"a", "test-sigorta", "b"};
    const QStringList usable = h.filterUsable(pool);
    CHECK(!usable.contains("test-sigorta"));
    CHECK(usable.contains("a") && usable.contains("b"));
    // Başarı devreyi kapatır
    h.recordSuccess("test-sigorta", 100);
    CHECK(!h.isTripped("test-sigorta"));
    CHECK(h.isUsable("test-sigorta"));
    // Sabit hata (401/404) sigortayı tetiklemez: kalıcı hata
    h.reset();
    for (int i = 0; i < 5; ++i) h.recordFailure("test-sabit", 10, 401);
    CHECK(!h.isTripped("test-sabit"));
    // Kalıcılık
    h.recordFailure("test-sigorta2", 10, 429);
    h.recordFailure("test-sigorta2", 10, 429);
    h.recordFailure("test-sigorta2", 10, 429);
    ProviderHealth h2(d.path() + "/health.json");
    CHECK(h2.isTripped("test-sigorta2"));
}

// --- 13: yönlendirme rehberi ---
static void testSetupAdvisor() {
    QTemporaryDir d;
    qputenv("XDG_DATA_HOME", d.path().toUtf8());
    qputenv("HOME", d.path().toUtf8());
    ProviderPrefs::reset();
    SecretStore store;
    store.setUseKeyring(false);
    // Yerel sağlayıcı: anahtar gerekmez, uyarı beklenmez
    ProviderPrefs::setActiveProvider("ollama");
    const auto local = SetupAdvisor::analyzeProvider("ollama");
    for (const SetupAction& a : local) {
        CHECK(a.title != QStringLiteral("Ollama (yerel) için API anahtarı gerekli"));
    }
    // Anahtarı olmayan bulut sağlayıcısı → kritik eylem
    const auto noKey = SetupAdvisor::analyzeProvider("nvidia-nim");
    bool hasKeyAction = false;
    for (const SetupAction& a : noKey) {
        if (a.title.contains("anahtar")) {
            hasKeyAction = true;
            CHECK(a.severity == SetupAction::Critical);
            CHECK(a.action.contains("build.nvidia.com"));
        }
    }
    CHECK(hasKeyAction);
    // Anahtar varsa eylem kaybolur
    qputenv("VERSO_AI_KEY_NVIDIA_NIM", "test");
    const auto withKey = SetupAdvisor::analyzeProvider("nvidia-nim");
    for (const SetupAction& a : withKey) CHECK(!a.title.contains("anahtar"));
    qunsetenv("VERSO_AI_KEY_NVIDIA_NIM");
    // Ağ yoksa önce ağ uyarısı
    const auto offline = SetupAdvisor::analyzeProvider("openai", false);
    CHECK(!offline.isEmpty());
    CHECK(offline.first().title.contains("Ağ yok"));
    CHECK(offline.first().severity == SetupAction::Critical);
    // Bilinmeyen sağlayıcı
    const auto unknown = SetupAdvisor::analyzeProvider("yok-boyle");
    CHECK(unknown.size() == 1);
    CHECK(unknown.first().severity == SetupAction::Critical);
    // Anahtar ipuçları
    CHECK(SetupAdvisor::keyHintFor("unorouter").contains("unorouter.com"));
    CHECK(SetupAdvisor::keyHintFor("openai").contains("openai.com"));
    CHECK(SetupAdvisor::keyHintFor("yok").isEmpty());
    // Özet
    CHECK(SetupAdvisor::summary({}).contains("hazır"));
    const QList<SetupAction> mixed{{"a", "", "", "", SetupAction::Info, ""},
                                   {"b", "", "", "", SetupAction::Critical, ""}};
    CHECK(SetupAdvisor::summary(mixed).contains("1 kritik"));
    CHECK(SetupAdvisor::mostCritical(mixed).title == "b");
    CHECK(SetupAdvisor::mostCritical({}).title.isEmpty());
    // Anahtarlı hiçbir sağlayıcı yokken ücretsiz seçenek önerilir
    ProviderPrefs::setActiveProvider("openai");
    const auto all = SetupAdvisor::analyze();
    bool found = false;
    for (const SetupAction& a : all)
        if (a.title.contains("ücretsiz seçenek")) found = true;
    CHECK2(found, "anahtarsız durumda ücretsiz seçenek önerisi üretilmedi");
    // Yerel sağlayıcı etkinken bu eylem çıkmamalı (her zaman bir yol vardır)
    ProviderPrefs::setActiveProvider("ollama");
    bool found2 = false;
    for (const SetupAction& a : SetupAdvisor::analyze())
        if (a.title.contains("kullanılabilir")) found2 = true;
    CHECK(!found2);
    ProviderPrefs::reset();
}

// --- 10: önbellek temizleme ---
static void testCacheCleaner() {
    QTemporaryDir d;
    qputenv("XDG_DATA_HOME", d.path().toUtf8());
    const auto entries = CacheCleaner::aiCacheEntries();
    CHECK(entries.size() >= 5);
    bool sawCatalog = false, sawKeys = false, sawUsage = false;
    for (const auto& e : entries) {
        CHECK(!e.id.isEmpty() && !e.label.isEmpty());
        if (e.id == "catalog") sawCatalog = true;
        if (e.id == "keys") {
            sawKeys = true;
            CHECK(!e.removable); // anahtar kasası asla silinmez
        }
        if (e.id == "usage") {
            sawUsage = true;
            CHECK(!e.removable); // kullanım geçmişi veri
        }
    }
    CHECK(sawCatalog && sawKeys && sawUsage);
    // Dosya boyutu ölçümü (gerçek uygulama yolu kullanılır)
    QString cat;
    for (const auto& e : CacheCleaner::aiCacheEntries())
        if (e.id == "catalog") cat = e.path;
    CHECK(!cat.isEmpty());
    CHECK(QDir().mkpath(QFileInfo(cat).absolutePath()));
    QFile f(cat);
    CHECK(f.open(QIODevice::WriteOnly));
    f.write(QByteArray(2000, 'x'));
    f.close();
    CacheCleaner cc;
    CHECK(CHECK_GE(CacheCleaner::aiCacheBytes(), 2000));
    CHECK(cc.totalSummary().contains("AI önbelleği"));
    // Sadece önbellekler temizlenir, geçmiş/kasa korunur
    UsageLedger::instance().record("openai", "gpt-4o", 10, 10);
    CHECK(cc.cleanAiCache(false));
    CHECK(!QFile::exists(cat));
    CHECK(CHECK_GE(UsageLedger::instance().today("openai").calls, 1)); // geçmiş durdu
}

// --- 11: kesintili RAG ilerlemesi ---
static void testRagProgress() {
    QTemporaryDir d;
    RagProgressStore st(d.path() + "/rag.json");
    CHECK(!st.load());
    RagProgress p;
    p.root = d.path();
    p.pending = {"a.cpp", "b.cpp", "c.cpp"};
    p.indexed = 10;
    p.total = 13;
    CHECK(p.valid());
    CHECK(p.doneCount() == 3);
    CHECK(p.percent() == 76); // 10/13
    CHECK(p.describe().contains("3 dosya"));
    // Kalıcılık
    st.setProgress(p);
    RagProgressStore st2(d.path() + "/rag.json");
    CHECK(st2.load());
    CHECK(st2.progress().pending.size() == 3);
    CHECK(st2.matchesRoot(d.path()));
    CHECK(!st2.matchesRoot("/başka/proje"));
    // İlerleme
    st2.advance("a.cpp");
    CHECK(st2.progress().pending.size() == 2);
    CHECK(!st2.progress().pending.contains("a.cpp"));
    CHECK(st2.progress().indexed == 11);
    st2.advance("olmayan.cpp"); // listede yoksa dokunmaz
    CHECK(st2.progress().pending.size() == 2);
    // Bekleme durumu
    CHECK(!st2.isWaiting());
    st2.markWaiting(0);
    CHECK(st2.isWaiting());
    CHECK(CHECK_GE(st2.waitLeftSec(), 30));
    CHECK(st2.progress().describe().contains("beklemede"));
    // Süre doldu
    st2.markWaiting(QDateTime::currentDateTime().toMSecsSinceEpoch() - 1000);
    CHECK(!st2.isWaiting());
    st2.markWaiting(0);
    st2.clearWaiting();
    CHECK(!st2.isWaiting());
    CHECK(st2.progress().waitUntilMs == 0);
    // Temizle
    st2.clear();
    CHECK(!st2.progress().valid());
    CHECK(!QFile::exists(st2.filePath()));
    // Yüzde sınırları
    RagProgress e;
    e.total = 0;
    CHECK(e.percent() == 0);
    e.total = 5;
    e.indexed = 99;
    CHECK(e.percent() == 100);
}

// --- 8–9: sızıntı ölçümü ---
static void testLeakWatch() {
    LeakWatch w("test", 5);
    const double rss = LeakWatch::rssMb();
    w.reset();
    for (int i = 0; i < 5; ++i) {
        // Gerçekten ayırdığımız küçük bir yük
        QList<QByteArray> keep;
        for (int k = 0; k < 200; ++k) keep << QByteArray(1024, 'x');
        keep.clear();
        w.tick();
    }
    const LeakWatch::Result res = w.result();
    CHECK(res.iterations == 5);
    CHECK(!res.verdict.isEmpty());
    if (rss > 0.0) {
        CHECK(res.available);
        CHECK(res.endMb > 0.0);
        CHECK(res.peakMb >= res.endMb - 0.001);
        // Ölçüm kendi içinde tutarlı
        CHECK(qAbs(res.growthMb - (res.endMb - res.startMb)) < 0.01);
    } else {
        // /proc yoksa dürüstçe "ölçülemiyor" demeli
        CHECK(res.verdict.contains("ölçülemiyor"));
    }
    // Eşik ayarı
    const double old = LeakWatch::growthThresholdKb();
    LeakWatch::setGrowthThresholdKb(1.0);
    CHECK(LeakWatch::growthThresholdKb() == 1.0);
    LeakWatch::setGrowthThresholdKb(-5.0);
    CHECK(LeakWatch::growthThresholdKb() >= 1.0);
    LeakWatch::setGrowthThresholdKb(old);
    // İterasyon yokken "ölçüm yok"
    LeakWatch empty("e");
    CHECK(empty.result().verdict.contains("ölçüm yok"));
}

// --- 14: model havuzu hâlâ çalışıyor (Stage 37 geri uyumu) ---
static void testModelPoolRegression() {
    ModelPool::clear();
    const ProviderSpec uno = ProviderRegistry::byId("unorouter");
    ModelPool::setCatalog("unorouter", {"absolutereality:free", "qwen3-32b:free",
                                        "gpt-4o:free", "text-embedding-3-small"});
    const QString next = ModelPool::nextCandidate(uno, "gpt-4o:free", QStringList{"gpt-4o:free"});
    CHECK(!next.isEmpty());
    CHECK(ModelPool::isChatCapable(next));
    CHECK(!ModelCapabilities::isEmbeddingModel(next));
    ModelPool::clear();
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir home;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, home.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, home.path());
    qputenv("XDG_DATA_HOME", home.path().toUtf8());
    qputenv("HOME", home.path().toUtf8());

    testPathGuard();
    testCommandHardening();
    testCommandAudit();
    testCircuitBreaker();
    testSetupAdvisor();
    testCacheCleaner();
    testRagProgress();
    testLeakWatch();
    testModelPoolRegression();

    fprintf(stderr, "STAGE38: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
