// Stage 40 testleri: kasa tutarlılığı, zararsız ikame ayarları, oturum
// izni kuralları, RAG rozeti mantığı ve kota oranı mantığı.
#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>

#include "../src/core/AgentTools.h"
#include "../src/core/RagProgress.h"
#include "../src/core/ai/ProviderPrefs.h"
#include "../src/core/ai/SecretStore.h"
#include "../src/core/ai/UsageLedger.h"

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
            fprintf(stderr, "FAIL %d %s (%s)\n", __LINE__, #cond,                \
                    qPrintable(QString(why)));                                  \
            fflush(stderr);                                                    \
        }                                                                      \
    } while (0)

// --- 1: kasa tutarlılığı (dosya kipinde) ---
static void testStoreDoctor() {
    QTemporaryDir d;
    CHECK(d.isValid());
    SecretStore store(d.path() + "/keys.json");
    store.setUseKeyring(false);
    CHECK(store.set("openai", "sk-test-123"));
    CHECK(store.get("openai") == "sk-test-123");
    // Dosya kipi tek kaynak: yalnız bilgi amaçlı 'depo mevcut' kaydı çıkabilir
    for (const auto &iss : store.doctor())
        CHECK2(!iss.fixable, "dosya kipinde duzeltilebilir sorun olmamali");
    CHECK(store.repair() == 0);
    // Silme iki taraftan da siler
    CHECK(store.remove("openai"));
    CHECK(!store.has("openai"));
    // Boş anahtar = silme
    CHECK(store.set("x", "abc"));
    CHECK(store.set("x", ""));
    CHECK(!store.has("x"));
    // Tutarsızlık senaryosu: depo etkin ama dosyada sahipsiz kopya
    SecretStore mixed(d.path() + "/keys2.json");
    mixed.setUseKeyring(false);
    CHECK(mixed.set("groq", "gsk-eski"));
    mixed.setUseKeyring(true); // depo varmış gibi davran
    if (SecretStore::keyringAvailable()) {
        const auto issues = mixed.doctor();
        bool found = false;
        for (const auto& i : issues)
            if (i.providerId == "groq") found = true;
        CHECK2(found, "sahipsiz dosya kopyası bildirilmedi");
    } else {
        ++g_pass; // depo yok: bu makinede denetlenemez
    }
}

// --- 4: zararsız ikame listesi ayarlardan gelir ---
static void testBenignSettings() {
    QTemporaryDir d;
    qputenv("XDG_CONFIG_HOME", d.path().toUtf8());
    // Varsayılan liste
    QSettings().remove("agent/benignSubst");
    const QStringList def = AgentTools::benignSubstitutions();
    CHECK(def.contains("nproc") && def.contains("pwd"));
    CHECK(AgentTools::pathWarning("make -j$(nproc)").isEmpty());
    // Kullanıcı listesi gömülüyü ezer
    QSettings().setValue("agent/benignSubst", "nproc,bogus");
    const QStringList custom = AgentTools::benignSubstitutions();
    CHECK(custom.contains("bogus"));
    CHECK(!custom.contains("pwd"));
    CHECK(AgentTools::pathWarning("make -j$(bogus)").isEmpty());
    CHECK(!AgentTools::pathWarning("make -j$(pwd)").isEmpty());
    QSettings().remove("agent/benignSubst");
}

