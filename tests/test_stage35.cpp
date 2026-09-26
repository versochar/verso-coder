// Stage 35 testleri: sağlayıcı kayıt defteri (Nvidia NIM + UnoRouter dahil),
// gizli anahtar kasası, birleşik mesaj modeli, OpenAI-uyumlu / Anthropic /
// Gemini / Ollama gövde üretimi ve çözümleme, SSE ayrıştırıcı, hata eşleme,
// retry/backoff, gömme, araç çağırma köprüsü, sağlayıcı tercihleri.
// Ağ dışı: yalnız 127.0.0.1 üzerinde sahte sunucu (retry testi).
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>
#include <cstdio>

#include "../src/core/AgentTools.h"
#include "../src/core/ai/AiMessage.h"
#include "../src/core/ai/AiToolBridge.h"
#include "../src/core/ai/LlmClient.h"
#include "../src/core/ai/LlmProvider.h"
#include "../src/core/ai/ProviderPrefs.h"
#include "../src/core/ai/SecretStore.h"
#include "../src/core/ai/providers/ProviderCodec.h"
#include "../src/core/ai/providers/SseParser.h"

static int g_pass = 0, g_fail = 0;

static ProviderSpec uno_ref();

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

// --- 1 & 2: sağlayıcı kayıt defteri ---
static void testRegistry() {
    const auto all = ProviderRegistry::builtin();
    CHECK(all.size() >= 18);
    CHECK(ProviderRegistry::exists("ollama"));
    CHECK(ProviderRegistry::exists("openai"));
    CHECK(ProviderRegistry::exists("anthropic"));
    CHECK(ProviderRegistry::exists("gemini"));
    // istenen iki sağlayıcı
    CHECK(ProviderRegistry::exists("nvidia-nim"));
    CHECK(ProviderRegistry::exists("nvidia-nim-local"));
    CHECK(ProviderRegistry::exists("unorouter"));
    CHECK(ProviderRegistry::exists("unorouter-anthropic"));
    CHECK(ProviderRegistry::exists("unorouter-gemini"));
    CHECK(ProviderRegistry::ids().size() == all.size());
    CHECK(ProviderRegistry::defaultProvider().id == "ollama");

    // Ollama anahtar istemez
    const ProviderSpec ollama = ProviderRegistry::byId("ollama");
    CHECK(!ollama.requiresKey());
    CHECK(ollama.chatPath == "/api/chat");
    CHECK(ollama.modelsPath == "/api/tags");
    CHECK(ollama.badge() == "yerel");

    // NVIDIA NIM
    const ProviderSpec nim = ProviderRegistry::byId("nvidia-nim");
    CHECK(nim.baseUrl == "https://integrate.api.nvidia.com/v1");
    CHECK(nim.auth == AuthStyle::Bearer);
    CHECK(nim.chatPath == "/chat/completions");
    CHECK(nim.supportsTools && nim.supportsVision && nim.supportsEmbed);
    CHECK(nim.quirks & QRequiresMaxTokens);      // max_tokens zorunlu
    CHECK(nim.quirks & QModelIdPrefix);          // nvidia/ öneki
    CHECK(nim.quirks & QReasoningEffort);
    CHECK(nim.hint.contains("build.nvidia.com"));
    CHECK(nim.resolveModelId("llama-3.1-70b-instruct") == "nvidia-nim/llama-3.1-70b-instruct");
    CHECK(nim.resolveModelId("nvidia/llama-3.1-70b-instruct")
          == "nvidia/llama-3.1-70b-instruct"); // zaten önekli, dokunmaz
    CHECK(ProviderRegistry::byId("nvidia-nim-local").baseUrl == "http://localhost:8000/v1");
    CHECK(!ProviderRegistry::byId("nvidia-nim-local").requiresKey());

    // UnoRouter
    const ProviderSpec uno = ProviderRegistry::byId("unorouter");
    CHECK(uno.baseUrl == "https://api.unorouter.com/v1");
    CHECK(uno.chatPath == "/chat/completions");
    CHECK(uno.modelsPath == "/models");
    CHECK(uno.embedPath == "/embeddings");
    CHECK(uno.quirks & QFreeModelSuffix);
    CHECK(uno.quirks & QNativeGateway);
    CHECK(uno.isGateway());
    CHECK(uno.badge() == "ağgeçidi");
    CHECK(uno.isFreeModel("gpt-oss-120b:free"));
    CHECK(!uno.isFreeModel("gpt-4o-mini"));
    CHECK(!nim.isFreeModel("gpt-oss-120b:free"));
    CHECK(uno.supportsTools && uno.supportsEmbed);

    // UnoRouter çoklu protokol geçidi
    const ProviderSpec unoA = ProviderRegistry::byId("unorouter-anthropic");
    CHECK(unoA.kind == ProviderKind::Anthropic);
    CHECK(unoA.chatPath == "/messages");
    CHECK(unoA.auth == AuthStyle::AnthropicKey);
    const ProviderSpec unoG = ProviderRegistry::byId("unorouter-gemini");
    CHECK(unoG.kind == ProviderKind::Gemini);
    CHECK(unoG.auth == AuthStyle::GoogleKey);
    CHECK(unoG.chatPath == "/models/%1:generateContent");

    // Diğer aile
    CHECK(ProviderRegistry::byId("anthropic").kind == ProviderKind::Anthropic);
    CHECK(ProviderRegistry::byId("gemini").kind == ProviderKind::Gemini);
    CHECK(ProviderRegistry::byId("groq").baseUrl.contains("groq"));
    CHECK(ProviderRegistry::byId("azure-openai").auth == AuthStyle::AzureApiKey);
    CHECK(ProviderRegistry::byId("openai-compatible").baseUrl.isEmpty()); // elle girilir
    CHECK(ProviderRegistry::byKind(ProviderKind::Gemini).size() == 2);     // gemini + uno geçidi
    CHECK(ProviderRegistry::byKind(ProviderKind::Anthropic).size() == 2);
    CHECK(!ProviderSpec().resolveModelId("x").isEmpty());                  // quirk yoksa dokunmaz
    CHECK(ProviderSpec().resolveModelId("gpt-4o") == "gpt-4o");
    CHECK(!ProviderRegistry::sampleModels("nvidia-nim").isEmpty());
    CHECK(ProviderRegistry::sampleModels("unorouter").contains("gpt-oss-120b:free"));
    CHECK(ProviderRegistry::sampleModels("bilinmeyen").isEmpty());
    CHECK(ProviderRegistry::byId("yok").id.isEmpty());
}

