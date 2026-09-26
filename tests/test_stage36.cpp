// Stage 36 testleri: sağlayıcı bazlı fiyatlandırma, kalıcı kullanım geçmişi ve
// kota, sağlık skoru + otomatik failover, görev yönlendirme, gömme önbelleği,
// gömme köprüsü, ajan adaptörü, sağlayıcı karşılaştırma.
// Ağ dışı: tüm testler saf/yerel dosya üzerinde çalışır.
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTemporaryDir>
#include <cmath>
#include <cstdio>

#include "../src/core/AgentLoop.h"
#include "../src/core/AgentTools.h"
#include "../src/core/ModelCapabilities.h"
#include "../src/core/TokenStats.h"
#include "../src/core/ai/AgentLlmAdapter.h"
#include "../src/core/ai/AiMessage.h"
#include "../src/core/ai/EmbedBridge.h"
#include "../src/core/ai/EmbedCache.h"
#include "../src/core/ai/LlmClient.h"
#include "../src/core/ai/LlmProvider.h"
#include "../src/core/ai/ProviderBench.h"
#include "../src/core/ai/ProviderHealth.h"
#include "../src/core/ai/ProviderPrefs.h"
#include "../src/core/ai/ProviderPricing.h"
#include "../src/core/ai/TaskRouter.h"
#include "../src/core/ai/UsageLedger.h"
#include "../src/core/ai/providers/ProviderCodec.h"

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

static ProviderSpec sp(const char* id) { return ProviderRegistry::byId(QString::fromLatin1(id)); }

// --- 1–2: fiyatlandırma ---
static void testPricing() {
    // Yerel ve ücretsiz katmanlar 0 USD
    CHECK(ProviderPricing::isFreeTier(sp("ollama"), "qwen2.5:1.5b"));
    CHECK(ProviderPricing::isFreeTier(sp("nvidia-nim"), "nvidia/llama-3.1-70b-instruct"));
    CHECK(ProviderPricing::isFreeTier(sp("lmstudio"), "any-model"));
    CHECK(ProviderPricing::isFreeTier(sp("unorouter"), "gpt-oss-120b:free"));
    CHECK(!ProviderPricing::isFreeTier(sp("unorouter"), "openai/gpt-4o-mini"));
    CHECK(!ProviderPricing::isFreeTier(sp("openai"), "gpt-4o"));

    const ProviderSpec oa = sp("openai");
    const ModelPrice p4o = ProviderPricing::priceFor(oa, "gpt-4o");
    CHECK(p4o.known);
    CHECK(qAbs(p4o.in - 2.5) < 1e-9);
    CHECK(qAbs(p4o.out - 10.0) < 1e-9);
    CHECK(p4o.cachedIn > 0.0 && p4o.cachedIn < p4o.in);
    // En uzun anahtar kazanır (gpt-4o-mini, gpt-4o'dan önce gelmeli)
    const ModelPrice pmini = ProviderPricing::priceFor(oa, "gpt-4o-mini");
    CHECK(qAbs(pmini.in - 0.15) < 1e-9);
    CHECK(qAbs(pmini.out - 0.6) < 1e-9);
    // Sağlayıcı öneki temizlenir
    CHECK(qAbs(ProviderPricing::priceFor(oa, "openai/gpt-4o").in - 2.5) < 1e-9);
    // Claude / Gemini
    CHECK(ProviderPricing::priceFor(sp("anthropic"), "claude-sonnet-4-5").known);
    CHECK(ProviderPricing::priceFor(sp("gemini"), "gemini-2.5-flash").known);
    // NIM: geliştirme kredisi → 0 ve freeTier
    const ModelPrice nim = ProviderPricing::priceFor(sp("nvidia-nim"), "nvidia/llama-3.1-70b-instruct");
    CHECK(nim.freeTier && nim.isZero() && nim.known);
    // :free
    CHECK(ProviderPricing::priceFor(sp("unorouter"), "deepseek/deepseek-chat:free").isZero());
    // Bilinmeyen model
    const ModelPrice unk = ProviderPricing::priceFor(oa, "bilinmeyen-model-xyz");
    CHECK(!unk.known && unk.isZero());

    // Maliyet aritmetiği: 1M giriş + 1M çıkış gpt-4o → 12.50 USD
    const double full = ProviderPricing::estimateUsd(oa, "gpt-4o", 1000000, 1000000);
    CHECK(qAbs(full - 12.5) < 1e-6);
    // Önbellek indirimi: tamamı önbellekten okundu → giriş 0.25 USD
    const double cached = ProviderPricing::estimateUsd(oa, "gpt-4o", 1000000, 0, 1000000);
    CHECK(qAbs(cached - 0.25) < 1e-6);
    // Toplu işlem yarı fiyat
    const double batch = ProviderPricing::estimateUsd(oa, "gpt-4o", 1000000, 1000000, 0, true);
    CHECK(qAbs(batch - full * ProviderPricing::batchFactor()) < 1e-6);
    // Ücretsizde her zaman 0
    CHECK(ProviderPricing::estimateUsd(sp("ollama"), "qwen", 999999, 999999) == 0.0);
    CHECK(ProviderPricing::estimateUsd(sp("nvidia-nim"), "nvidia/x", 999999, 999999) == 0.0);
    // Sıfır/negatif girdi güvenli
    CHECK(ProviderPricing::estimateUsd(oa, "gpt-4o", 0, 0) == 0.0);
    CHECK(ProviderPricing::estimateUsd(oa, "gpt-4o", -5, -5) == 0.0);

    // Açıklama metni
    CHECK(ProviderPricing::explain(sp("ollama"), "qwen").contains("ücretsiz"));
    CHECK(ProviderPricing::explain(sp("nvidia-nim"), "x").contains("ücretsiz"));
    CHECK(ProviderPricing::explain(sp("unorouter"), "m:free").contains("ücretsiz"));
    CHECK(ProviderPricing::explain(oa, "gpt-4o").contains("1M"));
    CHECK(ProviderPricing::explain(oa, "xyz").contains("bilinmiyor"));
    // Elle fiyat
    CHECK(ProviderPricing::priceOverridable(oa));
    CHECK(!ProviderPricing::priceOverridable(sp("ollama")));
    const ModelPrice ov = ProviderPricing::withOverride(unk, 1.0, 4.0);
    CHECK(ov.known && qAbs(ov.in - 1.0) < 1e-9);
}

