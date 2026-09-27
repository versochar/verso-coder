// Stage 37 testleri: tek AI cephesi (AiRunner), görev profilleri, istem
// görev ipuçları, hayalet tamamlama modları, görsel kararı, arena hedef havuzu
// ve "her yüzey sağlayıcıya gider" davranışı (yerel sahte sunucu).
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonObject>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <cmath>
#include <cstdio>

#include "../src/core/AgentTools.h"
#include "../src/core/ConversationSummarizer.h"
#include "../src/core/GhostCompletion.h"
#include "../src/core/ModelCapabilities.h"
#include "../src/core/PromptLibrary.h"
#include "../src/core/ai/AiProfiles.h"
#include "../src/core/ai/AiRunner.h"
#include "../src/core/ai/LlmClient.h"
#include "../src/core/ai/LlmProvider.h"
#include "../src/core/ai/ProviderHealth.h"
#include "../src/core/ai/ProviderPrefs.h"
#include "../src/core/ai/SecretStore.h"
#include "../src/core/ai/TaskRouter.h"
#include "../src/core/ai/UsageLedger.h"
#include "../src/widgets/ModelArenaDialog.h"

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

// CHECK2: görev adıyla raporlayan varyant
#define CHECK2(cond, task)                                                       \
    do {                                                                         \
        if (cond) {                                                              \
            ++g_pass;                                                            \
        } else {                                                                  \
            ++g_fail;                                                            \
            fprintf(stderr, "FAIL %d %s [%s]\n", __LINE__, #cond,                \
                    qPrintable(AiProfiles::label(task)));                        \
            fflush(stderr);                                                      \
        }                                                                        \
    } while (0)

// --- 1–2: görev profilleri ---
static void testProfiles() {
    // Her görevin geçerli bir profili var
    for (int i = 0; i <= int(AiTask::Extract); ++i) {
        const AiProfiles::Profile p = AiProfiles::profile(AiTask(i));
        CHECK(p.timeoutMs > 0);
        CHECK(p.temperature >= -1.0 && p.temperature <= 2.0); // -1 = kullanıcı ayarı
        CHECK(p.maxTokens >= 0);
        CHECK(!AiProfiles::label(AiTask(i)).isEmpty());
    }
    // Göreve özgü beklentiler
    CHECK(AiProfiles::profile(AiTask::Fim).timeoutMs <= 5000);   // hayalet hızlı olmalı
    CHECK(AiProfiles::profile(AiTask::Fim).maxTokens <= 256);
    CHECK(AiProfiles::profile(AiTask::Commit).maxTokens <= 1024); // kısa çıktı
    CHECK(AiProfiles::profile(AiTask::Summarize).streaming == false);
    CHECK(!AiProfiles::profile(AiTask::Summarize).systemSuffix.isEmpty());
    CHECK(AiProfiles::profile(AiTask::Vision).hint == TaskClass::Vision);
    CHECK(AiProfiles::profile(AiTask::Test).hint == TaskClass::Medium);
    CHECK(AiProfiles::profile(AiTask::Theme).systemSuffix.contains("JSON"));
    // Sistem tonu ekleme
    CHECK(AiProfiles::withSuffix(QString(), AiTask::Explain).length() > 0);
    CHECK(AiProfiles::withSuffix("Kısa yaz.", AiTask::Explain).startsWith("Kısa yaz."));
    CHECK(AiProfiles::withSuffix("Kısa yaz.", AiTask::Explain).length() > 12);
    // Süreç tonu yoksa dokunulmaz
    CHECK(AiProfiles::withSuffix("Sadece bu.", AiTask::Chat) == "Sadece bu.");
    CHECK(AiProfiles::allLabels().size() == int(AiTask::Extract) + 1);
}