// --- URL + kimlik doğrulama başlıkları ---
static void testUrlsAndAuth() {
    ProviderSpec s = ProviderRegistry::byId("openai");
    s.baseUrl = "https://api.openai.com/v1/";
    CHECK(s.url("/models") == "https://api.openai.com/v1/models");
    CHECK(s.url("models") == "https://api.openai.com/v1/models");
    CHECK(s.url("") == "https://api.openai.com/v1");

    ProviderSpec a = ProviderRegistry::byId("anthropic");
    QJsonObject h = a.allHeaders("sk-ant-123");
    CHECK(h.value("x-api-key").toString() == "sk-ant-123");
    CHECK(h.value("anthropic-version").toString() == "2023-06-01");
    CHECK(!h.contains("Authorization"));

    ProviderSpec g = ProviderRegistry::byId("gemini");
    CHECK(g.allHeaders("gk").value("x-goog-api-key").toString() == "gk");

    ProviderSpec z = ProviderRegistry::byId("azure-openai");
    CHECK(z.allHeaders("azkey").value("api-key").toString() == "azkey");

    CHECK(ProviderRegistry::byId("ollama").allHeaders("").isEmpty());
    CHECK(a.allHeaders("").contains("anthropic-version")); // ek başlık anahtarsız da gelir

    ProviderSpec c;
    c.auth = AuthStyle::CustomHeader;
    c.apiKeyHeader = "X-Custom";
    c.apiKeyPrefix = "Token ";
    c.extraHeaders << "X-Trace: 1";
    const QJsonObject ch = c.allHeaders("abc");
    CHECK(ch.value("X-Custom").toString() == "Token abc");
    CHECK(ch.value("X-Trace").toString() == "1");
}

static void testCustomProvider() {
    const ProviderSpec c = ProviderRegistry::makeCustom("ozel-1", "Özel Sunucu",
                                                        "http://localhost:9999/v1/");
    CHECK(c.id == "ozel-1");
    CHECK(c.baseUrl == "http://localhost:9999/v1"); // sondaki / temizlendi
    CHECK(!c.builtin);
    CHECK(ProviderRegistry::save(c));
    const ProviderSpec got = ProviderRegistry::byId("ozel-1");
    CHECK(got.label == "Özel Sunucu");
    CHECK(got.baseUrl == "http://localhost:9999/v1");
    CHECK(ProviderRegistry::remove("ozel-1"));
    CHECK(!ProviderRegistry::exists("ozel-1"));
    CHECK(!ProviderRegistry::remove("ollama"));   // dahili silinemez
    CHECK(!ProviderRegistry::save(ProviderSpec())); // eksik veri
}

// --- 3: gizli anahtar kasası ---
static void testSecrets() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString f = dir.path() + "/keys.json";
    SecretStore s(f);
    s.setUseKeyring(false); // testte ağsız/alt sistemden bağımsız
    CHECK(s.count() == 0);
    CHECK(!s.has("openai"));
    CHECK(s.set("openai", "sk-test-1234567890"));
    CHECK(s.has("openai"));
    CHECK(s.get("openai") == "sk-test-1234567890");
    CHECK(s.count() == 1);
    // 0600 izin
    QFile::setPermissions(f, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    const auto perms = QFile::permissions(f);
    CHECK(!(perms & QFileDevice::ReadGroup));
    CHECK(!(perms & QFileDevice::ReadOther));

    // Yeni örnek: diskten okur
    SecretStore again(f);
    again.setUseKeyring(false);
    CHECK(again.get("openai") == "sk-test-1234567890");
    CHECK(again.providers().contains("openai"));

    // Maskeleme: anahtar sızmaz
    const QString masked = SecretStore::maskKey("sk-test-1234567890");
    CHECK(masked != "sk-test-1234567890");
    CHECK(masked.startsWith("sk-t"));
    CHECK(masked.endsWith("890"));
    CHECK(SecretStore::maskKey("k").size() == 1);
    CHECK(SecretStore::maskKey("").isEmpty());
    CHECK(SecretStore::maskKey("12345678").size() == 8);

    // Ortam değişkeni yedeği
    qputenv("VERSO_AI_KEY_GROQ", "env-key-1");
    CHECK(SecretStore::envKey("groq") == "env-key-1");
    SecretStore noKey(f);
    noKey.setUseKeyring(false);
    CHECK(noKey.effectiveKey("groq") == "env-key-1");
    noKey.set("groq", "file-key");
    CHECK(noKey.effectiveKey("groq") == "file-key"); // kasa env'ye üstün
    qunsetenv("VERSO_AI_KEY_GROQ");

    CHECK(s.remove("openai"));
    CHECK(!s.has("openai"));
    CHECK(!s.remove("openai"));
    CHECK(!s.set("", "x"));
    CHECK(!s.set("openai", "   ")); // boş = sil
    s.set("anthropic", "a1");
    s.clearAll();
    CHECK(s.count() == 0);
}