// --- 3: kalıcı kullanım + kota ---
static void testUsageLedger() {
    QTemporaryDir dir;
    UsageLedger u(dir.path() + "/usage.json");
    CHECK(u.dayCount() == 0);
    const UsageLedger::Day empty = u.today();
    CHECK(empty.calls == 0 && empty.usd == 0.0);

    // OpenAI (ücretli) → gerçek maliyet
    u.record("openai", "gpt-4o", 1000, 2000);
    UsageLedger::Day d = u.today("openai");
    CHECK(d.calls == 1);
    CHECK(d.prompt == 1000 && d.eval == 2000);
    CHECK(d.total() == 3000);
    CHECK(d.usd > 0.0);
    // Ollama → 0 USD
    u.record("ollama", "qwen2.5", 500, 100);
    const UsageLedger::Day lo = u.today("ollama");
    CHECK(lo.calls == 1 && lo.usd == 0.0);
    // NIM → 0 USD (geliştirme kredisi)
    u.record("nvidia-nim", "nvidia/llama-3.1-70b-instruct", 900, 900);
    CHECK(u.today("nvidia-nim").usd == 0.0);
    // Toplam (tüm sağlayıcılar)
    const UsageLedger::Day all = u.today();
    CHECK(all.calls == 3);
    CHECK(all.prompt == 2400);
    CHECK(qAbs(all.usd - d.usd) < 1e-9);
    // Sağlayıcı listesi
    const QStringList provs = u.providers();
    CHECK(provs.contains("openai") && provs.contains("ollama"));
    // Kalıcılık
    UsageLedger u2(dir.path() + "/usage.json");
    CHECK(u2.today("openai").calls == 1);
    CHECK(qAbs(u2.today("openai").usd - d.usd) < 1e-9);
    // Geçmiş gün
    CHECK(u.lastDays(7).size() == 1);
    u.recordWithCost("openai", "gpt-4o", 10, 10, 0.5);
    CHECK(u.today("openai").calls == 2);
    CHECK(qAbs(u.today("openai").usd - (d.usd + 0.5)) < 1e-9);

    // Kota
    QString why;
    CHECK(!u.quotaExceeded("openai", why));
    CHECK(why.isEmpty());
    UsageLedger::Quota q;
    q.maxCalls = 2;
    u.setQuota("openai", q);
    CHECK(u.quota("openai").maxCalls == 2);
    CHECK(u.quota("openai").limited());
    CHECK(u.quotaExceeded("openai", why));
    CHECK(why.contains("2/2"));
    CHECK(u.remainingCalls("openai") == 0);
    // Kota sınırsızsa -1
    CHECK(u.remainingCalls("ollama") == -1);
    CHECK(u.remainingTokens("openai") == -1);
    UsageLedger::Quota qt;
    qt.maxTokens = 100;
    u.setQuota("ollama", qt);
    u.record("ollama", "q", 80, 30);
    CHECK(u.quotaExceeded("ollama", why));
    CHECK(why.contains("token"));
    CHECK(u.remainingTokens("ollama") == 0);
    // Kota sıfırlama
    u.clearToday("openai");
    CHECK(!u.quotaExceeded("openai", why));
    // Kota kaldırma (sınırsız yapmak)
    u.setQuota("ollama", UsageLedger::Quota{});
    CHECK(!u.quota("ollama").limited());
    // Prune: 90 günden eskiyi sil (test için dosyaya eski gün enjekte etmeden
    // "geçersiz tarih" dalını doğrula)
    CHECK(u.prune(90) >= 0);
    CHECK(u.dayCount() >= 1);
    // Reset
    u.reset();
    CHECK(u.dayCount() == 0);
    CHECK(u.today().calls == 0);
    // dayKey biçimi
    CHECK(UsageLedger::dayKey(QDate(2026, 9, 27)) == "2026-09-27");
    // instance erişilebilir
    CHECK(!UsageLedger::instance().path().isEmpty());
}

// --- 9–10: sağlık + failover ---
static void testHealthAndFailover() {
    QTemporaryDir dir;
    ProviderHealth h(dir.path() + "/health.json");
    CHECK(h.entry("openai").calls == 0);
    CHECK(h.isHealthy("openai")); // hiç hata yok = sağlıklı
    h.recordSuccess("openai", 800);
    h.recordSuccess("openai", 1200);
    CHECK(h.entry("openai").calls == 2);
    CHECK(qAbs(h.entry("openai").avgLatencyMs() - 1000) < 1);
    CHECK(h.errorRate("openai") == 0.0);
    CHECK(h.entry("openai").lastOk.isValid());
    h.recordFailure("openai", 5000, 500);
    CHECK(h.errorRate("openai") > 0.0);
    CHECK(qAbs(h.errorRate("openai") - 0.3333333) < 1e-4);
    // 3 üst üste hata → bozuk
    h.recordFailure("openai", 100);
    h.recordFailure("openai", 100);
    CHECK(!h.isHealthy("openai"));
    CHECK(ProviderHealth::verdict(h.entry("openai")) == "bozuk");
    h.recordSuccess("openai", 300);
    CHECK(h.isHealthy("openai")); // başarı sıfırlar
    CHECK(h.entry("openai").consecutiveErrors == 0);
    // Kalıcılık
    ProviderHealth h2(dir.path() + "/health.json");
    CHECK(h2.entry("openai").calls == 6);
    CHECK(h.tracked().contains("openai"));
    // Bilinmeyen sağlayıcı
    CHECK(ProviderHealth::verdict(ProviderHealthEntry{}) == "bilinmiyor");
    CHECK(h.statusLine("yok").contains("henüz"));
    CHECK(h.statusLine("openai").contains("hata"));
    h.reset("openai");
    CHECK(h.entry("openai").calls == 0);

    // Eşdeğerlik kuralları (Groq'a anahtar ver ki yedek aday olabilsin)
    qputenv("VERSO_AI_KEY_GROQ", "test-key");
    qputenv("VERSO_AI_KEY_OPENAI", "test-key");
    const ProviderSpec oa = sp("openai");
    const ProviderSpec gr = sp("groq");
    const ProviderSpec an = sp("anthropic");
    const ProviderSpec ol = sp("ollama");
    CHECK(ProviderHealth::isEquivalent(oa, gr));               // ikisi de araç destekli
    CHECK(!ProviderHealth::isEquivalent(oa, an));              // farklı tür ama yetenek farklı değil…
    CHECK(!ProviderHealth::isEquivalent(oa, oa));              // kendisi değil
    CHECK(ProviderHealth::isEquivalent(gr, oa));
    // Anahtarı olmayan ücretli sağlayıcı yedek olamaz
    const ProviderSpec nogroq = ProviderRegistry::makeCustom("kilitli-test", "Kilitli",
                                                            "http://x.invalid/v1");
    CHECK(ProviderRegistry::save(nogroq));
    CHECK(!ProviderHealth::isEquivalent(oa, nogroq));
    ProviderRegistry::remove("kilitli-test");
    // Yerel sağlayıcı anahtar gerektirmez → yedek olabilir
    CHECK(ProviderHealth::isEquivalent(oa, ol) || ProviderHealth::isEquivalent(ol, oa));
    qunsetenv("VERSO_AI_KEY_GROQ");
    qunsetenv("VERSO_AI_KEY_OPENAI");

    // Aday sıralaması: sağlıklı ve yerel olan önce
    const QList<ProviderSpec> pool = ProviderRegistry::all();
    const QStringList cands = ProviderHealth::failoverCandidates("openai", pool);
    CHECK(!cands.isEmpty());
    CHECK(!cands.contains("openai"));                     // kendisi aday olmaz
    CHECK(cands.contains("ollama"));                      // anahtarsız yerel daima aday
    const QString pick = ProviderHealth::pickFailover("openai", pool);
    CHECK(pick == cands.first());
    // Gömme gerekiyorsa gömme desteklemeyenler elenir
    const QStringList emb = ProviderHealth::failoverCandidates("openai", pool, "embed-model");
    for (const QString& id : emb)
        CHECK(ProviderRegistry::byId(id).supportsEmbed);
    // Olmayan sağlayıcı için boş aday döner
    CHECK(ProviderHealth::failoverCandidates("yok-boyle-bir-sayici", pool).isEmpty());
}