// --- 3: seçenek üretimi + plan (ağ YOK) ---
static void testPlan() {
    ProviderPrefs::reset();
    TaskRouter::resetPrefs();
    ProviderPrefs::setActiveProvider("nvidia-nim");
    ProviderPrefs::setModel("nvidia-nim", "nvidia/llama-3.1-70b-instruct");

    const AiRunner::Options o = AiRunner::optionsFor(AiTask::Review, "Titiz inceleme.");
    CHECK(o.task == AiTask::Review);
    CHECK(o.systemPrompt.contains("Titiz inceleme."));
    CHECK(o.systemPrompt.contains("Maddeli", Qt::CaseInsensitive));
    CHECK(o.temperature == AiProfiles::profile(AiTask::Review).temperature);
    CHECK(o.maxTokens == AiProfiles::profile(AiTask::Review).maxTokens);
    CHECK(o.timeoutMs == AiProfiles::profile(AiTask::Review).timeoutMs);

    // Yönlendirme kapalı olsa bile AiRunner yönlendirir (her zaman geçerli sağlayıcı)
    const AiRunner::Plan p = AiRunner::planFor(o);
    CHECK(p.ok);
    CHECK(!p.providerId.isEmpty());
    CHECK(!p.model.isEmpty());
    CHECK(!p.reason.isEmpty());
    CHECK(p.cls != TaskClass::Embed);

    // Verilen sağlayıcı/model kilitler
    AiRunner::Options forced = o;
    forced.providerId = "openai";
    forced.model = "gpt-4o";
    const AiRunner::Plan fp = AiRunner::planFor(forced);
    CHECK(fp.providerId == "openai");
    CHECK(fp.model == "gpt-4o");
    CHECK(fp.forceLocked);

    // Bypass: model verilmiş ama sağlayıcı yok → etkin sağlayıcı
    AiRunner::Options modelOnly = o;
    modelOnly.model = "gpt-4o-mini";
    const AiRunner::Plan mp = AiRunner::planFor(modelOnly);
    CHECK(mp.providerId == "nvidia-nim");
    CHECK(mp.model == "gpt-4o-mini");

    // Bypass + sağlayıcı
    AiRunner::Options both = o;
    both.bypassRouting = true;
    both.providerId = "groq";
    both.model = "";
    const AiRunner::Plan bp = AiRunner::planFor(both);
    CHECK(bp.providerId == "groq");
    // Bulunmayan sağlayıcı → plan başarısız
    AiRunner::Options bad = o;
    bad.bypassRouting = true;
    bad.providerId = "yok-boyle-bir-sayici";
    CHECK(!AiRunner::planFor(bad).ok);
    // resolve() = planFor()
    CHECK(AiRunner::resolve(o).providerId == p.providerId);
    ProviderPrefs::reset();
}

// --- 8: görsel kararı ---
static void testVisionDecision() {
    // Görsel yoksa her şey geçerli
    CHECK(AiRunner::canRunWithImages("openai", "gpt-4o", 0));
    CHECK(AiRunner::imageBlockReason("openai", 0).isEmpty());
    // Görsel destekleyen
    CHECK(AiRunner::canRunWithImages("openai", "gpt-4o", 1));
    CHECK(AiRunner::canRunWithImages("nvidia-nim", "nvidia/llama-3.1-70b-vision", 2));
    // Görsel desteklemeyen (özel sağlayıcı: araç var, görü yok)
    ProviderSpec noVis = ProviderRegistry::makeCustom("s37-gorselsiz", "Görselsiz",
                                                      "http://127.0.0.1:1/v1");
    noVis.supportsVision = false;
    CHECK(ProviderRegistry::save(noVis));
    CHECK(!AiRunner::canRunWithImages("s37-gorselsiz", "some-model", 1));
    const QString why = AiRunner::imageBlockReason("s37-gorselsiz", 3);
    CHECK(why.contains("görsel"));
    CHECK(why.contains("3"));
    ProviderRegistry::remove("s37-gorselsiz");
    // Gömme modeli görsel kabul etmez
    CHECK(!AiRunner::canRunWithImages("openai", "text-embedding-3-small", 1));
    // Base64 temizleme
    const QStringList dirty{"data:image/png;base64,AAAA", "BBBB\n CCCC ", "", "  "};
    const QStringList clean = AiRunner::cleanBase64(dirty);
    CHECK(clean.size() == 2);
    CHECK(clean[0] == "AAAA");
    CHECK(clean[1] == "BBBBCCCC");
    const auto imgs = AiRunner::imagesFromBase64({"AAAA"}, "image/png");
    CHECK(imgs.size() == 1);
    CHECK(imgs.first().mime == "image/png");
    CHECK(imgs.first().bytes == QByteArray::fromBase64("AAAA"));
    // Bozuk base64 elenir
    CHECK(AiRunner::imagesFromBase64({"!!!"}).isEmpty());
}

