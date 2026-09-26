#pragma once
#include <QFlags>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

// Stage 35: sağlayıcı türü. Yeni bir sağlayıcı eklemek için yeni tür değil,
// mevcut türlerden birini + ProviderSpec kullanmak yeterlidir.
enum class ProviderKind {
    Ollama,        // yerel, anahtar yok
    OpenAICompat,  // /v1/chat/completions şekli
    Anthropic,     // /v1/messages (ayrı system)
    Gemini,        // :generateContent (x-goog-api-key)
};

// Kimlik doğrulama stili.
enum class AuthStyle {
    None,
    Bearer,          // Authorization: Bearer <key>
    AnthropicKey,    // x-api-key: <key>
    GoogleKey,       // x-goog-api-key: <key>
    AzureApiKey,     // api-key: <key>
    CustomHeader,    // apiKeyHeader + apiKeyPrefix
};

// Sağlayıcıya özel tuhaflıklar — kod değil VERİ olarak tutulur
// (adaptörde "if (nvidia)" yazmamak için).
enum Quirk {
    QRequiresMaxTokens = 0x01, // istekte max_tokens zorunlu
    QFreeModelSuffix   = 0x02, // ":free" gibi ücretsiz model soneki
    QModelIdPrefix     = 0x04, // model kimliği sağlayıcı öneki ister
    QReasoningEffort   = 0x08, // reasoning_effort parametresi
    QNativeGateway     = 0x10, // aynı anahtarla başka protokol geçidi sunar
};
Q_DECLARE_FLAGS(Quirks, Quirk)

struct ProviderSpec {
    QString id;                    // "openai", "nvidia-nim", "unorouter"
    QString label;                 // "OpenAI", "NVIDIA NIM", "UnoRouter"
    ProviderKind kind = ProviderKind::OpenAICompat;
    QString baseUrl;               // "https://api.openai.com/v1"
    AuthStyle auth = AuthStyle::Bearer;
    QString apiKeyHeader = "Authorization";
    QString apiKeyPrefix = "Bearer ";
    QString chatPath = "/chat/completions";
    QString modelsPath = "/models";
    QString embedPath = "/embeddings";
    QStringList extraHeaders;      // "Ad: Değer"
    bool supportsVision = false;
    bool supportsTools = false;
    bool supportsEmbed = false;
    bool supportsStreaming = true;
    Quirks quirks;
    QString hint;                  // kullanıcıya gösterilen ipucu (ücretsiz kredi vb.)
    bool builtin = true;

    bool requiresKey() const { return auth != AuthStyle::None; }
    QString url(const QString& path) const;   // baseUrl + path
    bool isGateway() const { return quirks & QNativeGateway; }
    // Kimlik doğrulama başlıkları (anahtar boşsa yalnız ek başlıklar döner).
    QJsonObject authHeaders(const QString& apiKey) const;
    QJsonObject allHeaders(const QString& apiKey) const;
    // Sağlayıcı kurallarına göre model kimliği ("nvidia/x" -> "nvidia/x" vb.)
    QString resolveModelId(const QString& model) const;
    bool isFreeModel(const QString& model) const;
    QString badge() const;         // "ücretsiz" / "yerel" / "ağgeçidi" / ""
};

class ProviderRegistry {
public:
    static QList<ProviderSpec> builtin();
    static QList<ProviderSpec> custom();
    static QList<ProviderSpec> all();      // builtin + custom
    static ProviderSpec byId(const QString& id);
    static bool exists(const QString& id);
    static QStringList ids();
    static QStringList labels();
    static QList<ProviderSpec> byKind(ProviderKind k);
    // Anahtarsız (yerel) sağlayıcılar: varsayılan seçim için
    static ProviderSpec defaultProvider();

    static bool save(const ProviderSpec& s);   // custom olarak kalıcı yazar
    static bool remove(const QString& id);
    static ProviderSpec makeCustom(const QString& id, const QString& label,
                                   const QString& baseUrl, ProviderKind kind = ProviderKind::OpenAICompat);

    // HTTP durumunu sağlayıcıya özel Türkçe mesaja çevirir.
    static QString mapError(const ProviderSpec& spec, int status, const QByteArray& body);
    // Sağlayıcının önerdiği varsayılan model (model listesi boş dönerse)
    static QStringList sampleModels(const QString& id);
};
