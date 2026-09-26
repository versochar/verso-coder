// Stage 33 testleri: yetenek algılama, görsel hazırlık, gömme, hibrit sıralama,
// yanıt puanı, sohbet özeti ve sohbet dallanma ağacı.
#include <QCoreApplication>
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QTemporaryDir>
#include <cstdio>

#include "../src/core/ChatStore.h"
#include "../src/core/ConversationSummarizer.h"
#include "../src/core/Embedding.h"
#include "../src/core/EmbeddingClient.h"
#include "../src/core/HybridRanker.h"
#include "../src/core/ImageUtil.h"
#include "../src/core/ModelCapabilities.h"
#include "../src/core/ResponseScorer.h"

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

static void testCapabilities() {
    auto c = ModelCapabilities::fromName("llava:13b");
    CHECK(c.vision);
    auto e = ModelCapabilities::fromName("nomic-embed-text");
    CHECK(e.embedding && !e.completion);
    auto t = ModelCapabilities::fromName("qwen2.5-coder:7b");
    CHECK(t.tools);

    QJsonObject show;
    show["capabilities"] = QJsonArray{"completion", "tools", "vision"};
    show["details"] = QJsonObject{{"family", "llama"}};
    show["model_info"] = QJsonObject{{"llama.context_length", 8192}};
    auto sc = ModelCapabilities::fromShow(show);
    CHECK(sc.completion && sc.tools && sc.vision && !sc.embedding);
    CHECK(sc.contextLength == 8192);
    CHECK(sc.badges().contains("görü"));

    // capabilities yoksa aile adından sez
    QJsonObject show2;
    show2["details"] = QJsonObject{{"family", "llava"}};
    auto sc2 = ModelCapabilities::detect("", show2);
    CHECK(sc2.vision);
    // show boşsa ad sezgisi
    CHECK(ModelCapabilities::detect("moondream", QJsonObject()).vision);
}

static void testImageUtil() {
    QImage img(2000, 1000, QImage::Format_RGB32);
    img.fill(Qt::red);
    QImage s = ImageUtil::scaled(img, 1024);
    CHECK(s.width() == 1024 && s.height() == 512);
    CHECK(ImageUtil::scaled(img, 4000).size() == img.size()); // küçültme yok
    PreparedImage p = ImageUtil::prepare(img, 512, 70);
    CHECK(p.valid());
    CHECK(p.width == 512 && p.height == 256);
    CHECK(p.mime == "image/jpeg");
    CHECK(ImageUtil::withinBudget(p.bytes, 4 * 1024 * 1024));
    CHECK(!ImageUtil::base64(p.bytes).isEmpty());
    CHECK(ImageUtil::isImagePath("a/b/c.PNG"));
    CHECK(!ImageUtil::isImagePath("a/b/c.cpp"));
    CHECK(!ImageUtil::prepare(QImage(), 100, 80).valid());
}

static void testEmbedding() {
    QList<float> a = {1.0f, 2.0f, 3.0f};
    QList<float> b = Embedding::deserialize(Embedding::serialize(a));
    CHECK(a == b);
    CHECK(Embedding::serialize(a).size() == 12);
    CHECK(qAbs(Embedding::cosine(a, a) - 1.0) < 1e-6);
    CHECK(qAbs(Embedding::cosine(a, QList<float>{-1.0f, -2.0f, -3.0f}) + 1.0) < 1e-6);
    CHECK(qAbs(Embedding::cosine(QList<float>{1, 0}, QList<float>{0, 1})) < 1e-6);
    CHECK(Embedding::cosine(a, QList<float>{1, 2}) == 0.0); // boyut uyuşmaz
    auto n = Embedding::normalize(QList<float>{3, 4});
    CHECK(qAbs(n[0] - 0.6) < 1e-5 && qAbs(n[1] - 0.8) < 1e-5);
    auto avg = Embedding::average({QList<float>{0, 0}, QList<float>{2, 4}});
    CHECK(avg[0] == 1.0f && avg[1] == 2.0f);
    CHECK(Embedding::average({QList<float>{1}, QList<float>{1, 2}}).isEmpty());
}

static void testEmbeddingClient() {
    QJsonObject o;
    o["embeddings"] = QJsonArray{QJsonArray{0.1, 0.2}, QJsonArray{0.3, 0.4, 0.5}};
    auto v = EmbeddingClient::parseResponse(o);
    CHECK(v.size() == 2 && v[0].size() == 2 && v[1].size() == 3);
    QJsonObject old;
    old["embedding"] = QJsonArray{1.0, 2.0};
    CHECK(EmbeddingClient::parseResponse(old).size() == 1);
    const QByteArray payload = EmbeddingClient::payload("mxbai", {"a", "b"});
    CHECK(payload.contains("mxbai") && payload.contains("\"input\""));
}