// --- 5: birleşik mesaj modeli ---
static void testMessages() {
    const AiMessage sys = AiMessage::system("Sistem");
    CHECK(sys.role == AiRole::System && sys.text() == "Sistem");
    const AiMessage u = AiMessage::user("Merhaba");
    CHECK(u.role == AiRole::User && !u.isEmpty());

    AiImage img;
    img.mime = "image/jpeg";
    img.bytes = QByteArray("abc");
    CHECK(img.base64() == "YWJj");
    const AiMessage uv = AiMessage::userText("Resim", {img});
    CHECK(uv.images.size() == 1);
    CHECK(uv.text() == "Resim");

    const AiMessage tr = AiMessage::toolResultMsg("call_1", "read_file", "çıktı", true);
    CHECK(tr.role == AiRole::Tool);
    CHECK(tr.callId == "call_1" && tr.name == "read_file");
    CHECK(tr.toolOutput == "çıktı" && tr.ok);
    CHECK(!tr.isEmpty());

    AiUsage u2;
    u2.promptTokens = 10;
    u2.evalTokens = 5;
    CHECK(u2.total() == 15);
    CHECK(AiMessage().isEmpty());
}

// --- 6/7/8/9: gövde üretimi ---
static void testChatRequestBodies() {
    AiChatRequest req;
    req.model = "gpt-4o";
    req.systemPrompt = "SİSTEM";
    req.messages << AiMessage::user("Merhaba");
    req.temperature = 0.3;

    // --- OpenAI-uyumlu ---
    const ProviderSpec oa = ProviderRegistry::byId("openai");
    QJsonObject b = ProviderCodec::chatRequest(oa, req);
    CHECK(b.value("model").toString() == "gpt-4o");
    CHECK(b.value("temperature").toDouble() == 0.3);
    CHECK(b.value("stream").toBool() == false);
    // NIM benzeri max_tokens otomatik tamamlanır
    CHECK(b.value("max_tokens").toInt() == 4096);
    const QJsonArray msgs = b.value("messages").toArray();
    CHECK(msgs.size() == 2);
    CHECK(msgs.first().toObject().value("role").toString() == "system");
    CHECK(msgs.at(1).toObject().value("content").toString() == "Merhaba");

    // Görsel içerik parçaları
    AiImage img;
    img.bytes = QByteArray("x");
    AiChatRequest vreq = req;
    vreq.messages = {AiMessage::userText("bak", {img})};
    const QJsonObject vb = ProviderCodec::chatRequest(oa, vreq);
    const QJsonValue content = vb.value("messages").toArray().at(1).toObject().value("content");
    CHECK(content.isArray());
    CHECK(content.toArray().size() == 2);
    CHECK(content.toArray().at(1).toObject().value("type").toString() == "image_url");
    CHECK(content.toArray().at(1).toObject().value("image_url").toObject()
              .value("url").toString().startsWith("data:image/jpeg;base64,"));

    // Araç şeması + yalnız destekleyen sağlayıcıda reasoning_effort
    AiChatRequest treq = req;
    treq.wantTools = true;
    AiToolDef td;
    td.name = "read_file";
    td.description = "oku";
    td.parameters = QJsonObject{{"type", "object"}};
    treq.tools << td;
    const QJsonObject tb = ProviderCodec::chatRequest(oa, treq);
    CHECK(tb.value("tools").isArray());
    CHECK(tb.value("tools").toArray().at(0).toObject().value("type").toString() == "function");
    CHECK(tb.value("tool_choice").toString() == "auto");
    CHECK(!tb.contains("reasoning_effort"));
    const ProviderSpec nim = ProviderRegistry::byId("nvidia-nim");
    AiChatRequest nreq = treq;
    nreq.model = "llama-3.1-70b-instruct";
    nreq.reasoningEffort = "medium";
    const QJsonObject nb = ProviderCodec::chatRequest(nim, nreq);
    CHECK(nb.value("reasoning_effort").toString() == "medium");
    CHECK(nb.value("model").toString() == "nvidia-nim/llama-3.1-70b-instruct");

    // --- Anthropic ---
    const ProviderSpec an = ProviderRegistry::byId("anthropic");
    QJsonObject ab = ProviderCodec::chatRequest(an, req);
    CHECK(ab.value("system").toString() == "SİSTEM");
    CHECK(!ab.contains("messages") || ab.value("messages").toArray().at(0).toObject()
              .value("role").toString() == "user");
    CHECK(ab.value("max_tokens").toInt() == 4096);
    const QJsonArray atb = ProviderCodec::chatRequest(an, treq).value("tools").toArray();
    CHECK(atb.at(0).toObject().contains("input_schema"));
    CHECK(!atb.at(0).toObject().contains("type")); // Anthropic'da type yok
    // Araç sonucu tool_result bloğu olur
    AiChatRequest tres = req;
    tres.messages = {AiMessage::user("q"),
                     AiMessage::toolResultMsg("tu_1", "read_file", "SONUÇ")};
    const QJsonObject arb = ProviderCodec::chatRequest(an, tres);
    const QJsonArray amsgs = arb.value("messages").toArray();
    const QJsonObject block = amsgs.at(1).toObject().value("content").toArray().at(0).toObject();
    CHECK(block.value("type").toString() == "tool_result");
    CHECK(block.value("tool_use_id").toString() == "tu_1");
    CHECK(block.value("content").toString() == "SONUÇ");

    // --- Gemini ---
    const ProviderSpec ge = ProviderRegistry::byId("gemini");
    QJsonObject gb = ProviderCodec::chatRequest(ge, req);
    CHECK(gb.contains("contents"));
    const QJsonArray gcontents = gb.value("contents").toArray();
    CHECK(gcontents.size() == 2); // sistem + kullanıcı
    CHECK(gcontents.at(1).toObject().value("role").toString() == "user");
    CHECK(gcontents.at(1).toObject().value("parts").toArray().at(0).toObject()
              .contains("text"));
    const QJsonObject gtb = ProviderCodec::chatRequest(ge, treq);
    CHECK(gtb.value("tools").toArray().at(0).toObject().contains("functionDeclarations"));
    AiChatRequest gv = req;
    gv.messages = {AiMessage::userText("bak", {img})};
    const QJsonObject gvb = ProviderCodec::chatRequest(ge, gv);
    const QJsonValue gpart = gvb.value("contents").toArray().at(1).toObject()
                                 .value("parts").toArray().at(1).toObject();
    CHECK(gpart.toObject().contains("inlineData"));
    CHECK(gpart.toObject().value("inlineData").toObject().value("mimeType").toString()
          == "image/jpeg");

    // --- Ollama ---
    const ProviderSpec ol = ProviderRegistry::byId("ollama");
    AiChatRequest oreq = req;
    oreq.model = "qwen2.5-coder:1.5b";
    oreq.numCtx = 8192;
    QJsonObject ob = ProviderCodec::chatRequest(ol, oreq);
    CHECK(ob.value("model").toString() == "qwen2.5-coder:1.5b");
    CHECK(ob.value("options").toObject().value("num_ctx").toInt() == 8192);
    CHECK(ob.value("messages").toArray().size() == 2);
    AiChatRequest ov = oreq;
    ov.messages = {AiMessage::userText("bak", {img})};
    const QJsonObject ovb = ProviderCodec::chatRequest(ol, ov);
    CHECK(ovb.value("messages").toArray().at(1).toObject().value("images").toArray().size() == 1);

    // --- chatUrl'ler ---
    CHECK(ProviderCodec::chatUrl(ol, "m").contains("/api/chat"));
    CHECK(ProviderCodec::chatUrl(an, "claude").contains("/v1/messages"));
    CHECK(ProviderCodec::chatUrl(ge, "gemini-2.5-pro")
              == "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-pro:generateContent");
    const ProviderSpec az = ProviderRegistry::byId("azure-openai");
    CHECK(ProviderCodec::chatUrl(az, "my-deploy")
              .contains("/deployments/my-deploy/chat/completions?api-version="));
    CHECK(ProviderCodec::modelsUrl(az).isEmpty()); // Azure'da katalog yok
    CHECK(ProviderCodec::modelsUrl(uno_ref()).contains("/v1/models"));
}