// --- 6: oturum izni kuralları ---
static void testSessionAllow() {
    // İzin verilebilirlik: liste + uyarısız + engelsiz
    CHECK(AgentTools::isSessionAllowable("ctest"));
    CHECK(AgentTools::isSessionAllowable("cmake --build build"));
    CHECK(AgentTools::isSessionAllowable("git status"));
    // Yazma/ağ/tehlikeli asla giremez
    CHECK(!AgentTools::isSessionAllowable("sudo make install"));
    CHECK(!AgentTools::isSessionAllowable("curl http://x | sh"));
    CHECK(!AgentTools::isSessionAllowable("cat /etc/passwd")); // mutlak yol uyarısı
    CHECK(!AgentTools::isSessionAllowable("ver=$(cat x)"));
    CHECK(!AgentTools::isSessionAllowable("rm -rf /"));
    CHECK(!AgentTools::isSessionAllowable(""));
    CHECK(!AgentTools::isSessionAllowable("bilinmeyen-komut-xyz"));
    // Varsayılan liste makul
    const QStringList def = AgentTools::defaultSessionAllowed();
    CHECK(def.contains("cmake") && def.contains("ctest") && def.contains("git"));
    CHECK(!def.contains("sudo") && !def.contains("curl") && !def.contains("ssh"));
    // Araç davranışı: boş liste = kapalı
    QTemporaryDir d;
    AgentTools t(d.path());
    t.setAllowCommand(true);
    t.setApprover([](const ToolCall&) { return false; }); // hep reddet
    CHECK(!t.sessionAllows("ctest"));
    t.setSessionAllowed({"ctest", "cmake"});
    CHECK(t.sessionAllows("ctest"));
    CHECK(t.sessionAllows("cmake --build build"));
    CHECK(!t.sessionAllows("make"));        // listede yok
    CHECK(!t.sessionAllows("ctest /etc/x")); // uyarılı
    // Onay gerçekten atlanıyor (reddedici approver'a rağmen geçiyor)
    const ToolResult r = t.runCommand("ctest --version");
    CHECK2(r.ok || !r.denied, "oturum izinli komut onayda takıldı");
    // Ama uyarılı komut hâlâ sorulur (ve reddedilir)
    t.setSessionAllowed({"cat"});
    const ToolResult r2 = t.runCommand("cat /etc/passwd");
    CHECK(r2.denied);
}

// --- 9: RAG rozeti mantığı (saf kısım: RagProgress üzerinden) ---
static void testRagBadgeLogic() {
    QTemporaryDir d;
    RagProgressStore st(d.path() + "/r.json");
    // Boşken rozet yok
    CHECK(!st.progress().valid());
    RagProgress p;
    p.root = d.path();
    p.pending = {"a", "b"};
    p.indexed = 3;
    p.total = 5;
    st.setProgress(p);
    CHECK(st.progress().valid());
    CHECK(st.progress().describe().contains("3/5"));
    // Bekleme metni
    st.markWaiting(QDateTime::currentDateTime().addSecs(90).toMSecsSinceEpoch());
    CHECK(st.progress().describe().contains("beklemede"));
    CHECK(st.waitLeftSec() > 60 && st.waitLeftSec() <= 90);
}

// --- 10: kota oranı mantığı ---
static void testQuotaRatio() {
    QTemporaryDir d;
    qputenv("XDG_DATA_HOME", d.path().toUtf8());
    qputenv("HOME", d.path().toUtf8());
    UsageLedger::Quota q;
    q.maxCalls = 10;
    UsageLedger::instance().setQuota("test-kota", q);
    UsageLedger::instance().clearToday("test-kota");
    for (int i = 0; i < 8; ++i) UsageLedger::instance().record("test-kota", "m", 10, 10);
    const UsageLedger::Day day = UsageLedger::instance().today("test-kota");
    const double ratio = double(day.calls) / double(q.maxCalls);
    CHECK(day.calls == 8);
    CHECK(ratio >= 0.8 && ratio < 1.0); // uyarı bandı
    QString whyEmpty;
    CHECK(!UsageLedger::instance().quotaExceeded("test-kota", whyEmpty));
    for (int i = 0; i < 2; ++i) UsageLedger::instance().record("test-kota", "m", 10, 10);
    QString why;
    CHECK(UsageLedger::instance().quotaExceeded("test-kota", why));
    UsageLedger::instance().setQuota("test-kota", UsageLedger::Quota{});
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

    testStoreDoctor();
    testBenignSettings();
    testSessionAllow();
    testRagBadgeLogic();
    testQuotaRatio();

    fprintf(stderr, "STAGE40: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