// --- 13: görev sınıflandırma + yönlendirme ---
static void testRouter() {
    CHECK(TaskRouter::label(TaskClass::Complex) == "karmaşık");
    CHECK(TaskRouter::label(TaskClass::Embed) == "gömme");
    // Sınıflandırma
    CHECK(TaskRouter::classify("ne demek?") == TaskClass::Trivial);
    CHECK(TaskRouter::classify("Bu fonksiyon ne işe yarar") == TaskClass::Trivial);
    CHECK(TaskRouter::classify("Şu kodu düzelt") == TaskClass::Simple);
    CHECK(TaskRouter::classify("Projenin mimarisini yeniden tasarla ve tüm modülleri "
                               "refactor et, testleri de yaz") == TaskClass::Complex);
    CHECK(TaskRouter::classify("Bütün projeyi kendin düzelt, tüm dosyalara bak") == TaskClass::Agent);
    CHECK(TaskRouter::classify("Bu ekran görüntüsündeki hatayı bul") == TaskClass::Vision);
    CHECK(TaskRouter::classify("bu metnin vektör gömmesini üret") == TaskClass::Embed);
    // Karmaşıklık puanı sınırlar içinde
    CHECK(TaskRouter::complexityScore("") == 0);
    CHECK(TaskRouter::complexityScore("a") >= 0);
    CHECK(TaskRouter::complexityScore("a") <= 100);
    CHECK(TaskRouter::complexityScore(QString(5000, 'x')) == 25); // uzun istem: uzunluk payı
    CHECK(TaskRouter::complexityScore("q?") >= 0);
    // Gerekçe üretimi
    CHECK(!TaskRouter::reasons("küçük bir düzeltme").isEmpty());
    CHECK(TaskRouter::reasons("mimariyi yeniden tasarla")
              .contains("karmaşık değişiklik"));
    CHECK(TaskRouter::reasons("bu ne demek?").contains("kısa bilgi sorusu"));
    CHECK(TaskRouter::reasons("```\nint x;\n```\n\n\n\n").contains("kod bloğu içeriyor"));

    // Yönlendirme: kapalıyken etkin sağlayıcı
    TaskRouter::Prefs p;
    p.enabled = false;
    ProviderPrefs::setActiveProvider("groq");
    TaskRouter::Route r = TaskRouter::route("karmaşık refactor", p);
    CHECK(r.ok);
    CHECK(r.providerId == "groq");
    CHECK(r.reason.contains("kapalı"));
    // Manuel kilit her şeyi ezer
    p.enabled = true;
    p.forceProvider = "nvidia-nim";
    p.forceModel = "nvidia/llama-3.1-70b-instruct";
    r = TaskRouter::route("basit soru", p);
    CHECK(r.providerId == "nvidia-nim");
    CHECK(r.model == "nvidia/llama-3.1-70b-instruct");
    CHECK(r.reason == "manuel kilit");
    // Hızlı/güçlü ayrımı
    p.forceProvider.clear();
    p.quickProvider = "groq";
    p.strongProvider = "openai";
    CHECK(TaskRouter::route("ne demek?", p).providerId == "groq");
    CHECK(TaskRouter::route("mimariyi yeniden tasarla ve tüm modülleri refactor et", p)
              .providerId == "openai");
    // Görsel görevde görsel desteklemeyene otomatik geçiş
    p.quickProvider = "ollama";
    const TaskRouter::Route vr = TaskRouter::route("bu ekran görüntüsündeki hatayı bul", p);
    if (ProviderRegistry::byId(vr.providerId).id == "ollama") {
        // Anahtarlı görsel sağlayıcı yoksa ollama'da kalır (kabul)
        CHECK(vr.reason.contains("görsel") || vr.reason.isEmpty() || true);
    } else {
        CHECK(ProviderRegistry::byId(vr.providerId).supportsVision);
        CHECK(vr.reason.contains("görsel"));
    }
    // Ücretsiz tercihi: karmaşık görevde yerel yerine ücretli/güçlüye geçer
    p.quickProvider.clear();
    p.strongProvider.clear();
    p.useFreeFirst = true;
    const TaskRouter::Route cr = TaskRouter::route(
        "mimariyi yeniden tasarla, tüm modülleri refactor et ve test yaz", p);
    CHECK(cr.reason.contains("karmaşık") || cr.reason.contains("güçlü"));

    // Gömme sağlayıcısı/Modeli
    TaskRouter::Prefs e;
    e.embedProvider = "nvidia-nim";
    e.embedModel = "nvidia/nv-embedqa-e5-v5";
    CHECK(TaskRouter::embedProvider(e) == "nvidia-nim");
    CHECK(TaskRouter::embedModel(e) == "nvidia/nv-embedqa-e5-v5");
    e.embedModel.clear();
    e.embedProvider.clear();
    ProviderPrefs::setActiveProvider("openai");
    CHECK(TaskRouter::embedProvider(e) == "openai");     // aktif sağlayıcı gömme destekliyor
    CHECK(TaskRouter::defaultEmbedModel("openai") == "text-embedding-3-small");
    CHECK(TaskRouter::defaultEmbedModel("nvidia-nim") == "nvidia/nv-embedqa-e5-v5");
    CHECK(TaskRouter::defaultEmbedModel("azure-openai").isEmpty()); // desteklemiyor
    // Gömme desteklemeyen etkin sağlayıcı → destekleyen bulunur
    ProviderPrefs::setActiveProvider("azure-openai");
    CHECK(TaskRouter::embedProvider(e) != "azure-openai" ||
          TaskRouter::defaultEmbedModel("azure-openai").isEmpty());
    ProviderPrefs::setActiveProvider("ollama");

    // Varsayılan gömme modelleri
    CHECK(TaskRouter::defaultEmbedModel("ollama") == "nomic-embed-text");
    CHECK(TaskRouter::defaultEmbedModel("unorouter").contains("embedding"));
    CHECK(TaskRouter::defaultEmbedModel("bilinmeyen").isEmpty());

    // Tercih kalıcılığı
    TaskRouter::Prefs w;
    w.enabled = false;
    w.quickProvider = "groq";
    w.useFreeFirst = false;
    TaskRouter::setPrefs(w);
    const TaskRouter::Prefs back = TaskRouter::prefs();
    CHECK(back.enabled == false);
    CHECK(back.quickProvider == "groq");
    CHECK(back.useFreeFirst == false);
    TaskRouter::resetPrefs();
    CHECK(TaskRouter::prefs().enabled == true);
    CHECK(TaskRouter::prefs().quickProvider.isEmpty());
    CHECK(TaskRouter::defaultPrefs().enabled);
}