static ProviderSpec uno_ref() { return ProviderRegistry::byId("unorouter"); }

// --- 6/7/8: yanıt çözümleme ---
static void testParseReplies() {
    // OpenAI-uyumlu
    const ProviderSpec oa = ProviderRegistry::byId("openai");
    const QByteArray oaBody = R"({"model":"gpt-4o","choices":[{"message":{"content":"Merhaba"},
        "finish_reason":"stop"}],"usage":{"prompt_tokens":11,"completion_tokens":7,
        "completion_tokens_details":{"reasoning_tokens":3}}})";
    AiReply r = ProviderCodec::parseReply(oa, oaBody, "gpt-4o");
    CHECK(r.ok);
    CHECK(r.text == "Merhaba");
    CHECK(r.usage.promptTokens == 11 && r.usage.evalTokens == 7);
    CHECK(r.usage.reasoningTokens == 3);
    CHECK(r.finishReason == "durdu");
    CHECK(r.model == "gpt-4o");

    // reasoning_content + tool_calls
    const QByteArray toolBody = R"({"choices":[{"message":{"content":"","reasoning_content":"düşünüyorum",
        "tool_calls":[{"id":"c1","function":{"name":"read_file",
        "arguments":"{\"path\":\"a.cpp\"}"}}]},"finish_reason":"tool_calls"}]})";
    AiReply tr = ProviderCodec::parseReply(oa, toolBody);
    CHECK(tr.toolCalls.size() == 1);
    CHECK(tr.toolCalls.first().name == "read_file");
    CHECK(tr.toolCalls.first().args.value("path").toString() == "a.cpp");
    CHECK(tr.toolCalls.first().id == "c1");
    CHECK(tr.reasoning == "düşünüyorum");
    CHECK(tr.finishReason == "araç çağrısı");

    // Anthropic
    const ProviderSpec an = ProviderRegistry::byId("anthropic");
    const QByteArray anBody = R"({"content":[{"type":"thinking","thinking":"hm"},
        {"type":"text","text":"Cevap"},{"type":"tool_use","id":"tu1","name":"list_dir",
        "input":{"path":"src"}}],"stop_reason":"tool_use","usage":{"input_tokens":5,
        "output_tokens":9}})";
    AiReply ar = ProviderCodec::parseReply(an, anBody);
    CHECK(ar.ok);
    CHECK(ar.text == "Cevap");
    CHECK(ar.reasoning == "hm");
    CHECK(ar.toolCalls.size() == 1);
    CHECK(ar.toolCalls.first().name == "list_dir");
    CHECK(ar.toolCalls.first().args.value("path").toString() == "src");
    CHECK(ar.usage.promptTokens == 5 && ar.usage.evalTokens == 9);
    CHECK(ar.finishReason == "tool_use");

    // Gemini
    const ProviderSpec ge = ProviderRegistry::byId("gemini");
    const QByteArray geBody = R"({"candidates":[{"content":{"parts":[{"text":"Merhaba"},
        {"functionCall":{"name":"search","args":{"pattern":"TODO"}}}]},
        "finishReason":"STOP"}],"usageMetadata":{"promptTokenCount":4,
        "candidatesTokenCount":6,"thoughtsTokenCount":2},"modelVersion":"gemini-2.5-pro"})";
    AiReply gr = ProviderCodec::parseReply(ge, geBody);
    CHECK(gr.ok);
    CHECK(gr.text == "Merhaba");
    CHECK(gr.toolCalls.size() == 1);
    CHECK(gr.toolCalls.first().args.value("pattern").toString() == "TODO");
    CHECK(gr.usage.reasoningTokens == 2);
    CHECK(gr.finishReason == "durdu");
    CHECK(gr.model == "gemini-2.5-pro");

    // Ollama
    const ProviderSpec ol = ProviderRegistry::byId("ollama");
    const QByteArray olBody = R"({"model":"qwen","message":{"content":"Yanıt"},
        "prompt_eval_count":8,"eval_count":3})";
    AiReply orp = ProviderCodec::parseReply(ol, olBody, "qwen");
    CHECK(orp.ok);
    CHECK(orp.text == "Yanıt");
    CHECK(orp.usage.promptTokens == 8 && orp.usage.evalTokens == 3);

    // Hata gövdesi
    const QByteArray errBody = R"({"error":{"message":"Incorrect API key"}})";
    AiReply er = ProviderCodec::parseReply(oa, errBody);
    CHECK(!er.ok);
    CHECK(er.error.contains("Incorrect API key"));
    // Bozuk JSON
    AiReply bad = ProviderCodec::parseReply(oa, QByteArray("not json"));
    CHECK(!bad.ok);
    CHECK(!bad.error.isEmpty());
}