// --- 4: hayalet tamamlama modları ---
static void testGhostModes() {
    // Yerel → FIM
    CHECK(GhostCompletion::modeFor("ollama", false) == GhostCompletion::Mode::Fim);
    CHECK(GhostCompletion::modeFor("lmstudio", true) == GhostCompletion::Mode::Fim);
    CHECK(GhostCompletion::modeFor("", false) == GhostCompletion::Mode::Fim);
    // Bulut + kapalı → Off
    CHECK(GhostCompletion::modeFor("openai", false) == GhostCompletion::Mode::Off);
    CHECK(GhostCompletion::modeFor("nvidia-nim", false) == GhostCompletion::Mode::Off);
    // Bulut + açık → Prefix
    CHECK(GhostCompletion::modeFor("openai", true) == GhostCompletion::Mode::Prefix);
    CHECK(GhostCompletion::modeFor("unorouter", true) == GhostCompletion::Mode::Prefix);
    CHECK(GhostCompletion::wantsSuffix("ollama", false));
    CHECK(!GhostCompletion::wantsSuffix("openai", true));
    // Etiketler
    CHECK(GhostCompletion::modeLabel(GhostCompletion::Mode::Fim).contains("FIM"));
    CHECK(GhostCompletion::modeLabel(GhostCompletion::Mode::Off).contains("kapalı"));
    // FIM istemi soneki kullanır, prefix istemi kullanmaz
    const QString prefix = "int a = 1;\nint b = 2;\n";
    const QString suffix = "int c = 3;";
    const QString fim = GhostCompletion::buildPrompt(GhostCompletion::Mode::Fim, prefix, suffix, "cpp");
    const QString pre = GhostCompletion::buildPrompt(GhostCompletion::Mode::Prefix, prefix, suffix, "cpp");
    CHECK(fim.contains("cpp"));
    CHECK(fim != pre);
    CHECK(pre.contains("devam") || pre.contains("kod"));
    // Önek istemi kısa tutulur (40 satır sınırı)
    QString big;
    for (int i = 0; i < 200; ++i) big += QStringLiteral("satır%1\n").arg(i);
    const QString preBig = GhostCompletion::buildPrompt(GhostCompletion::Mode::Prefix, big, QString(), "cpp");
    CHECK(preBig.size() < big.size());
    // Mevcut temizleme kuralları korunur
    CHECK(!GhostCompletion::clean("```cpp\nint x = 1;\n```", "int x = ").isEmpty());
    CHECK(GhostCompletion::clean("", "x").isEmpty());
}

// --- 9: istem görev ipuçları ---
static void testPromptHints() {
    CHECK(PromptLibrary::hasTaskHint("!test Şunu test et"));
    CHECK(!PromptLibrary::hasTaskHint("Şunu test et"));
    CHECK(!PromptLibrary::hasTaskHint("!bilinmeyen Şunu yap"));
    CHECK(!PromptLibrary::hasTaskHint("!  boş"));
    CHECK(PromptLibrary::taskHint("!test Şunu test et") == "test");
    CHECK(PromptLibrary::taskHint("!COMMIT mesaj") == "commit");
    CHECK(PromptLibrary::taskHint("iptal et") == "");
    CHECK(PromptLibrary::stripTaskHint("!test Şunu test et") == "Şunu test et");
    CHECK(PromptLibrary::stripTaskHint("!review iki yaz") == "iki yaz");
    CHECK(PromptLibrary::stripTaskHint("normal metin") == "normal metin");
    // Yalnız ipucu olan metin boş gövdeye düşer
    CHECK(PromptLibrary::stripTaskHint("!test").isEmpty());
    CHECK(PromptLibrary::knownTaskHints().contains("explain"));
    CHECK(PromptLibrary::knownTaskHints().contains("theme"));
    CHECK(PromptLibrary::isKnownTaskHint("TEST"));
    CHECK(!PromptLibrary::isKnownTaskHint("yok"));
    // Her ipucu gerçek bir göreve eşlenebilmeli
    for (const QString& h : PromptLibrary::knownTaskHints()) {
        AiTask t = AiTask::Chat;
        if (h == "explain") t = AiTask::Explain;
        else if (h == "review") t = AiTask::Review;
        else if (h == "test") t = AiTask::Test;
        else if (h == "doc") t = AiTask::Doc;
        else if (h == "commit") t = AiTask::Commit;
        else if (h == "summarize") t = AiTask::Summarize;
        else if (h == "theme") t = AiTask::Theme;
        else if (h == "vision") t = AiTask::Vision;
        CHECK(!AiProfiles::label(t).isEmpty());
    }
}

