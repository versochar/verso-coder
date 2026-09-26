#pragma once
#include "../core/AgentLoop.h"
#include "../core/AgentMemory.h"
#include "../core/AgentRunStore.h"
#include "../core/AgentTools.h"
#include "../core/ChatStore.h"
#include "../core/ContextResolver.h"
#include "../core/ConversationSummarizer.h"
#include "../core/ImageUtil.h"
#include "../core/ModelCapabilities.h"
#include "../core/OllamaClient.h"
#include "../core/PatchQueue.h"
#include "../core/RagIndexer.h"
#include "../core/SkillChain.h"
#include "../core/AgentLoop.h"
#include "../core/ai/AgentLlmAdapter.h"
#include "../core/ai/AiMessage.h"
#include "../core/ai/EmbedBridge.h"
#include "../core/ai/AiToolBridge.h"
#include "../core/ai/LlmClient.h"
#include "../core/ai/LlmProvider.h"
#include "../core/ai/ProviderPrefs.h"
#include "../core/ai/SecretStore.h"
#include "../core/TokenStats.h"
#include <QComboBox>
#include <QFutureWatcher>
#include <QHash>
#include <QJsonObject>

struct AppSettings;
class QCheckBox;
class QLabel;
class QLineEdit;
class QSpinBox;
#include <QPushButton>
#include <QTextBrowser>
#include <QWidget>

struct AiProfile {
    QString name;
    QString model;
    QString system;
    double temperature = 0.7;
    int numCtx = 4096;
};

// Stage 3: streaming + bağlam modları + hazır komutlar + RAG + profil + geçmiş + ajan.
// Stage 7 (AI Ajan 2.0): araç kullanan çok adımlı ajan, @-bağlam etiketleri,
// çoklu dosya düzenleme kuyruğu, çoklu sohbet oturumu, token/maliyet sayacı,
// sorunlardan "AI ile düzelt".
class AiPanel : public QWidget {
    Q_OBJECT
public:
    explicit AiPanel(QWidget* parent = nullptr);
    void reloadSettings();
    void setAgentMode(bool on); // Stage 7: komut paletinden ajan modu
    bool agentMode() const;

    // Sorunlar panelinden çağrılır: tanılama + kod parçasını ajana gönderir.
    void fixProblem(const QString& path, int line, const QString& message, const QString& code);
    // Stage 15: komut paletinden — istem gönder + son kod bloğunu uygula
    void send(const QString& preset = QString());
    void applyLastCodeBlock();
    // Stage 21: sohbeti markdown dosyası olarak dışa aktar
    void exportChatMarkdown();
    // Ollama'yı uygulama içinden başlat (async) + model listesini tazele
    void ensureServerAsync();
    void refreshModels();
    bool m_modelsPending = false;
    // Stage 25: iki modelle karşılaştır + son yanıtı geri al
    void sendCompare();
    void undoLastAnswer();
    // Stage 30: eski sohbetlerde metin bul
    void searchChats(const QString& query);

    // --- Stage 33 ---
    void attachImages();   // görsel ek (vision)
    void clearImages();
    void sendArena();      // çok-modelli arena
    void showBranches();   // sohbet dallanma ağacı
    void openAgentPanel(); // Stage 34: ajan paneli (bütçe/beceri/bellek/günlük)
    void runAutonomousAudit(); // Stage 34: otonom salt-okunur proje denetimi

private slots:
    void stop();
    void saveProfile();
    void deleteProfile();
    void onProfileChanged(const QString& name);
    void saveChat();
    void loadChat(const QString& file);
    void refreshHistory();

signals:
    void aiModelsChanged(bool hasModels); // Stage 25: çevrimdışı rozeti
    void currentFileRequested(QString& path, QString& content);
    void selectionRequested(QString& selected, int& start, int& len);
    void projectRootRequested(QString& root);
    void stagedDiffRequested(QString& diff);
    void applyToEditorRequested(const QString& newText, bool wholeFile, int selStart, int selLen);
    void problemsTextRequested(QString& text); // Stage 7: @sorunlar + get_problems
    void applyFileContentRequested(const QString& path, const QString& content); // Stage 7: ajan yazımı

private:
    QList<AiProfile> loadProfiles() const;
    void storeProfiles(const QList<AiProfile>& ps) const;
    void refreshProfileBox();
    QJsonObject currentOptions() const;
    QString buildContext(const QString& question);
    void appendUser(const QString& q);
    void beginAnswer(const QString& model, const AppSettings& s, const QString& prompt);
    void setBusy(bool b);
    static QString extractLastCodeBlock(const QString& text);