// --- 10: akış ayrıştırıcı ---
static void testSse() {
    SseParser p;
    // Parçalar bölünmüş hâlde gelsin
    p.feed("data: {\"choices\":[{\"delta\":{\"content\":\"Mer");
    CHECK(p.eventCount() == 0); // olay henüz tamamlanmadı
    p.feed("haba\"}}]}\n\n");
    CHECK(p.eventCount() == 1);
    CHECK(p.events().first().contains("Merhaba"));

    // Çok satırlı data + diğer alanlar
    p.feed("event: message\n: yorum satiri\ndata: {\"a\":1}\ndata: {\"b\":2}\n\n");
    CHECK(p.eventCount() == 2);
    CHECK(p.events().at(1).contains("{\"a\":1}"));
    CHECK(p.events().at(1).contains("{\"b\":2}"));

    // [DONE]
    p.feed("data: [DONE]\n\n");
    CHECK(p.isDone());
    // CRLF normalizasyonu
    p.feed("data: {\"x\":1}\r\n\r\n");
    CHECK(p.eventCount() == 3);
    // flush
    p.feed("data: {\"y\":2}\n");
    CHECK(p.eventCount() == 3);
    p.flush();
    CHECK(p.eventCount() == 4);
    p.clear();
    CHECK(p.eventCount() == 0 && !p.isDone());
    p.feed("data: boş değil\n\n");
    CHECK(p.events().first() == "boş değil");
    // Gerçekçi OpenAI akışı
    SseParser s2;
    s2.feed("data: {\"choices\":[{\"delta\":{\"content\":\"A\"}}]}\n\n"
            "data: {\"choices\":[{\"delta\":{\"content\":\"B\"}}]}\n\n"
            "data: {\"choices\":[{\"delta\":{},\"finish_reason\":\"stop\"}]}\n\n"
            "data: [DONE]\n\n");
    CHECK(s2.eventCount() == 3); // 3 olay + [DONE]
    CHECK(s2.isDone());
}

static void testStreamEvents() {
    const ProviderSpec oa = ProviderRegistry::byId("openai");
    AiChunk c = ProviderCodec::parseStreamEvent(oa, R"({"choices":[{"delta":{"content":"Mer"}}]})");
    CHECK(c.text == "Mer");
    CHECK(!c.done);
    c = ProviderCodec::parseStreamEvent(oa, R"({"choices":[{"delta":{},"finish_reason":"stop"}]})");
    CHECK(c.done && c.finishReason == "durdu");
    c = ProviderCodec::parseStreamEvent(oa, R"({"choices":[{"delta":{"reasoning_content":"d"}}]})");
    CHECK(c.reasoning == "d");
    c = ProviderCodec::parseStreamEvent(oa, R"({"choices":[{"delta":{"tool_calls":[{"id":"t1",
        "function":{"name":"x","arguments":"{}"}}]}}]})");
    CHECK(c.toolCalls.size() == 1 && c.toolCalls.first().name == "x");
    c = ProviderCodec::parseStreamEvent(oa, "[DONE]");
    CHECK(c.done);
    c = ProviderCodec::parseStreamEvent(oa, R"({"choices":[],"usage":{"prompt_tokens":5,
        "completion_tokens":6}})");
    CHECK(c.usage.promptTokens == 5 && c.usage.evalTokens == 6);

    const ProviderSpec an = ProviderRegistry::byId("anthropic");
    c = ProviderCodec::parseStreamEvent(an,
        R"({"type":"content_block_delta","delta":{"type":"text_delta","text":"Par"}})");
    CHECK(c.text == "Par");
    c = ProviderCodec::parseStreamEvent(an, R"({"type":"message_start","message":{"usage":
        {"input_tokens":12}}})");
    CHECK(c.usage.promptTokens == 12);
    c = ProviderCodec::parseStreamEvent(an, R"({"type":"message_delta","delta":
        {"stop_reason":"end_turn"},"usage":{"output_tokens":8}})");
    CHECK(c.done && c.usage.evalTokens == 8);

    const ProviderSpec ge = ProviderRegistry::byId("gemini");
    c = ProviderCodec::parseStreamEvent(ge, R"({"candidates":[{"content":{"parts":
        [{"text":"Sa"}]}}]})");
    CHECK(c.text == "Sa");
    c = ProviderCodec::parseStreamEvent(ge, R"({"candidates":[{"finishReason":"STOP"}],
        "usageMetadata":{"candidatesTokenCount":3}})");
    CHECK(c.done && c.usage.evalTokens == 3);
}