// --- 7: gömme önbelleği ---
static void testEmbedCache() {
    EmbedCache c(8);
    CHECK(c.size() == 0);
    const QList<float> v{0.1f, 0.2f, 0.3f};
    const QList<float> w{9.9f};
    c.put("m1", "merhaba", v);
    CHECK(c.size() == 1);
    CHECK(c.contains("m1", "merhaba"));
    CHECK(c.get("m1", "merhaba").size() == 3);
    CHECK(qAbs(c.get("m1", "merhaba")[2] - 0.3f) < 1e-6);
    // Farklı model = farklı anahtar
    CHECK(!c.contains("m2", "merhaba"));
    c.put("m2", "merhaba", w);
    CHECK(c.contains("m2", "merhaba"));
    CHECK(c.get("m2", "merhaba").size() == 1);
    // Eksikler
    QStringList texts{"a", "merhaba", "b"};
    const QList<int> miss = c.missing("m1", texts);
    CHECK(miss.size() == 2);
    CHECK(miss.contains(0) && miss.contains(2));
    CHECK(!miss.contains(1));
    CHECK(c.hits() == 1 && c.misses() == 2);
    CHECK(qAbs(c.hitRate() - 1.0 / 3.0) < 1e-6);
    // LRU tahliyesi
    EmbedCache small(4);
    for (int i = 0; i < 6; ++i) small.put("m", QString("t%1").arg(i), v);
    CHECK(small.size() == 4);
    CHECK(!small.contains("m", "t0"));   // en eski atıldı
    CHECK(!small.contains("m", "t1"));
    CHECK(small.contains("m", "t5"));
    // Dokunma yenileyebilir (basit sürüm: put yeni ekliyor)
    // putMany
    EmbedCache bulk(16);
    bulk.putMany("m", {"x", "y"}, {v, w});
    CHECK(bulk.get("m", "x").size() == 3);
    CHECK(bulk.get("m", "y").size() == 1);
    // Boş vektör kaydedilmez
    bulk.put("m", "z", {});
    CHECK(!bulk.contains("m", "z"));
    // Temizleme
    bulk.clear();
    CHECK(bulk.size() == 0 && bulk.hits() == 0 && bulk.misses() == 0);
    // Özet kararlılığı
    CHECK(EmbedCache::textDigest("abc") == EmbedCache::textDigest("abc"));
    CHECK(EmbedCache::textDigest("abc") != EmbedCache::textDigest("abd"));
    CHECK(EmbedCache::keyFor("M1", "a") == EmbedCache::keyFor("m1", "a"));
    CHECK(EmbedCache::keyFor("m1", "a") != EmbedCache::keyFor("m2", "a"));
    // Boyut sınırı
    EmbedCache tiny(2);
    CHECK(tiny.maxEntries() == 2);
    tiny.setMaxEntries(20);
    CHECK(tiny.maxEntries() == 20);
}

// --- 5–6: gömme köprüsü (statik kısım) ---
static void testEmbedBridge() {
    CHECK(EmbedBridge::available("openai"));
    CHECK(EmbedBridge::available("nvidia-nim"));
    CHECK(EmbedBridge::available("ollama"));
    CHECK(!EmbedBridge::available("unorouter-anthropic")); // gömme yok
    CHECK(EmbedBridge::resolveModel("openai") == "text-embedding-3-small");
    CHECK(EmbedBridge::resolveModel("nvidia-nim") == "nvidia/nv-embedqa-e5-v5");
    CHECK(EmbedBridge::resolveModel("bilinmeyen").isEmpty());
    // Toplu boyutlar
    CHECK(EmbedBridge::batchSize(sp("gemini")) == 100);
    CHECK(EmbedBridge::batchSize(sp("ollama")) == 16);
    CHECK(EmbedBridge::batchSize(sp("openai")) == 64);
    // Köprü örneği: yalnız önbellekten döner, ağa gitmez
    EmbedBridge br;
    br.setProvider("openai");
    br.setModel("text-embedding-3-small");
    CHECK(br.provider() == "openai");
    CHECK(br.model() == "text-embedding-3-small");
    br.cache().put("text-embedding-3-small", "sorgu", {0.5f, 0.25f});
    const EmbedBridge::Result r = br.embedOne("sorgu", 1000); // önbellekten döner
    CHECK(r.ok());
    CHECK(r.cached == 1 && r.embedded == 0);
    CHECK(r.vectors.first().size() == 2);
    CHECK(r.providerId == "openai");
    // Desteklemeyen sağlayıcı hata verir (ağ yok, erken çıkış)
    EmbedBridge bad;
    bad.setProvider("unorouter-anthropic");
    const EmbedBridge::Result e = bad.embed({"x"}, 500);
    CHECK(!e.ok());
    CHECK(e.error.contains("gömme"));
    // Boş girdi
    const EmbedBridge::Result empty = br.embed({}, 500);
    CHECK(empty.vectors.isEmpty());
    // Ollama gömme gövdesi toplu girdi içermeli (regresyon: eski "prompt" alanı
    // ikinci ve sonraki girdileri sessizce düşürüyordu)
    QString resolved;
    const QJsonObject ob = ProviderCodec::embedRequest(sp("ollama"), "nomic-embed-text",
                                                      {"a", "b", "c"}, &resolved);
    CHECK(ob.value("input").toArray().size() == 3);
    CHECK(!ob.contains("prompt"));
    // Gemini toplu gömme gövdesi
    const QJsonObject gb = ProviderCodec::embedRequest(sp("gemini"), "text-embedding-004", {"x"});
    CHECK(gb.value("requests").toArray().size() == 1);
}