// --- 6: arena hedef havuzu ---
static void testArenaTargets() {
    ProviderPrefs::reset();
    SecretStore store;
    store.setUseKeyring(false);
    // Yerel modeller etkin sağlayıcıya bağlanır
    const QList<ArenaTarget> t1 =
        ModelArenaDialog::defaultTargets({"qwen2.5:1.5b", "llava:7b"}, "ollama");
    CHECK(t1.size() >= 2);
    CHECK(t1.first().providerId == "ollama");
    CHECK(t1.first().model == "qwen2.5:1.5b");
    // Anahtarı olan bulut sağlayıcı da havuza girer
    // Anahtar ortam değişkeniyle: hem kasa yolu hem env yolu bunu okur
    qputenv("VERSO_AI_KEY_GROQ", "test-key");
    const QList<ArenaTarget> t2 =
        ModelArenaDialog::defaultTargets({"qwen2.5:1.5b"}, "ollama");
    bool hasGroq = false;
    for (const ArenaTarget& t : t2)
        if (t.providerId == "groq") hasGroq = true;
    CHECK(hasGroq);
    // Etiket okunabilir
    CHECK(!t2.first().label().isEmpty());
    // Sınır uygulanır
    const QList<ArenaTarget> t3 = ModelArenaDialog::defaultTargets(
        {"a", "b", "c", "d", "e", "f", "g", "h"}, "ollama", 3);
    CHECK(t3.size() <= 3);
    // Aynı hedef iki kez eklenmez
    bool dup = false;
    for (int i = 0; i < t2.size(); ++i)
        for (int j = i + 1; j < t2.size(); ++j)
            if (t2.at(i).providerId == t2.at(j).providerId &&
                t2.at(i).model == t2.at(j).model)
                dup = true;
    CHECK(!dup);
    // Gömme MODELİ arena havuzuna girmez (sağlayıcı gömme yapsa da)
    CHECK(ModelCapabilities::isEmbeddingModel("text-embedding-3-small"));
    CHECK(ModelCapabilities::isEmbeddingModel("nvidia/nv-embedqa-e5-v5"));
    CHECK(!ModelCapabilities::isEmbeddingModel("gpt-4o"));
    CHECK(!ModelCapabilities::isEmbeddingModel("llama-3.3-70b-versatile"));
    ProviderPrefs::setModel("openai", "text-embedding-3-small");
    ProviderPrefs::setModel("openai", "");
    // Gömme MODELİ arena havuzuna girmez (sağlayıcı gömme desteklese de)
    for (const ArenaTarget& t : t2) CHECK(!ModelCapabilities::isEmbeddingModel(t.model));
    qunsetenv("VERSO_AI_KEY_GROQ");
}

// --- 11: ortak kuyruk durumu ---
static void testQueue() {
    CHECK(!AiRunner::busy());
    CHECK(AiRunner::activeCount() == 0);
    AiRunner r;
    CHECK(!r.hasActive());
    // İptal edilebilir aktif istek yoksa false
    CHECK(!r.cancelActive());
    // Plan başarısızken run hata döner, kuyruğa girmez
    AiRunner::Options bad = AiRunner::optionsFor(AiTask::Chat);
    bad.bypassRouting = true;
    bad.providerId = "yok-boyle-bir-sayici";
    const AiRunner::Result res = r.run(bad, "merhaba");
    CHECK(!res.ok);
    CHECK(!res.error.isEmpty());
    CHECK(!AiRunner::busy());
    // Geçici işaretleme
    ProviderPrefs::setActiveProvider("openai");
    const int n = AiRunner::activeCount();
    CHECK(n == 0);
    ProviderPrefs::reset();
}