// --- 4: model listeleme ---
static void testParseModels() {
    const ProviderSpec oa = ProviderRegistry::byId("openai");
    const QByteArray oaBody = R"({"data":[{"id":"gpt-4o"},{"id":"gpt-4o-mini"}]})";
    CHECK(ProviderCodec::parseModels(oa, oaBody).size() == 2);
    CHECK(ProviderCodec::parseModels(oa, oaBody).first() == "gpt-4o");
    // NIM/UnoRouter aynı şekilde
    CHECK(ProviderCodec::parseModels(ProviderRegistry::byId("nvidia-nim"), oaBody).size() == 2);
    CHECK(ProviderCodec::parseModels(ProviderRegistry::byId("unorouter"), oaBody).size() == 2);
    // Anthropic
    CHECK(ProviderCodec::parseModels(ProviderRegistry::byId("anthropic"), oaBody).size() == 2);
    // Gemini: "models/gemini-2.5-pro" → "gemini-2.5-pro"
    const QByteArray gBody = R"({"models":[{"name":"models/gemini-2.5-pro"}]})";
    CHECK(ProviderCodec::parseModels(ProviderRegistry::byId("gemini"), gBody).first()
          == "gemini-2.5-pro");
    // Ollama
    const QByteArray oBody = R"({"models":[{"name":"qwen2.5:1.5b"},{"name":"llava:7b"}]})";
    CHECK(ProviderCodec::parseModels(ProviderRegistry::byId("ollama"), oBody).size() == 2);
    // bozuk
    CHECK(ProviderCodec::parseModels(oa, QByteArray("[]")).isEmpty());
}

// --- 12: gömme ---
static void testEmbed() {
    const ProviderSpec oa = ProviderRegistry::byId("openai");
    QString resolved;
    const QJsonObject b = ProviderCodec::embedRequest(oa, "text-embedding-3-small",
                                                      {"a", "b"}, &resolved);
    CHECK(resolved == "text-embedding-3-small");
    CHECK(b.value("input").toArray().size() == 2);
    const QByteArray resp = R"({"data":[{"embedding":[0.1,0.2]},{"embedding":[0.3,0.4]}]})";
    auto v = ProviderCodec::parseEmbed(oa, resp);
    CHECK(v.size() == 2 && v[0].size() == 2);
    CHECK(qAbs(v[1][1] - 0.4) < 1e-6);

    const ProviderSpec ge = ProviderRegistry::byId("gemini");
    const QJsonObject gb = ProviderCodec::embedRequest(ge, "text-embedding-004", {"a"}, &resolved);
    CHECK(resolved.startsWith("models/"));
    CHECK(gb.value("requests").toArray().size() == 1);
    auto gv = ProviderCodec::parseEmbed(ge, R"({"embeddings":[{"values":[1,2,3]}]})");
    CHECK(gv.size() == 1 && gv[0].size() == 3);

    // NIM embeddings
    const ProviderSpec nim = ProviderRegistry::byId("nvidia-nim");
    CHECK(nim.supportsEmbed);
    const QJsonObject nb = ProviderCodec::embedRequest(nim, "nvidia/nv-embedqa-e5-v5", {"x"});
    CHECK(nb.value("model").toString() == "nvidia/nv-embedqa-e5-v5");
    // UnoRouter embeddings
    CHECK(ProviderCodec::embedRequest(uno_ref(), "openai/text-embedding-3-small", {"x"})
              .value("model").toString().contains("text-embedding"));
    // desteklemeyen
    ProviderSpec az = ProviderRegistry::byId("azure-openai");
    az.supportsEmbed = false;
    CHECK(ProviderCodec::embedUrl(az, "x").isEmpty());
}