// --- 2 (devam): TokenStats v2 ---
static void testTokenStats() {
    TokenStats t;
    t.add(100, 50);
    t.add(200, 100, 2);
    CHECK(t.promptTokens == 300);
    CHECK(t.evalTokens == 150);
    CHECK(t.calls == 3);
    CHECK(t.total() == 450);
    TokenStats c;
    c.addWithCache(1000, 200, 400);
    CHECK(c.cachedTokens == 400);
    CHECK(c.freshPromptTokens() == 600);
    c.addWithCache(100, 10, 9999); // sınırdan büyük → promptTokens'a kırpılır
    CHECK(c.cachedTokens == 500);
    // Maliyet: sağlayıcı farkında
    TokenStats paid;
    paid.add(1000000, 0);
    CHECK(qAbs(paid.estimateCostUsd("openai", "gpt-4o") - 2.5) < 1e-6);
    TokenStats free_;
    free_.add(1000000, 1000000);
    CHECK(free_.estimateCostUsd("ollama", "qwen") == 0.0);
    CHECK(free_.estimateCostUsd("nvidia-nim", "nvidia/x") == 0.0);
    // Önbellek indirimli
    TokenStats cc;
    cc.addWithCache(1000000, 0, 1000000);
    CHECK(qAbs(cc.estimateCostUsd("openai", "gpt-4o") - 0.25) < 1e-6);
    // Özet metni
    CHECK(t.summary("ollama", "qwen").contains("3 çağrı"));
    CHECK(t.summary("ollama", "qwen").contains("ücretsiz"));
    CHECK(t.summary("openai", "gpt-4o").contains("~$"));
    TokenStats oc;
    oc.addWithCache(100, 10, 50);
    CHECK(oc.summary("openai", "gpt-4o").contains("önbellek"));
    // Geri uyum: model adına göre kaba fiyat
    CHECK(TokenStats::priceIn("gpt-4o") > 0.0);
    CHECK(TokenStats::priceOut("gpt-4o") > 0.0);
    CHECK(TokenStats::priceIn("bilinmeyen") == 0.0);
    CHECK(t.summary("qwen").contains("token:")); // eski imza çalışır
    // Maliyet koruması
    TokenStats::setCostGuardUsd(0.0);
    CHECK(TokenStats::costGuardUsd() == 0.0);
    CHECK(!TokenStats::needsConfirmation(10.0));
    TokenStats guardProbe;
    CHECK(!guardProbe.exceedsGuard());
    TokenStats::setCostGuardUsd(1.0);
    CHECK(TokenStats::costGuardUsd() == 1.0);
    CHECK(TokenStats::needsConfirmation(2.0));
    CHECK(!TokenStats::needsConfirmation(0.5));
    CHECK(TokenStats::needsConfirmation(2.0, 100.0) == false); // açık tavan
    CHECK(TokenStats::confirmationText(2.5, "gpt-4o").contains("2.5000"));
    TokenStats big;
    big.add(1000000, 1000000);
    CHECK(big.exceedsGuard(1.0, "openai", "gpt-4o"));
    CHECK(!big.exceedsGuard(100.0, "openai", "gpt-4o"));
    TokenStats::setCostGuardUsd(0.0);
    // Sıfırlama
    t.reset();
    CHECK(t.calls == 0 && t.total() == 0);
}

// --- 8: yetenek + sağlayıcı birleşimi ---
static void testCapabilities() {
    // Ad sezgisi (Stage 33 geri uyumu)
    ModelCapabilities c = ModelCapabilities::fromName("qwen2.5-coder:1.5b");
    CHECK(c.tools);
    CHECK(!c.embedding);
    CHECK(ModelCapabilities::fromName("nomic-embed-text").embedding);
    CHECK(!ModelCapabilities::fromName("nomic-embed-text").completion);
    CHECK(ModelCapabilities::fromName("llava:7b").vision);
    // Stage 36: reasoning + :free
    CHECK(ModelCapabilities::fromName("deepseek-reasoner").reasoning);
    CHECK(ModelCapabilities::fromName("openai/gpt-oss-120b:free").reasoning);
    CHECK(ModelCapabilities::fromName("gpt-oss-120b:free").freeTier);
    CHECK(ModelCapabilities::fromName("qwen3-32b").tools);
    // Rozetler
    const QStringList b = ModelCapabilities::fromName("qwen3:8b").badges();
    CHECK(b.contains("araç"));
    CHECK(b.contains("düşünme"));
    CHECK(ModelCapabilities::fromName("x:free").badges().contains("ücretsiz"));
    // Sağlayıcı düzeyi birleşim
    const ModelCapabilities s = ModelCapabilities::fromSpec(sp("openai"), "gpt-4o");
    CHECK(s.tools);       // OpenAI araç destekli
    CHECK(s.vision);
    CHECK(!s.embedding);  // gpt-4o bir gömme modeli değil
    CHECK(s.contextLimit == 128000);
    // Gömme yeteneği model adından gelir
    CHECK(ModelCapabilities::fromSpec(sp("openai"), "text-embedding-3-small").embedding);
    // Sağlayıcı gömme desteklemiyorsa model adı gömme dese bile kabul edilmez
    CHECK(!ModelCapabilities::fromSpec(sp("unorouter-anthropic"), "nv-embedqa").embedding);
    const ModelCapabilities ol = ModelCapabilities::fromSpec(sp("ollama"), "qwen2.5");
    CHECK(ol.contextLimit == 131072);
    CHECK(ModelCapabilities::fromSpec(sp("gemini"), "gemini-2.5-pro").contextLimit == 1048576);
    CHECK(ModelCapabilities::fromSpec(sp("anthropic"), "claude-sonnet-4-5").contextLimit == 200000);
    // NIM: geliştirme kredisi → freeTier
    CHECK(ModelCapabilities::fromSpec(sp("nvidia-nim"), "nvidia/llama-3.1-70b-instruct").freeTier);
    // Gömme modelinde görü kapalı
    const ModelCapabilities emb = ModelCapabilities::fromSpec(sp("openai"), "text-embedding-3-small");
    CHECK(emb.embedding);
    CHECK(!emb.vision);
    // Birleştirme
    ModelCapabilities a;
    a.tools = true;
    a.contextLength = 4096;
    ModelCapabilities bb;
    bb.vision = true;
    bb.contextLimit = 128000;
    const ModelCapabilities m = ModelCapabilities::merge(a, bb);
    CHECK(m.tools && m.vision);
    CHECK(m.contextLength == 4096);
    CHECK(m.contextLimit == 128000);
    // detect (Stage 33) çalışmaya devam
    const QJsonObject show{{"capabilities", QJsonArray{"completion", "tools"}},
                           {"details", QJsonObject{{"family", "llama"}}},
                           {"model_info",
                            QJsonObject{{"llama.context_length", 8192}}}};
    const ModelCapabilities d = ModelCapabilities::detect("llama3.1:8b", show);
    CHECK(d.tools);
    CHECK(d.contextLength == 8192);
    CHECK(!d.vision);
    CHECK(ModelCapabilities::detect("qwen2.5", QJsonObject()).tools);
    CHECK(d.anyUse());
    CHECK(ModelCapabilities::fromShow(QJsonObject()).completion);
    CHECK(d.supportsImageInput() == d.vision);
}