    // Stage 7
    void runAgent(const QString& task);
    bool approveTool(const ToolCall& c);
    void showPatchReview();
    void refreshSessions();
    void loadSession(const QString& id);
    void openSessionMenu(const QPoint& pos);
    void persistMessage(const QString& role, const QString& text);
    QString problemsText();
    // --- Stage 33 yardımcıları ---
    ModelCapabilities capsFor(const QString& model) const;
    QStringList currentImageBase64() const;
    void refreshImageChips();
    QString conversationContext();
    void configureProvider();    // Stage 35: sağlayıcıyı uygula + modelleri getir
    void refreshEmbedRoute();   // Stage 36: gömme sağlayıcısı/modelini güncelle
    bool isCloudProvider() const;
    void renderBranch(const ChatSession& s, const QString& leafId);

    OllamaClient m_client;
    LlmClient m_llm;                     // Stage 35: bulut sağlayıcıları
    EmbedBridge m_embed;                  // Stage 36: sağlayıcı-duyarsız gömme
    SecretStore m_secrets;               // Stage 35: API anahtarı kasası
    RagIndexer m_rag;
    QFutureWatcher<QPair<int, int>> m_ragWatcher; // (dosya, parça)
    QComboBox* m_profiles;
    QComboBox* m_provider;        // Stage 35
    QComboBox* m_models;
    QComboBox* m_ctxMode;
    QComboBox* m_history;
    QCheckBox* m_stream;
    QCheckBox* m_agentMode;   // Stage 7
    QCheckBox* m_agentCmd;    // Stage 7
    QSpinBox* m_agentSteps;   // Stage 7
    QLabel* m_ragLabel;
    QLabel* m_tokenLabel;     // Stage 7
    QTextBrowser* m_view;
    QLineEdit* m_input;
    QPushButton* m_send;
    QPushButton* m_stop;
    QString m_lastResponse; // ajan için son AI yanıtı (düz metin)
    QTextCursor m_streamCursor;
    bool m_streamActive = false;

    // Stage 7 durum
    ContextResolver m_ctx;
    ChatStore* m_chatStore = nullptr;
    TokenStats m_tokens;
    // Stage 25
    bool m_budgetWarned = false;
    bool m_compareArmed = false;
    QString m_compareModel;
    QString m_compareSys;
    QString m_comparePrompt;
    QJsonObject m_compareOpts;
    int m_agentMaxSteps = 5;
    QString m_activeSessionId;
    QString m_activeModel; // token maliyeti için
    PatchQueue m_patchQueue;
    bool m_agentRunning = false;

    // --- Stage 33 durum ---
    QList<PreparedImage> m_images;              // görsel ekler
    QWidget* m_chipBar = nullptr;               // ek rozet satırı
    QHash<QString, ModelCapabilities> m_caps;   // model yetenek önbelleği
    QString m_convSummary;                       // yerel sohbet özeti
    QString m_activeBranch;                      // seçili dal yaprağı
    // --- Stage 34 durum ---
    AgentMemory* m_agentMemory = nullptr;
    AgentRunStore* m_runStore = nullptr;
    SkillChain m_chain;
    bool m_forceAutonomous = false; // Stage 34: tek seferlik otonom denetim
};