static void testHybrid() {
    // RRF: iki listede de üst sırada olan kazanır
    auto fused = HybridRanker::fuse({1, 2, 3}, {3, 1}, 1.0, 1.0, 60, 3);
    CHECK(fused.size() == 3);
    CHECK(fused.contains(1) && fused.contains(3));
    CHECK(fused.first() == 1 || fused.first() == 3); // ikisi de güçlü
    // ağırlık: yalnız anahtar kelime
    auto kwOnly = HybridRanker::fuse({1, 2, 3}, {}, 1.0, 0.0, 60, 3);
    CHECK(kwOnly == QList<int>({1, 2, 3}));

    QList<HybridRanker::Item> items = {
        {10, 5.0, 0.0}, {11, 0.0, 9.0}, {12, 1.0, 1.0}};
    auto combined = HybridRanker::combine(items, 0.3, 0.7, 2);
    CHECK(combined.size() == 2);
    CHECK(combined.first().id == 11); // semantik ağırlık yüksek
}

static void testScorer() {
    auto weak = ResponseScorer::score("tamam");
    auto strong = ResponseScorer::score(
        "İşte düzeltme:\n```cpp\nint main() { return 0; }\n```\nsrc/main.cpp dosyası güncellendi.");
    CHECK(strong.score > weak.score);
    CHECK(strong.hasCode && strong.hasFileRef);
    CHECK(weak.tooShort);
    CHECK(ResponseScorer::combine(40, 0) == 40);      // oy yok
    CHECK(ResponseScorer::combine(40, 5, 100) == 100); // oy ağırlığı
    CHECK(ResponseScorer::combine(100, 1, 50) == 60);
    CHECK(ResponseScorer::label(85) == "çok iyi");
    CHECK(ResponseScorer::label(20) == "zayıf");
}

static void testSummarizer() {
    QList<ConvTurn> turns;
    for (int i = 0; i < 10; ++i) turns << ConvTurn{i % 2 ? "ai" : "user", QString("tur %1").arg(i)};
    CHECK(ConversationSummarizer::olderTurns(turns, 4).size() == 6);
    CHECK(ConversationSummarizer::olderTurns(turns, 0).isEmpty());
    CHECK(ConversationSummarizer::needed(turns, "", 5, 4));   // çok küçük eşik
    CHECK(!ConversationSummarizer::needed(turns, "", 100000, 4));
    const QString prompt = ConversationSummarizer::summarizePrompt(turns);
    CHECK(prompt.contains("tur 0"));
    const QString merged = ConversationSummarizer::merge("ÖZET", {turns.last()});
    CHECK(merged.contains("ÖZET") && merged.contains("tur 9"));
}

static void testChatBranch() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    ChatStore store(dir.path());
    const QString sid = store.createSession("dallanma");
    const QString root = store.appendMsg(sid, "", ChatMessage{"", "", "user", "q1", 0});
    const QString a = store.appendMsg(sid, root, ChatMessage{"", "", "ai", "a1", 0});
    const QString b = store.appendMsg(sid, root, ChatMessage{"", "", "ai", "a2", 0});
    store.appendMsg(sid, a, ChatMessage{"", "", "user", "a1-devam", 0});
    const QString aLeaf = store.load(sid).messages.last().id;

    ChatSession s = store.load(sid);
    CHECK(s.messages.size() == 4);
    auto path = ChatStore::pathTo(s, aLeaf);
    CHECK(path.size() == 3);
    CHECK(path.first().text == "q1" && path.last().text == "a1-devam");
    CHECK(ChatStore::childrenOf(s, root).size() == 2);
    CHECK(ChatStore::childrenOf(s, "").size() == 1); // tek kök
    const QStringList leaves = ChatStore::leafIds(s);
    CHECK(leaves.size() == 2 && leaves.contains(b) && leaves.contains(aLeaf));

    // Eski doğrusal append de üst zincir kurar
    const QString sid2 = store.createSession("dogrusal");
    store.append(sid2, ChatMessage{"", "", "user", "ilk", 0});
    store.append(sid2, ChatMessage{"", "", "ai", "ikinci", 0});
    ChatSession s2 = store.load(sid2);
    auto p2 = ChatStore::pathTo(s2, s2.messages.last().id);
    CHECK(p2.size() == 2);
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    testCapabilities();
    testImageUtil();
    testEmbedding();
    testEmbeddingClient();
    testHybrid();
    testScorer();
    testSummarizer();
    testChatBranch();
    fprintf(stderr, "STAGE33: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