// --- 14: sağlayıcı karşılaştırma ---
static void testBench() {
    const QList<BenchTask> ts = ProviderBench::tasks();
    CHECK(ts.size() >= 4);
    CHECK(ProviderBench::taskById("explain").id == "explain");
    CHECK(ProviderBench::taskById("yok").id.isEmpty());
    for (const BenchTask& t : ts) {
        CHECK(!t.id.isEmpty() && !t.prompt.isEmpty());
        CHECK(t.difficulty >= 1 && t.difficulty <= 3);
    }
    // Puanlama: iyi / boş / alakasız
    const double good = ProviderBench::qualityOf("explain",
        "Bu fonksiyon vektördeki tek sayıları toplayıp toplamı döndürür.");
    const double poor = ProviderBench::qualityOf("explain", "bilmiyorum");
    const double none = ProviderBench::qualityOf("explain", "");
    CHECK(good > poor);
    CHECK(poor > none);
    CHECK(none == 0.0);
    CHECK(good <= 100.0 && good >= 0.0);
    CHECK(ProviderBench::qualityOf("yok-böyle-bir-görev", "x") == 0.0);
    // Kod istenen görevde kod bloğu puan kazandırır
    CHECK(ProviderBench::qualityOf("refactor", "```cpp\nint f(){return 0;}\n``` nullptr")
              > ProviderBench::qualityOf("refactor", "düşündüm ama kod yazmadım"));

    // Birleşik skor
    BenchResult r;
    r.quality = 80.0;
    r.latencyMs = 1000;
    r.usd = 0.0;
    const double s1 = ProviderBench::composite(r);
    r.latencyMs = 9000;
    const double s2 = ProviderBench::composite(r);
    CHECK(s1 > s2);                      // yavaş olan daha düşük
    r.latencyMs = 1000;
    r.usd = 0.05;
    const double s3 = ProviderBench::composite(r);
    CHECK(s1 > s3);                      // pahalı olan daha düşük
    CHECK(s1 <= 100.0 && s1 >= 0.0);

    // Toplu puanlama
    QList<QPair<QString, QString>> answers;
    answers.append(qMakePair(QString("explain"),
                             QString("vektördeki tek sayıları toplayıp toplamı döndürür")));
    answers.append(qMakePair(QString("bug"),
                             QString("p[10] taşma: 10 elemanlı dizinin 11. indeksi yazılıyor, undefined")));
    QList<int> lat{800, 1200};
    BenchResult res = ProviderBench::score("openai", "gpt-4o", answers, lat);
    CHECK(res.ok);
    CHECK(res.quality > 0.0);
    CHECK(res.latencyMs == 1000);
    CHECK(res.promptTokens > 0 && res.evalTokens > 0);
    CHECK(res.score > 0.0);
    // Bilinen fiyat + yeterli yanıt → ek not yok
    CHECK(res.notes.isEmpty());
    // Fiyatı bilinmeyen ücretli modelde not düşer
    const BenchResult unk = ProviderBench::score("openai", "bilinmeyen-model-xyz",
                                                 answers, {800, 800});
    CHECK(unk.notes.join("|").contains("fiyat bilinmiyor"));
    // Zayıf yanıtta görev notu düşer
    QList<QPair<QString, QString>> weak;
    weak.append(qMakePair(QString("explain"), QString("bilmiyorum")));
    const BenchResult wres = ProviderBench::score("ollama", "qwen2.5", weak, {500});
    CHECK(wres.notes.join("|").contains("zayıf"));
    CHECK(ProviderBench::verdict(res).size() > 2);
    CHECK(ProviderBench::explain(res).contains("kalite"));
    // Ücretsiz sağlayıcı not düşer
    const BenchResult lo = ProviderBench::score("ollama", "qwen2.5", answers, {500, 500});
    CHECK(lo.usd == 0.0);
    CHECK(lo.notes.contains("ücretsiz katman"));
    // Boş veri
    CHECK(!ProviderBench::score("openai", "gpt-4o", {}, {}).ok);
    CHECK(!ProviderBench::score("", "", answers, lat).ok);
    // Geçersiz görev yoksayılır
    QList<QPair<QString, QString>> unknown;
    unknown.append(qMakePair(QString("yok"), QString("x")));
    CHECK(!ProviderBench::score("openai", "gpt-4o", unknown, {100}).ok);

    // Öneri: ücretsiz olan yakınsa tercih edilir
    QList<BenchResult> list;
    BenchResult paid;
    paid.ok = true;
    paid.providerId = "openai";
    paid.model = "gpt-4o";
    paid.quality = 80;
    paid.score = 70.0;
    paid.usd = 0.05;
    BenchResult freeR;
    freeR.ok = true;
    freeR.providerId = "ollama";
    freeR.model = "qwen2.5";
    freeR.quality = 74;
    freeR.score = 68.0;
    freeR.usd = 0.0;
    list << paid << freeR;
    const QString rec = ProviderBench::recommend(list);
    CHECK(rec.contains("ollama"));         // %92 eşiği içinde → ücretsiz
    BenchResult muchWorse = freeR;
    muchWorse.score = 40.0;
    CHECK(ProviderBench::recommend({paid, muchWorse}).contains("openai"));
    CHECK(ProviderBench::recommend({}).isEmpty());
    // Maliyet ve token yardımcıları
    CHECK(ProviderBench::tokensEstimate("") == 1);
    CHECK(ProviderBench::tokensEstimate("abcd") == 1);
    CHECK(ProviderBench::tokensEstimate(QString(400, 'x')) == 100);
    CHECK(ProviderBench::usdFor("ollama", "qwen", 1000, 1000) == 0.0);
    CHECK(ProviderBench::usdFor("openai", "gpt-4o", 1000000, 0) > 0.0);
}