// --- 10: özet yardımcıları (prompt üretimi) ---
static void testSummary() {
    QList<ConvTurn> turns;
    for (int i = 0; i < 12; ++i) turns << ConvTurn{i % 2 ? "ai" : "user", QString("tur %1").arg(i)};
    CHECK(ConversationSummarizer::needed(turns, QString(), 5, 6));
    CHECK(!ConversationSummarizer::needed(turns, QString(), 100000, 6));
    CHECK(ConversationSummarizer::olderTurns(turns, 6).size() == 6);
    const QString p = ConversationSummarizer::summarizePrompt(turns, 4000);
    CHECK(p.contains("tur 0"));
    CHECK(ConversationSummarizer::merge("özet", turns, 2000).contains("özet"));
}

static ProviderSpec spec37() { return ProviderRegistry::byId("unorouter"); }

// --- 11: gerçek sağlayıcı davranışından çıkan hata sınıflandırması ---
// UnoRouter/NIM gibi ağgeçitler hız sınırını 429 değil 403 olarak ve
// "per minute limit / try again in a minute" diyerek bildirir. Bu GEÇİCİDİR:
// istek sağlamdır, kaynağın kendisi yoğundur. Yanlışlıkla failover
// tetiklenmesin ve saniyelik geri çekilme yerine makul beklenmelidir.
static void testRealProviderErrorShape() {
    const QByteArray rl =
        R"({"error":{"message":"This model is at its provider's per minute limit right now. Nothing is used up on your side. Try again in a minute.","type":"bad_response_status_code"}})";
    // Hız sınırı tanınır (403 + 429 + 5xx ipuçlu)
    CHECK(LlmClient::isRateLimited(429, QByteArray()));
    CHECK(LlmClient::isRateLimited(403, rl));
    CHECK(LlmClient::isRateLimited(503, "quota exceeded"));
    CHECK(LlmClient::isRateLimited(502, "rate_limit"));
    // Hız sınırı olmayan 403 hâlâ yetki hatasıdır
    CHECK(!LlmClient::isRateLimited(403, R"({"error":{"message":"Invalid API key"}})"));
    CHECK(!LlmClient::isRateLimited(403, QByteArray()));
    // Yeniden denemeye girer
    CHECK(LlmClient::isRetryable(429));
    CHECK(LlmClient::isRetryable(403, rl));
    CHECK(LlmClient::isRetryable(503, rl));
    // ...ama failover TETİKLEMEZ (kalıcı hata değil)
    CHECK(!LlmClient::isHardFailure(403, rl));
    CHECK(!LlmClient::isHardFailure(429));
    CHECK(!LlmClient::isHardFailure(503, rl));
    // Gerçek kalıcı hatalar failover eder
    CHECK(LlmClient::isHardFailure(401, QByteArray()));
    CHECK(LlmClient::isHardFailure(403, R"({"error":{"message":"forbidden"}})"));
    CHECK(LlmClient::isHardFailure(404, QByteArray()));
    CHECK(LlmClient::isHardFailure(500));
    // Diğer durumlar
    CHECK(LlmClient::isRetryable(408));
    CHECK(!LlmClient::isRetryable(400));
    CHECK(!LlmClient::isRetryable(401));
    CHECK(!LlmClient::isRetryable(404));
    // Hız sınırı mesajı "anahtar hatalı" gibi gösterilmez
    const QString rlMsg = ProviderRegistry::mapError(spec37(), 403, rl);
    if (!rlMsg.contains("Hız sınırı")) fprintf(stderr, "  (403 hız sınırı msg=%s)\n", qPrintable(rlMsg));
    CHECK(rlMsg.contains("Hız sınırı"));
    CHECK(!rlMsg.contains("anahtar"));
    CHECK(ProviderRegistry::mapError(spec37(), 429, QByteArray()).contains("429"));
    // Gerçek yetki hatası yine yetki hatasıdır
    const QString keyMsg =
        ProviderRegistry::mapError(spec37(), 403, R"({"error":{"message":"forbidden"}})");
    CHECK(keyMsg.contains("yetkisiz") || keyMsg.contains("anahtar"));

    // Çok uzun Retry-After (ör. ":free" katmanı 1 istek / 30 dk) → yeniden deneme
    CHECK(!LlmClient::retryAfterTooLong("44"));
    CHECK(!LlmClient::retryAfterTooLong("120"));
    CHECK(LlmClient::retryAfterTooLong("1628"));
    CHECK(LlmClient::retryAfterTooLong("3600"));
    CHECK(!LlmClient::retryAfterTooLong(QByteArray()));
    CHECK(!LlmClient::retryAfterTooLong("abc"));
    CHECK(LlmClient::maxRetryAfterSec() == 120);

    // Hız sınırında bekleme süresi üstel geri çekilmeyi aşar
    const int rl0 = LlmClient::retryDelayMs(0, 403, QByteArray(), rl);
    CHECK(rl0 >= 20000);
    CHECK(LlmClient::retryDelayMs(1, 403, QByteArray(), rl) > rl0);
    CHECK(LlmClient::rateLimitDelayMs(9) <= 60000);
    // Retry-After varsa o baskın (UnoRouter 429'da retry-after: 44 gönderiyor)
    CHECK(LlmClient::retryDelayMs(0, 429, "44", rl) == 44000);
    // Retry-After yoksa 429 da hız sınırı sayılır
    CHECK(LlmClient::retryDelayMs(0, 429, QByteArray()) >= 20000);
    // Hız sınırı olmayan 5xx kısa üstel geri çekilmeyi korur
    const int s5 = LlmClient::retryDelayMs(0, 500);
    CHECK(s5 < 20000);
}