// --- 11: hata eşleme + retry ---
static void testErrors() {
    const ProviderSpec oa = ProviderRegistry::byId("openai");
    const ProviderSpec nim = ProviderRegistry::byId("nvidia-nim");
    const ProviderSpec az = ProviderRegistry::byId("azure-openai");

    CHECK(ProviderRegistry::mapError(oa, 0, QByteArray()).contains("Ağ"));
    CHECK(ProviderRegistry::mapError(oa, 401, R"({"error":{"message":"bad key"}})")
              .contains("anahtar"));
    CHECK(ProviderRegistry::mapError(oa, 401, QByteArray()).contains("anahtar"));
    // NIM'e özel 404 ipucu
    const QString n404 = ProviderRegistry::mapError(nim, 404, QByteArray());
    CHECK(n404.contains("nvidia/"));
    CHECK(ProviderRegistry::mapError(oa, 404, QByteArray()).contains("Model"));
    // Azure deployment ipucu
    CHECK(ProviderRegistry::mapError(az, 404, QByteArray()).contains("deployment"));
    // 422 + max_tokens ipucu
    CHECK(ProviderRegistry::mapError(nim, 422, QByteArray()).contains("max_tokens"));
    CHECK(ProviderRegistry::mapError(oa, 429, QByteArray()).contains("429"));
    CHECK(ProviderRegistry::mapError(oa, 503, QByteArray()).contains("503"));
    CHECK(ProviderRegistry::mapError(oa, 413, QByteArray()).contains("büyük"));
    // sunucu mesajı eklenir
    CHECK(ProviderRegistry::mapError(oa, 500, R"({"error":{"message":"boom"}})").contains("boom"));
    // bilinmeyen kod
    CHECK(ProviderRegistry::mapError(oa, 418, QByteArray()).contains("418"));

    // Retry politikası
    CHECK(LlmClient::isRetryable(429));
    CHECK(LlmClient::isRetryable(500));
    CHECK(LlmClient::isRetryable(503));
    CHECK(LlmClient::isRetryable(408));
    CHECK(!LlmClient::isRetryable(400));
    CHECK(!LlmClient::isRetryable(401));
    CHECK(!LlmClient::isRetryable(404));
    // Retry-After baskın
    CHECK(LlmClient::retryDelayMs(0, 429, "3") == 3000);
    CHECK(LlmClient::retryDelayMs(0, 500, "0") == 200); // alt sınır
    CHECK(LlmClient::retryDelayMs(0, 500, "9999") == 60000); // üst sınır
    // üstel büyüme
    const int d0 = LlmClient::retryDelayMs(0, 500);
    const int d1 = LlmClient::retryDelayMs(1, 500);
    const int d2 = LlmClient::retryDelayMs(2, 500);
    CHECK(d1 > d0 && d2 > d1);
    CHECK(d0 >= 200 && d0 <= 30000);
    CHECK(LlmClient::maxRetries() == 3);
    // istek başlıkları
    const QJsonObject h = LlmClient::requestHeaders(ProviderRegistry::byId("anthropic"), "k1");
    CHECK(h.value("Content-Type").toString() == "application/json");
    CHECK(h.value("x-api-key").toString() == "k1");
    CHECK(LlmClient::requestHeaders(ProviderRegistry::byId("ollama"), "").contains("Accept"));
}

// --- 13: araç çağırma köprüsü ---
static void testToolBridge() {
    const QJsonArray schemas = AgentTools().toolSchemas();
    const auto defs = AiToolBridge::fromOpenAiSchemas(schemas);
    CHECK(defs.size() == schemas.size());
    CHECK(defs.size() >= 13); // eski 6 + Stage 34'ün 7 aracı
    bool hasRead = false, hasHealth = false, hasTests = false;
    for (const AiToolDef& d : defs) {
        if (d.name == "read_file") hasRead = true;
        if (d.name == "health_scan") hasHealth = true;
        if (d.name == "run_tests") hasTests = true;
    }
    CHECK(hasRead && hasHealth && hasTests);
    CHECK(!defs.first().description.isEmpty());

    // native → ajan biçimi
    QList<AiToolCall> native;
    AiToolCall c;
    c.id = "call_1";
    c.name = "read_file";
    c.args = QJsonObject{{"path", "a.cpp"}};
    native << c;
    const auto agentCalls = AiToolBridge::toAgentCalls(native);
    CHECK(agentCalls.size() == 1);
    CHECK(agentCalls.first().name == "read_file");
    CHECK(agentCalls.first().args.value("path").toString() == "a.cpp");

    // extract: native öncelikli
    AiReply r;
    r.toolCalls = native;
    CHECK(AiToolBridge::extract(r).size() == 1);
    // extract: native yoksa metin protokolüne düşer
    AiReply r2;
    r2.text = "<tool_call>\n{\"name\":\"list_dir\",\"arguments\":{\"path\":\"src\"}}\n"
              "</tool_call>";
    const auto fallback = AiToolBridge::extract(r2);
    CHECK(fallback.size() == 1);
    CHECK(fallback.first().name == "list_dir");
    // hiçbiri yok
    CHECK(AiToolBridge::extract(AiReply()).isEmpty());
    // araç sonucu mesajı
    const AiMessage tr = AiToolBridge::toolResultMessage(c, "çıktı", true);
    CHECK(tr.role == AiRole::Tool && tr.toolOutput == "çıktı");
    CHECK(AiToolBridge::textProtocolHint().contains("tool_call"));
    CHECK(AiToolBridge::supportsNativeTools(ProviderRegistry::byId("openai")));
    CHECK(AiToolBridge::supportsNativeTools(ProviderRegistry::byId("nvidia-nim")));
    CHECK(AiToolBridge::supportsNativeTools(ProviderRegistry::byId("unorouter")));
}