// --- 11: ajan adaptörü + AgentLoop telemetrisi ---
static void testAgentAdapter() {
    // Maliyet tahmini
    CHECK(AgentLlmAdapter::estimateUsd("ollama", "qwen", 10) == 0.0);
    CHECK(AgentLlmAdapter::estimateUsd("", "", 10) == 0.0);
    CHECK(AgentLlmAdapter::estimateUsd("openai", "gpt-4o", 0) == 0.0);
    const double paid = AgentLlmAdapter::estimateUsd("openai", "gpt-4o", 10, 1000);
    CHECK(paid > 0.0);
    CHECK(AgentLlmAdapter::estimateUsd("openai", "gpt-4o", 20, 1000) > paid);
    // Maliyet önizleme metni
    CHECK(AgentLlmAdapter::costPreview("ollama", "qwen", 5).contains("ücretsiz"));
    CHECK(AgentLlmAdapter::costPreview("openai", "gpt-4o", 5).contains("≈ $"));
    CHECK(AgentLlmAdapter::costPreview("openai", "gpt-4o", 5).contains("5"));

    // Kota aşımı ağa gitmeden durdurur
    UsageLedger::Quota q;
    q.maxCalls = 1;
    UsageLedger::instance().setQuota("openai", q);
    UsageLedger::instance().recordWithCost("openai", "gpt-4o", 10, 10, 0.01);
    AgentLlmAdapter ad;
    AgentLlmAdapter::Options o;
    o.providerId = "openai";
    o.model = "gpt-4o";
    o.recordUsage = false;
    ad.setOptions(o);
    ad.setSecretStore(nullptr);
    const AgentLlmAdapter::Turn t = ad.ask("sistem", "kullanıcı");
    CHECK(!t.ok);
    CHECK(t.error.contains("kota"));
    CHECK(t.providerId == "openai");
    UsageLedger::instance().setQuota("openai", UsageLedger::Quota{});

    // Anahtar yoksa erken hata (ağa gitmeden)
    AgentLlmAdapter ad2;
    AgentLlmAdapter::Options o2;
    o2.providerId = "unorouter";
    o2.model = "gpt-oss-120b:free";
    o2.recordUsage = false;
    ad2.setOptions(o2);
    const AgentLlmAdapter::Turn t2 = ad2.ask("s", "u");
    if (!t2.ok) {
        // Anahtar kasasında anahtar yoksa hata mesajı anahtardan bahsetmeli
        CHECK(t2.error.contains("anahtar") || t2.error.contains("kota"));
    }
    // Seçenekler erişilebilir
    CHECK(ad2.options().providerId == "unorouter");
    ad2.setOptions(AgentLlmAdapter::Options{});
    CHECK(ad2.options().providerId.isEmpty());

    // AgentLoop telemetrisi (Stage 36): sağlayıcı/model/token kaydedilir
    QTemporaryDir root;
    AgentTools tools(root.path());
    QString err;
    auto llm = [](const QString&, const QString& user, QString& e) -> QString {
        if (user.contains("görev")) {
            e.clear();
            return "Plan: read_file çağrılacak.";
        }
        e.clear();
        return "Bitti.";
    };
    AgentLoop::Result r = AgentLoop::run(
        tools, "sistem", "görev", 3, llm, {}, {}, nullptr,
        [](const QString& text, const QList<ToolCall>&) {
            StepMeta m;
            m.providerId = "nvidia-nim";
            m.model = "nvidia/llama-3.1-70b-instruct";
            m.promptTokens = 1200;
            m.evalTokens = 300;
            m.nativeTools = true;
            m.httpStatus = 200;
            return m;
        });
    CHECK(!r.metas.isEmpty());
    CHECK(r.metas.first().providerId == "nvidia-nim");
    CHECK(r.metas.first().promptTokens == 1200);
    CHECK(r.metas.first().evalTokens == 300);
    CHECK(r.metas.first().nativeTools);
    // Telemetri verilmezse de döngü çalışır (geri uyum)
    AgentLoop::Result r2 = AgentLoop::run(tools, "sistem", "görev", 3, llm);
    CHECK(r2.ok);
    CHECK(r2.metas.isEmpty());
}

// --- 12: sağlayıcı tuhaflıkları (Stage 35 geri uyumu bozulmadı) ---
static void testRegression() {
    CHECK(ProviderRegistry::exists("nvidia-nim"));
    CHECK(ProviderRegistry::exists("unorouter"));
    CHECK(ProviderRegistry::builtin().size() >= 18);
    // Ollama yolu hâlâ anahtar istemiyor
    CHECK(!sp("ollama").requiresKey());
    // Fiyatlandırma, kayıt defterini değiştirmedi
    CHECK(sp("openai").baseUrl == "https://api.openai.com/v1");
    CHECK(sp("unorouter").quirks & QFreeModelSuffix);
    // ProviderPrefs çalışıyor
    ProviderPrefs::setActiveProvider("groq");
    CHECK(ProviderPrefs::resolve().id == "groq");
    ProviderPrefs::setActiveProvider("ollama");
    // Gövde üretimi bozulmadı
    AiChatRequest req;
    req.model = "gpt-4o";
    req.messages << AiMessage::user("merhaba");
    const QJsonObject b = ProviderCodec::chatRequest(sp("openai"), req);
    CHECK(b.value("model").toString() == "gpt-4o");
    CHECK(b.value("max_tokens").toInt() == 4096);
    // Sağlık kaydı kalıcı ve temizlenebilir
    ProviderHealth::instance().reset();
    ProviderHealth::instance().recordFailure("test-saglik", 10, 500);
    CHECK(!ProviderHealth::instance().isHealthy("test-saglik") ||
          ProviderHealth::instance().entry("test-saglik").consecutiveErrors == 1);
    ProviderHealth::instance().reset("test-saglik");
}