// --- 15: yerel sahte sunucu — her görev etkin sağlayıcıya gider ---
namespace {

class EchoServer : public QTcpServer {
public:
    bool start() {
        if (!listen(QHostAddress::LocalHost, 0)) return false;
        connect(this, &QTcpServer::newConnection, this, [this]() { serve(); });
        return true;
    }
    int hits = 0;
    QList<QJsonObject> bodies;

private:
    void serve() {
        QTcpSocket* sock = nextPendingConnection();
        ++hits;
        auto* buf = new QByteArray;
        connect(sock, &QTcpSocket::readyRead, sock, [this, sock, buf]() {
            buf->append(sock->readAll());
            const int he = buf->indexOf("\r\n\r\n");
            if (he < 0) return;
            int cl = 0;
            for (const QByteArray& line : buf->left(he).split('\n'))
                if (line.toLower().startsWith("content-length:"))
                    cl = line.mid(15).trimmed().toInt();
            if (buf->size() - (he + 4) < cl) return;
            bodies.append(QJsonDocument::fromJson(buf->mid(he + 4, cl)).object());
            const QByteArray body = R"({"choices":[{"message":{"content":"OK"},"finish_reason":"stop"}]})";
            const QByteArray resp = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"
                                    "Content-Length: " + QByteArray::number(body.size()) +
                                    "\r\nConnection: close\r\n\r\n" + body;
            sock->write(resp);
            sock->flush();
            sock->disconnectFromHost();
        });
        connect(sock, &QTcpSocket::disconnected, sock, [sock, buf]() {
            delete buf;
            sock->deleteLater();
        });
    }
};

// Bulut sağlayıcısı gibi davranan yerel sağlayıcı (test için)
ProviderSpec makeCloud(const QString& id, quint16 port) {
    ProviderSpec s = ProviderRegistry::makeCustom(
        id, QStringLiteral("Test Bulut"), QString("http://127.0.0.1:%1/v1").arg(port));
    s.supportsVision = true;
    s.supportsEmbed = false;
    return s;
}

} // namespace