// --- 14: sağlayıcı tercihleri ---
static void testPrefs() {
    ProviderPrefs::reset();
    CHECK(ProviderPrefs::activeProvider() == "ollama");
    ProviderPrefs::setActiveProvider("nvidia-nim");
    CHECK(ProviderPrefs::activeProvider() == "nvidia-nim");
    ProviderPrefs::setModel("nvidia-nim", "nvidia/llama-3.1-70b-instruct");
    CHECK(ProviderPrefs::modelFor("nvidia-nim") == "nvidia/llama-3.1-70b-instruct");
    CHECK(ProviderPrefs::modelFor("openai", "gpt-4o") == "gpt-4o"); // yedek değer
    ProviderPrefs::setUrl("openai-compatible", "http://localhost:1234/v1/");
    CHECK(ProviderPrefs::urlFor("openai-compatible") == "http://localhost:1234/v1");
    CHECK(ProviderPrefs::configuredProviders().contains("nvidia-nim"));
    CHECK(ProviderPrefs::configuredProviders().contains("openai-compatible"));

    const ProviderSpec r = ProviderPrefs::resolve("openai-compatible");
    CHECK(r.baseUrl == "http://localhost:1234/v1");
    const ProviderSpec r2 = ProviderPrefs::resolve("nvidia-nim");
    CHECK(r2.baseUrl == "https://integrate.api.nvidia.com/v1");
    const ProviderSpec r3 = ProviderPrefs::resolve();
    CHECK(r3.id == "nvidia-nim"); // aktif sağlayıcı
    const ProviderSpec r4 = ProviderPrefs::resolve("olmayan");
    CHECK(r4.id == "ollama"); // güvenli geri düşüş
    ProviderPrefs::setModel("nvidia-nim", "");
    CHECK(ProviderPrefs::modelFor("nvidia-nim", "x") == "x"); // temizlenince yedeğe düşer
    ProviderPrefs::setUrl("openai-compatible", "");
    CHECK(ProviderPrefs::urlFor("openai-compatible").isEmpty());
    ProviderPrefs::reset();
    CHECK(ProviderPrefs::activeProvider() == "ollama");
    CHECK(ProviderPrefs::configuredProviders().isEmpty());
}

// --- 11: yerel sahte sunucu ile gerçek retry ---
static void testLiveRetry() {
    QTcpServer server;
    CHECK(server.listen(QHostAddress::LocalHost, 0));
    const quint16 port = server.serverPort();
    int hits = 0;

    QObject::connect(&server, &QTcpServer::newConnection, &server, [&]() {
        QTcpSocket* sock = server.nextPendingConnection();
        ++hits;
        const int attempt = hits;
        auto* buf = new QByteArray;
        // İstek tamamen okunana kadar bekle, sonra yanıt ver
        QObject::connect(sock, &QTcpSocket::readyRead, sock, [sock, buf, attempt]() {
            buf->append(sock->readAll());
            const int headerEnd = buf->indexOf("\r\n\r\n");
            if (headerEnd < 0) return;
            // Content-Length kadar gövde beklenir
            const QByteArray head = buf->left(headerEnd);
            int contentLength = 0;
            for (const QByteArray& line : head.split('\n')) {
                if (line.toLower().startsWith("content-length:"))
                    contentLength = line.mid(15).trimmed().toInt();
            }
            if (buf->size() - (headerEnd + 4) < contentLength) return;
            QByteArray resp;
            if (attempt <= 2) { // ilk iki deneme 429 (Retry-After: 0)
                resp = "HTTP/1.1 429 Too Many Requests\r\nRetry-After: 0\r\n"
                       "Content-Type: application/json\r\nContent-Length: 2\r\n"
                       "Connection: close\r\n\r\n{}";
            } else {
                const QByteArray body = R"({"choices":[{"message":{"content":"TAMAM"},
                                   "finish_reason":"stop"}],"usage":{"prompt_tokens":1,
                                   "completion_tokens":2}})";
                resp = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " +
                       QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
            }
            sock->write(resp);
            sock->flush();
            sock->disconnectFromHost();
        });
        QObject::connect(sock, &QTcpSocket::disconnected, sock, [sock, buf]() {
            delete buf;
            sock->deleteLater();
        });
    });

    LlmClient client;
    ProviderSpec spec = ProviderRegistry::makeCustom(
        "test-retry", "Test", QString("http://127.0.0.1:%1/v1").arg(port));
    spec.quirks = QRequiresMaxTokens;
    client.setProvider(spec);
    client.setApiKey("k");

    AiChatRequest req;
    req.model = "test-model";
    req.messages << AiMessage::user("ping");
    const AiReply r = client.chatSync(req, 8000);
    CHECK(r.ok);
    CHECK(r.text == "TAMAM");
    CHECK(r.usage.promptTokens == 1 && r.usage.evalTokens == 2);
    CHECK(r.httpStatus == 200);
    CHECK(hits == 3); // 2 × 429 + 1 başarılı
    client.cancel();
}

// --- LlmClient temel davranışı (ağsız) ---
static void testClientBasics() {
    LlmClient client;
    CHECK(client.provider().id == "ollama");
    CHECK(!client.busy());
    client.setProvider(ProviderRegistry::byId("gemini"));
    CHECK(client.provider().kind == ProviderKind::Gemini);
    client.setApiKey("gk");
    CHECK(client.apiKey() == "gk");
    client.cancel();
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    // Ayar ve kayıt deposunu geçici dizine yönlendir (test izolasyonu)
    QTemporaryDir home;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, home.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, home.path());

    testRegistry();
    testUrlsAndAuth();
    testCustomProvider();
    testSecrets();
    testMessages();
    testChatRequestBodies();
    testParseReplies();
    testSse();
    testStreamEvents();
    testParseModels();
    testEmbed();
    testErrors();
    testToolBridge();
    testPrefs();
    testClientBasics();
    testLiveRetry();

    fprintf(stderr, "STAGE35: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