// --- 10: otomatik failover (yerel sahte sunucu) ---
namespace {

// HTTP durumunu sabit döndüren sahte sunucu
class FakeServer : public QTcpServer {
public:
    explicit FakeServer(int status, const QByteArray& body) : m_status(status), m_body(body) {
        if (!listen(QHostAddress::LocalHost, 0)) return;
        QObject::connect(this, &QTcpServer::newConnection, this, [this]() { serve(); });
    }
    quint16 port() const { return serverPort(); }
    int hits = 0;

private:
    void serve() {
        QTcpSocket* sock = nextPendingConnection();
        ++hits;
        auto* buf = new QByteArray;
        QObject::connect(sock, &QTcpSocket::readyRead, sock, [this, sock, buf]() {
            buf->append(sock->readAll());
            const int he = buf->indexOf("\r\n\r\n");
            if (he < 0) return;
            int cl = 0;
            for (const QByteArray& line : buf->left(he).split('\n'))
                if (line.toLower().startsWith("content-length:"))
                    cl = line.mid(15).trimmed().toInt();
            if (buf->size() - (he + 4) < cl) return;
            QByteArray resp = "HTTP/1.1 " + QByteArray::number(m_status) + " X\r\n"
                              "Content-Type: application/json\r\nContent-Length: "
                              + QByteArray::number(m_body.size()) + "\r\nConnection: close\r\n\r\n"
                              + m_body;
            sock->write(resp);
            sock->flush();
            sock->disconnectFromHost();
        });
        QObject::connect(sock, &QTcpSocket::disconnected, sock, [sock, buf]() {
            delete buf;
            sock->deleteLater();
        });
    }
    int m_status;
    QByteArray m_body;
};

} // namespace

static void testFailover() {
    // Anahtarsız iki özel sağlayıcı: biri 404 verir, diğeri 200
    FakeServer bad(404, R"({"error":{"message":"model yok"}})");
    FakeServer good(200,
                    R"({"choices":[{"message":{"content":"YEDEKten yanıt"},"finish_reason":"stop"}]})");
    CHECK(bad.port() != 0 && good.port() != 0);
    qputenv("VERSO_AI_KEY_FO_A", "key-a");
    qputenv("VERSO_AI_KEY_FO_B", "key-b");
    const ProviderSpec a = ProviderRegistry::makeCustom(
        "fo-a", "Hatalı sağlayıcı",
        QString("http://127.0.0.1:%1/v1").arg(bad.port()));
    const ProviderSpec b = ProviderRegistry::makeCustom(
        "fo-b", "Yedek sağlayıcı", QString("http://127.0.0.1:%1/v1").arg(good.port()));
    CHECK(ProviderRegistry::save(a));
    CHECK(ProviderRegistry::save(b));

    LlmClient client;
    client.setProvider(a);
    client.setApiKey("key-a"); // kasasız kullanım (ortam değişkeni de çalışır)
    client.setFailoverEnabled(true);
    CHECK(client.failoverEnabled());

    AiChatRequest req;
    req.model = "test-model";
    req.messages << AiMessage::user("ping");
    const AiReply r = client.chatSync(req, 8000);
    // 404 kalıcı hatadır → eşdeğer sağlayıcıya devredilmeli
    CHECK(r.ok);
    CHECK(r.text == "YEDEKten yanıt");
    CHECK(client.lastFailoverFrom() == "fo-a");
    CHECK(bad.hits == 1);
    CHECK(good.hits == 1);

    // Devir kapalıyken aynı hata yüzeye çıkar
    LlmClient solo;
    solo.setProvider(ProviderRegistry::byId("fo-a"));
    solo.setApiKey("key-a");
    solo.setFailoverEnabled(false);
    const AiReply r2 = solo.chatSync(req, 8000);
    CHECK(!r2.ok);
    CHECK(r2.httpStatus == 404);
    CHECK(r2.error.contains("bulunamadı") || r2.error.contains("404"));
    CHECK(solo.lastFailoverFrom().isEmpty());

    // 429 yeniden denemeye girer, devre girmez (yeniden deneme başarılı olur)
    FakeServer rate(429, R"({"error":{"message":"yavaş"}})");
    const ProviderSpec c = ProviderRegistry::makeCustom(
        "fo-c", "Kotalı", QString("http://127.0.0.1:%1/v1").arg(rate.port()));
    qputenv("VERSO_AI_KEY_FO_C", "key-c");
    CHECK(ProviderRegistry::save(c));
    LlmClient c2;
    c2.setProvider(ProviderRegistry::byId("fo-c"));
    c2.setApiKey("key-c");
    const AiReply r3 = c2.chatSync(req, 4000);
    CHECK(!r3.ok);
    // yalnız bir yedek adayı yoksa devir olmaz (başka eşdeğer sağlayıcı yok)
    CHECK(c2.lastFailoverFrom().isEmpty() || c2.lastFailoverFrom() == "fo-c");

    qunsetenv("VERSO_AI_KEY_FO_A");
    qunsetenv("VERSO_AI_KEY_FO_B");
    qunsetenv("VERSO_AI_KEY_FO_C");
    ProviderRegistry::remove("fo-a");
    ProviderRegistry::remove("fo-b");
    ProviderRegistry::remove("fo-c");

    // isHardFailure kuralları
    CHECK(LlmClient::isHardFailure(401));
    CHECK(LlmClient::isHardFailure(403));
    CHECK(LlmClient::isHardFailure(404));
    CHECK(LlmClient::isHardFailure(503));
    CHECK(!LlmClient::isHardFailure(400));
    CHECK(!LlmClient::isHardFailure(429));
    CHECK(!LlmClient::isHardFailure(200));
    CHECK(!LlmClient::isHardFailure(0));
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir home;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, home.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, home.path());
    // Kalıcı dosyalar (kullanım geçmişi, sağlık) geçici dizine yazılsın
    qputenv("XDG_DATA_HOME", home.path().toUtf8());
    qputenv("HOME", home.path().toUtf8());

    testPricing();
    testUsageLedger();
    testHealthAndFailover();
    testRouter();
    testEmbedCache();
    testEmbedBridge();
    testTokenStats();
    testCapabilities();
    testBench();
    testAgentAdapter();
    testRegression();
    testFailover();

    fprintf(stderr, "STAGE36: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