static void testAllSurfacesRoute() {
    EchoServer srv;
    CHECK(srv.start());
    const ProviderSpec cloud = makeCloud("s37-cloud", srv.serverPort());
    CHECK(ProviderRegistry::save(cloud));
    ProviderPrefs::setActiveProvider("s37-cloud");
    ProviderPrefs::setModel("s37-cloud", "test-model");
    SecretStore store;
    store.setUseKeyring(false);
    qputenv("VERSO_AI_KEY_S37_CLOUD", "test-key");

    AiRunner runner;
    // Stage 37'in ana iddiası: 12 görevin hepsi aynı cepheyi kullanır ve
    // etkin sağlayıcıya (yerel Ollama değil) gider.
    const QList<AiTask> tasks = {AiTask::Chat,      AiTask::Explain,  AiTask::Review,
                                 AiTask::Test,      AiTask::Doc,      AiTask::Commit,
                                 AiTask::Summarize, AiTask::Fim,      AiTask::Vision,
                                 AiTask::Theme,     AiTask::Arena,    AiTask::Extract};
    const int before = srv.hits;
    for (AiTask t : tasks) {
        AiRunner::Options o = AiRunner::optionsFor(t, "sistem");
        const AiRunner::Result r = runner.run(o, "merhaba dunya");
        CHECK2(r.ok, t);
        CHECK2(r.text == "OK", t);
        CHECK2(r.providerId == "s37-cloud", t);
    }
    // Her görev gerçekten ağa gitti
    CHECK(srv.hits - before == int(tasks.size()));
    // Görsel görevde görsel de gönderilir
    const QList<AiImage> imgs = AiRunner::imagesFromBase64({"QUJD"});
    AiRunner::Options vo = AiRunner::optionsFor(AiTask::Vision);
    const AiRunner::Result vr = runner.run(vo, "ne görüyorsun", imgs);
    CHECK(vr.ok);
    // Sunucu aldığı gövdede içerik parçaları (görsel) olmalı
    bool sawImage = false;
    for (const QJsonObject& b : srv.bodies) {
        const QJsonArray msgs = b.value("messages").toArray();
        for (const QJsonValue& m : msgs) {
            const QJsonValue content = m.toObject().value("content");
            if (content.isArray()) {
                for (const QJsonValue& part : content.toArray())
                    if (part.toObject().contains("image_url")) sawImage = true;
            }
        }
    }
    CHECK(sawImage);
    // Kota tükenince ağa gitmez
    UsageLedger::Quota q;
    q.maxCalls = 1;
    UsageLedger::instance().setQuota("s37-cloud", q);
    const int hitsBefore = srv.hits;
    const AiRunner::Result qres = runner.run(AiRunner::optionsFor(AiTask::Chat), "tekrar");
    CHECK(!qres.ok);
    CHECK(qres.error.contains("kota"));
    CHECK(srv.hits == hitsBefore); // ağa çıkmadı
    UsageLedger::instance().setQuota("s37-cloud", UsageLedger::Quota{});

    qunsetenv("VERSO_AI_KEY_S37_CLOUD");
    ProviderRegistry::remove("s37-cloud");
    ProviderPrefs::reset();
    ProviderHealth::instance().reset();
}

// --- 14: geri uyum ---
static void testRegression() {
    // Ollama yolu duruyor
    CHECK(sp("ollama").chatPath == "/api/chat");
    CHECK(!sp("ollama").requiresKey());
    CHECK(ProviderRegistry::exists("nvidia-nim"));
    CHECK(ProviderRegistry::builtin().size() >= 18);
    // Gövde üreticisi bozulmadı
    AiChatRequest req;
    req.model = "gpt-4o";
    req.messages << AiMessage::user("selam");
    ProviderPrefs::setActiveProvider("ollama");
    ProviderPrefs::setModel("ollama", "qwen2.5-coder:1.5b");
    const AiRunner::Options o = AiRunner::optionsFor(AiTask::Chat, "sistem");
    const AiRunner::Plan p = AiRunner::planFor(o);
    CHECK(p.ok);
    ProviderPrefs::reset();
    // Fiyatlandırma/kasa sağlam
    CHECK(ProviderRegistry::byId("openai").baseUrl == "https://api.openai.com/v1");
    // Dosya sayımı: yeni dosyalar yerinde
    const QString srcDir = QFileInfo(QString::fromLocal8Bit(__FILE__)).absolutePath();
    CHECK(QFile::exists(srcDir + QStringLiteral("/../src/core/ai/AiRunner.cpp")));
    CHECK(QFile::exists(srcDir + QStringLiteral("/../src/core/ai/AiProfiles.h")));
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir home;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, home.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, home.path());
    qputenv("XDG_DATA_HOME", home.path().toUtf8());
    qputenv("HOME", home.path().toUtf8());

    testProfiles();
    testPlan();
    testVisionDecision();
    testGhostModes();
    testPromptHints();
    testArenaTargets();
    testQueue();
    testRealProviderErrorShape();
    testSummary();
    testAllSurfacesRoute();
    testRegression();

    fprintf(stderr, "STAGE37: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
