#include "AiPanel.h"
#include "../core/EmbeddingClient.h"
#include "../core/TokenStats.h"
#include "../core/ai/AiRunner.h"
#include "../core/ai/ProviderHealth.h"
#include "../core/ai/TaskRouter.h"
#include "../core/ai/UsageLedger.h"
#include "../core/ProjectHealth.h"
#include "../core/SettingsManager.h"
#include "AgentPanelDialog.h"
#include "ApplyEditDialog.h"
#include "ModelArenaDialog.h"
#include "PatchReviewDialog.h"
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QtConcurrent>
#include <memory>

static QString chatDir() {
    QString d = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/chats";
    QDir().mkpath(d);
    return d;
}

AiPanel::AiPanel(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    // Profil satırı
    auto* pRow = new QHBoxLayout();
    m_profiles = new QComboBox(this);
    m_profiles->setToolTip("Model profili");
    auto* bProfSave = new QPushButton("+", this);
    bProfSave->setFixedWidth(28);
    bProfSave->setToolTip("Mevcut ayarı profil olarak kaydet");
    auto* bProfDel = new QPushButton("−", this);
    bProfDel->setFixedWidth(28);
    bProfDel->setToolTip("Profili sil");
    pRow->addWidget(m_profiles, 1);
    pRow->addWidget(bProfSave);
    pRow->addWidget(bProfDel);

    // Model satırı
    auto* top = new QHBoxLayout();
    // Stage 35: sağlayıcı seçimi (Ollama + bulut sağlayıcıları)
    m_provider = new QComboBox(this);
    m_provider->setToolTip("AI sağlayıcısı (Ayarlar → AI Sağlayıcıları)");
    m_provider->setMinimumWidth(120);
    for (const ProviderSpec& spec : ProviderRegistry::all())
        m_provider->addItem(spec.label, spec.id);
    top->addWidget(m_provider);
    m_models = new QComboBox(this);
    m_models->setEditable(true);
    auto* bReload = new QPushButton("⟳", this);
    bReload->setFixedWidth(32);
    bReload->setToolTip("Modelleri yenile (GET /api/tags)");
    auto* bStart = new QPushButton("▶", this);
    bStart->setFixedWidth(32);
    bStart->setToolTip("Ollama kapalıysa başlat (uygulama içinden)");
    top->addWidget(m_models, 1);
    top->addWidget(bStart);
    top->addWidget(bReload);
    connect(bStart, &QPushButton::clicked, this, &AiPanel::ensureServerAsync);

    // Bağlam + stream satırı
    auto* cRow = new QHBoxLayout();
    m_ctxMode = new QComboBox(this);
    m_ctxMode->addItems({"Açık dosya", "Seçili kod", "Proje (RAG)", "Bağlamsız"});
    m_ctxMode->setToolTip("AI bağlam modu");
    m_stream = new QCheckBox("Akış", this);
    m_stream->setToolTip("Token token canlı yanıt (streaming)");
    m_stream->setChecked(true);
    auto* bIndex = new QPushButton("İndeksle", this);
    bIndex->setToolTip("Projeyi RAG için indeksle");
    cRow->addWidget(m_ctxMode, 1);
    cRow->addWidget(m_stream);
    cRow->addWidget(bIndex);
    // Ajan satırı (Stage 7)
    auto* aRow = new QHBoxLayout();
    m_agentMode = new QCheckBox("Ajan", this);
    m_agentMode->setToolTip("Araç kullanan çok adımlı ajan modu");
    m_agentSteps = new QSpinBox(this);
    m_agentSteps->setRange(1, 12);
    m_agentSteps->setValue(5);
    m_agentSteps->setToolTip("Maksimum adım");
    m_agentSteps->setFixedWidth(48);
    m_agentCmd = new QCheckBox("komut", this);
    m_agentCmd->setToolTip("Ajanın kabuk komutu çalıştırmasına izin ver (her komut ayrıca onaylanır)");
    aRow->addWidget(m_agentMode);
    aRow->addWidget(new QLabel("adım:", this));
    aRow->addWidget(m_agentSteps);
    aRow->addWidget(m_agentCmd);
    // Stage 34: ajan paneli düğmesi
    auto* bAgentPanel = new QPushButton("Panel", this);
    bAgentPanel->setToolTip("Ajan paneli: politika, bütçe, beceri zinciri, bellek, koşu günlüğü");
    connect(bAgentPanel, &QPushButton::clicked, this, &AiPanel::openAgentPanel);
    aRow->addWidget(bAgentPanel);
    aRow->addStretch(1);

    m_ragLabel = new QLabel("indeks: yok", this);
    m_ragLabel->setStyleSheet("color:#858585;font-size:11px;");
    m_tokenLabel = new QLabel("token: 0", this);
    m_tokenLabel->setStyleSheet("color:#858585;font-size:11px;");
    m_tokenLabel->setToolTip("Bu oturumdaki token kullanımı ve yaklaşık maliyet");
    auto* infoRow = new QHBoxLayout();
    infoRow->addWidget(m_ragLabel, 1);
    infoRow->addWidget(m_tokenLabel);

    m_view = new QTextBrowser(this);
    m_view->setReadOnly(true);
    m_view->setOpenLinks(false); // Stage 30: chat: bağlantıları içeride yakala
    m_view->setPlaceholderText("Ollama AI asistan. Bağlam modunu seç, sor ve Gönder'e bas.");
    // Stage 30: sohbet arama bağlantıları (chat:<id>)
    connect(m_view, &QTextBrowser::anchorClicked, this, [this](const QUrl& url) {
        if (url.scheme() == "chat" && !url.path().mid(1).isEmpty())
            loadSession(url.path().mid(1));
        else if (url.scheme().startsWith("http"))
            QDesktopServices::openUrl(url);
    });

    // Hazır komutlar
    auto* qRow = new QHBoxLayout();
    auto* bExplain = new QPushButton("Açıkla", this);
    auto* bFix = new QPushButton("Düzelt", this);
    auto* bTest = new QPushButton("Test yaz", this);
    auto* bCommit = new QPushButton("Commit msg", this);
    auto* bApply = new QPushButton("Dosyaya uygula", this);
    bApply->setToolTip("Son AI yanıtındaki kod bloğunu diff onayıyla editöre uygula");
    auto* bExport = new QPushButton("MD", this);
    bExport->setToolTip("Sohbeti markdown olarak dışa aktar");
    bExport->setFixedWidth(36);
    connect(bExport, &QPushButton::clicked, this, &AiPanel::exportChatMarkdown);
    auto* bCompare = new QPushButton("⇄", this);
    bCompare->setToolTip("İki modelle karşılaştır (geçerli soru)");
    bCompare->setFixedWidth(36);
    connect(bCompare, &QPushButton::clicked, this, &AiPanel::sendCompare);
    auto* bUndo = new QPushButton("↩", this);
    bUndo->setToolTip("Son yanıtı geri al (dallan)");
    bUndo->setFixedWidth(36);
    connect(bUndo, &QPushButton::clicked, this, &AiPanel::undoLastAnswer);
    // Stage 33: görsel ek / arena / dallar
    auto* bImg = new QPushButton("🖼", this);
    bImg->setToolTip("Görsel ekle (vision destekli modeller için)");
    bImg->setFixedWidth(36);
    connect(bImg, &QPushButton::clicked, this, &AiPanel::attachImages);
    auto* bArena = new QPushButton("⚔", this);
    bArena->setToolTip("Çok-modelli arena: aynı istemi modellere koştur");
    bArena->setFixedWidth(36);
    connect(bArena, &QPushButton::clicked, this, &AiPanel::sendArena);
    auto* bBranch = new QPushButton("⑂", this);
    bBranch->setToolTip("Sohbet dallanma ağacı");
    bBranch->setFixedWidth(36);
    connect(bBranch, &QPushButton::clicked, this, &AiPanel::showBranches);
    for (auto* b : {bExplain, bFix, bTest, bCommit, bApply}) qRow->addWidget(b);
    qRow->addWidget(bExport);
    qRow->addWidget(bCompare);
    qRow->addWidget(bUndo);
    qRow->addWidget(bImg);
    qRow->addWidget(bArena);
    qRow->addWidget(bBranch);

    auto* row = new QHBoxLayout();
    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("AI'a sor...");
    m_input->setClearButtonEnabled(true);
    m_send = new QPushButton("Gönder", this);
    m_send->setDefault(true);
    m_stop = new QPushButton("■", this);
    m_stop->setFixedWidth(32);
    m_stop->setToolTip("Durdur");
    m_stop->setEnabled(false);
    row->addWidget(m_input, 1);
    row->addWidget(m_send);
    row->addWidget(m_stop);

    // Geçmiş satırı
    auto* hRow = new QHBoxLayout();
    m_history = new QComboBox(this);
    m_history->setToolTip("Kaydedilmiş sohbetler");
    auto* bHistSave = new QPushButton("Kaydet", this);
    auto* bHistClear = new QPushButton("Temizle", this);
    hRow->addWidget(m_history, 1);
    hRow->addWidget(bHistSave);
    hRow->addWidget(bHistClear);

    auto* hint = new QLabel("Ollama kapalıysa yukarıdaki ▶ düğmesi başlatır (yoksa: `ollama serve`)", this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color:#858585;font-size:11px;");

    lay->addLayout(pRow);
    lay->addLayout(top);
    lay->addLayout(cRow);
    lay->addLayout(aRow);
    lay->addLayout(infoRow);
    // Stage 33: görsel ek rozetleri
    m_chipBar = new QWidget(this);
    m_chipBar->setVisible(false);
    auto* chipLay = new QHBoxLayout(m_chipBar);
    chipLay->setContentsMargins(0, 0, 0, 0);
    chipLay->addStretch(1);
    lay->addWidget(m_chipBar);
    lay->addWidget(m_view, 1);
    lay->addLayout(qRow);
    lay->addLayout(row);
    lay->addLayout(hRow);
    lay->addWidget(hint);

    connect(bReload, &QPushButton::clicked, this, &AiPanel::refreshModels);
    connect(m_provider, &QComboBox::currentIndexChanged, this, [this](int) {
        ProviderPrefs::setActiveProvider(m_provider->currentData().toString());
        configureProvider();
    });
    // Stage 35: LlmClient olayları → mevcut arayüze bağlanır
    connect(&m_llm, &LlmClient::modelsReady, this, [this](const QStringList& ms) {
        const QString cur = m_models->currentText();
        m_models->clear();
        m_models->addItems(ms);
        const QString want = ProviderPrefs::modelFor(m_provider->currentData().toString());
        if (!want.isEmpty() && ms.contains(want)) m_models->setCurrentText(want);
        else if (!cur.isEmpty() && ms.contains(cur)) m_models->setCurrentText(cur);
        else if (!ms.isEmpty()) m_models->setCurrentText(ms.first());
        if (!ms.isEmpty())
            m_view->append(QString("<i>✓ %1 · %2 model</i>")
                               .arg(ProviderPrefs::resolve().label)
                               .arg(ms.size()));
    });
    connect(&m_llm, &LlmClient::chunkReady, this, [this](const AiChunk& c) {
        if (m_streamActive && !c.text.isEmpty()) m_streamCursor.insertText(c.text);
        m_lastResponse += c.text;
    });
    connect(&m_llm, &LlmClient::replyReady, this, [this](const AiReply& r) {
        m_lastResponse = r.text;
        m_view->append("<hr><b>AI:</b><br>" + r.text.toHtmlEscaped().replace("\n", "<br>"));
        persistMessage("ai", r.text);
        setBusy(false);
    });
    connect(&m_llm, &LlmClient::finished, this, [this](const AiReply& r) {
        m_streamActive = false;
        if (r.ok && m_streamActive) m_view->append("<br>");
        if (!r.reasoning.isEmpty())
            m_view->append("<i style='color:#9a9a9a'>düşünme: " +
                           r.reasoning.left(400).toHtmlEscaped() + "</i>");
        if (r.ok && m_lastResponse.isEmpty()) m_lastResponse = r.text;
        if (r.ok && !m_lastResponse.isEmpty()) persistMessage("ai", m_lastResponse);
        setBusy(false);
    });
    connect(&m_llm, &LlmClient::tokensUsed, this, [this](int p, int e, int) {
        m_tokens.add(p, e);
        m_tokenLabel->setText(m_tokens.summary(m_activeModel));
    });
    connect(&m_llm, &LlmClient::error, this, [this](const QString& e) {
        m_streamActive = false;
        m_view->append("<i style='color:red'>" + e.toHtmlEscaped() + "</i>");
        setBusy(false);
    });
    connect(&m_llm, &LlmClient::statusChanged, this, [this](const QString& st) {
        if (isCloudProvider()) m_view->append("<i style='color:#858585'>" +
                                             st.toHtmlEscaped() + "</i>");
    });
    connect(m_models, &QComboBox::currentTextChanged, this, [this](const QString& t) {
        AppSettings s = SettingsManager::instance().load();
        s.ollamaModel = t;
        SettingsManager::instance().save(s);
        if (!t.isEmpty()) {
            m_caps[t] = ModelCapabilities::fromName(t); // anlık sezgi
            m_client.showModel(t);                      // yetenekleri doğrula
        }
    });
    connect(m_send, &QPushButton::clicked, this, [this]() { send(); });
    connect(m_input, &QLineEdit::returnPressed, this, [this]() { send(); });
    connect(m_stop, &QPushButton::clicked, this, &AiPanel::stop);
    connect(bProfSave, &QPushButton::clicked, this, &AiPanel::saveProfile);
    connect(bProfDel, &QPushButton::clicked, this, &AiPanel::deleteProfile);
    connect(m_profiles, &QComboBox::currentTextChanged, this, &AiPanel::onProfileChanged);
    connect(bIndex, &QPushButton::clicked, this, [this, bIndex]() {
        // Stage 31: çalışırken iptal yolu
        if (m_ragWatcher.isRunning()) {
            m_ragWatcher.cancel();
            m_ragLabel->setText("indeksleme iptal edildi");
            bIndex->setEnabled(true);
            return;
        }
        QString root;
        emit projectRootRequested(root);
        m_ragLabel->setText("indeksleniyor (arka plan)...");
        bIndex->setEnabled(false);
        m_rag.clear();
        // Stage 33: gömme modeli tanımlıysa anlamsal vektörleri de üret
        const AppSettings rs = SettingsManager::instance().load();
        // Stage 36: gömme sağlayıcısı sohbet sağlayıcısından farklı olabilir
        refreshEmbedRoute();
        const QString embedProvider = m_embed.provider();
        const QString embedModel = m_embed.model().isEmpty() ? rs.aiEmbedModel.trimmed()
                                                              : m_embed.model();
        // Stage 38: kesintili indeksleme — bekleyen ilerleme varsa kullanıcıya
        // bildir (ücretsiz katmanlarda gömme 1 istek/30 dk; tamamlama tek
        // oturuma sığmıyor).
        if (!m_ragProgress) m_ragProgress = new RagProgressStore();
        if (m_ragProgress->matchesRoot(root) && m_ragProgress->progress().valid()) {
            m_ragLabel->setText(m_ragProgress->isWaiting()
                                    ? QString("beklemede (%1 sn) — %2")
                                          .arg(m_ragProgress->waitLeftSec())
                                          .arg(m_ragProgress->progress().describe())
                                    : QString("kaldığı yerden: %1")
                                          .arg(m_ragProgress->progress().describe()));
        }
        m_ragWatcher.setFuture(QtConcurrent::run([this, root, embedModel, embedProvider]() {
            int n = m_rag.indexProjectIncremental(root); // Stage 32: artımlı
            if (!embedModel.isEmpty() && m_rag.chunkCount() > 0) {
                // Stage 36: sağlayıcı-duyarsız gömme (Ollama dâhil) + önbellek
                auto eb = std::make_shared<EmbedBridge>();
                eb->setProvider(embedProvider);
                eb->setModel(embedModel);
                eb->cache().setMaxEntries(4096);
                m_rag.setEmbedder([eb, embedModel](const QStringList& texts) {
                    const EmbedBridge::Result r = eb->embed(texts, 60000);
                    return r.vectors;
                });
                m_rag.embedAll(16);
            }
            return qMakePair(m_rag.fileCount(), n);
        }));
    });
    connect(&m_ragWatcher, &QFutureWatcher<QPair<int, int>>::finished, this, [this, bIndex]() {
        bIndex->setEnabled(true);
        if (m_ragWatcher.isCanceled()) return;
        auto r = m_ragWatcher.result();
        m_ragLabel->setText(QString("indeks: %1 dosya, %2 parça").arg(r.first).arg(r.second));
        if (m_ragProgress) m_ragProgress->clear(); // tamamlandı
    });
    connect(bExplain, &QPushButton::clicked, this, [this]() {
        send("Aşağıdaki kodu satır satır Türkçe açıkla:");
    });
    connect(bFix, &QPushButton::clicked, this, [this]() {
        send("Aşağıdaki koddaki hatayı bul ve düzeltilmiş tam kodu ``` blok içinde ver:");
    });
    connect(bTest, &QPushButton::clicked, this, [this]() {
        send("Aşağıdaki kod için birim test yaz, tam kodu ``` blok içinde ver:");
    });
    connect(bCommit, &QPushButton::clicked, this, [this]() {
        QString diff;
        emit stagedDiffRequested(diff);
        if (diff.isEmpty()) { m_view->append("<i>Staged değişiklik yok (önce Git panelinden Stage yap).</i>"); return; }
        m_view->append("<b>Sen:</b> [commit mesajı isteği]<br><i>diff bağlam olarak eklendi.</i>");
        AppSettings s = SettingsManager::instance().load();
        m_client.setHost(s.ollamaHost);
        QString model = m_models->currentText().trimmed();
        QJsonObject opts = currentOptions();
        QString prompt = "Aşağıdaki git diff için Türkçe, Conventional Commits tarzı tek satırlık commit mesajı + kısa madde listesi yaz:\n```diff\n" +
                         diff.left(6000) + "\n```";
        beginAnswer(model, s, prompt);
    });
    connect(bApply, &QPushButton::clicked, this, &AiPanel::applyLastCodeBlock);
    connect(bHistSave, &QPushButton::clicked, this, &AiPanel::saveChat);
    connect(bHistClear, &QPushButton::clicked, this, [this]() {
        m_view->clear();
        m_lastResponse.clear();
        m_activeSessionId.clear();
        m_history->setCurrentIndex(0);
    });
    connect(m_history, &QComboBox::currentIndexChanged, this, [this](int idx) {
        if (idx <= 0) return; // 0 = (yeni sohbet)
        const QString id = m_history->itemData(idx).toString();
        if (!id.isEmpty()) loadSession(id);
    });
    m_history->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_history, &QComboBox::customContextMenuRequested, this, &AiPanel::openSessionMenu);

    connect(&m_client, &OllamaClient::modelsReady, this, [this](const QStringList& ms) {        emit aiModelsChanged(!ms.isEmpty()); // Stage 25
        m_modelsPending = false;
        QString cur = m_models->currentText();
        m_models->clear();
        m_models->addItems(ms);
        if (!cur.isEmpty()) m_models->setCurrentText(cur);
        if (!ms.isEmpty())
            m_view->append(QString("<i>✓ %1 model yüklendi.</i>").arg(ms.size()));
        if (ms.isEmpty()) {
            // Stage 22: kurulum kılavuzu (boş-durum kartı)
            m_view->append(
                "<b>Ollama bulunamadı.</b><br>"
                "1) <a href=\"https://ollama.com\">ollama.com</a> adresinden kurun ve çalıştırın<br>"
                "2) Terminalde: <code>ollama pull gemma3</code> (veya Ayarlar → AI'daki model)<br>"
                "3) Yukarıdaki ▶ düğmesiyle başlatıp ⟳ ile listeyi yenileyin");
        }
    });
    connect(&m_client, &OllamaClient::modelShow, this,
            [this](const QString& model, const QJsonObject& show) {
                m_caps[model] = ModelCapabilities::detect(model, show); // Stage 33
            });
    connect(&m_client, &OllamaClient::chatReply, this, [this](const QString& t) {
        m_lastResponse = t;
        m_view->append("<hr><b>AI:</b><br>" + t.toHtmlEscaped().replace("\n", "<br>"));
        persistMessage("ai", t);
        setBusy(false);
    });
    connect(&m_client, &OllamaClient::chatToken, this, [this](const QString& tok) {
        if (!m_streamActive) return;
        m_streamCursor.insertText(tok);
        m_lastResponse += tok;
    });
    connect(&m_client, &OllamaClient::chatFinished, this, [this](const QString&) {
        m_streamActive = false;
        m_view->append("<br>");
        persistMessage("ai", m_lastResponse);
        setBusy(false);
        // Stage 25: karşılaştırmanın B ayağı (akışsız tek yanıt)
        if (m_compareArmed && !m_compareModel.isEmpty()) {
            m_compareArmed = false;
            setBusy(true);
            m_view->append("<hr><i>B yanıtı yazıyor...</i>");
            m_client.chat(m_compareModel, m_compareSys, m_comparePrompt, m_compareOpts);
        }
    });
    connect(&m_client, &OllamaClient::tokensUsed, this, [this](int p, int e) {
        m_tokens.add(p, e);
        m_tokenLabel->setText(m_tokens.summary(m_activeModel));
        // Stage 25: oturum token bütçesi
        const int budget = SettingsManager::instance().load().aiTokenBudget;
        if (budget > 0 && !m_budgetWarned && m_tokens.total() > budget) {
            m_budgetWarned = true;
            m_view->append(QString("<i style='color:#f44747'>⚠ Token bütçesi aşıldı "
                                   "(%1 &gt; %2).</i>")
                               .arg(m_tokens.total())
                               .arg(budget));
        }
    });
    connect(&m_client, &OllamaClient::error, this, [this](const QString& e) {
        m_streamActive = false;
        m_view->append("<i style='color:red'>Hata: " + e.toHtmlEscaped() + "</i>");
        setBusy(false);
    });

    m_chatStore = new ChatStore(chatDir());
    // Stage 34: ajan belleği + koşu günlüğü
    const QString agentDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                             + "/agent";
    QDir().mkpath(agentDir);
    m_agentMemory = new AgentMemory(agentDir + "/memory.json");
    m_runStore = new AgentRunStore(agentDir + "/runs");
    reloadSettings();
    refreshHistory();
}

// --- Profil yönetimi (QSettings JSON) ---
QList<AiProfile> AiPanel::loadProfiles() const {
    QList<AiProfile> out;
    AppSettings s = SettingsManager::instance().load();
    QJsonDocument d = QJsonDocument::fromJson(s.profilesJson.toUtf8());
    if (d.isArray())
        for (const QJsonValue& v : d.array()) {
            QJsonObject o = v.toObject();
            AiProfile p;
            p.name = o["name"].toString();
            p.model = o["model"].toString();
            p.system = o["system"].toString();
            p.temperature = o["temperature"].toDouble(0.7);
            p.numCtx = o["num_ctx"].toInt(4096);
            if (!p.name.isEmpty()) out << p;
        }
    if (out.isEmpty()) {
        // İlk çalıştırma: mevcut ayarlardan varsayılan profil tohumla
        AiProfile p;
        p.name = "default";
        p.model = s.ollamaModel;
        p.system = s.systemPrompt;
        p.temperature = s.temperature;
        p.numCtx = s.contextWindow;
        out << p;
    }
    return out;
}

void AiPanel::storeProfiles(const QList<AiProfile>& ps) const {
    QJsonArray a;
    for (const auto& p : ps)
        a.append(QJsonObject{{"name", p.name}, {"model", p.model}, {"system", p.system},
                             {"temperature", p.temperature}, {"num_ctx", p.numCtx}});
    AppSettings s = SettingsManager::instance().load();
    s.profilesJson = QString::fromUtf8(QJsonDocument(a).toJson(QJsonDocument::Compact));
    SettingsManager::instance().save(s);
}

void AiPanel::refreshProfileBox() {
    AppSettings s = SettingsManager::instance().load();
    m_profiles->blockSignals(true);
    m_profiles->clear();
    for (const auto& p : loadProfiles()) m_profiles->addItem(p.name);
    m_profiles->setCurrentText(s.activeProfile);
    m_profiles->blockSignals(false);
}

void AiPanel::saveProfile() {
    QString name = QInputDialog::getText(this, "Profil kaydet", "Profil adı:");
    if (name.trimmed().isEmpty()) return;
    AppSettings s = SettingsManager::instance().load();
    auto ps = loadProfiles();
    AiProfile p{name.trimmed(), m_models->currentText().trimmed(), s.systemPrompt,
                s.temperature, s.contextWindow};
    bool replaced = false;
    for (auto& q : ps)
        if (q.name == p.name) { q = p; replaced = true; }
    if (!replaced) ps << p;
    s.activeProfile = p.name;
    SettingsManager::instance().save(s);
    storeProfiles(ps);
    refreshProfileBox();
    m_profiles->setCurrentText(p.name);
}

void AiPanel::deleteProfile() {
    QString name = m_profiles->currentText();
    if (name.isEmpty() || name == "default") return;
    auto ps = loadProfiles();
    ps.erase(std::remove_if(ps.begin(), ps.end(),
                            [&](const AiProfile& p) { return p.name == name; }),
             ps.end());
    AppSettings s = SettingsManager::instance().load();
    s.activeProfile = "default";
    SettingsManager::instance().save(s);
    storeProfiles(ps);
    refreshProfileBox();
}

void AiPanel::onProfileChanged(const QString& name) {
    if (name.isEmpty()) return;
    for (const auto& p : loadProfiles())
        if (p.name == name) {
            AppSettings s = SettingsManager::instance().load();
            s.activeProfile = name;
            s.ollamaModel = p.model;
            s.systemPrompt = p.system;
            s.temperature = p.temperature;
            s.contextWindow = p.numCtx;
            SettingsManager::instance().save(s);
            m_models->setCurrentText(p.model);
            break;
        }
}

void AiPanel::setAgentMode(bool on) {
    if (m_agentMode) m_agentMode->setChecked(on);
}

bool AiPanel::agentMode() const {
    return m_agentMode && m_agentMode->isChecked();
}

void AiPanel::reloadSettings() {
    m_embed.setSecretStore(&m_secrets); // Stage 36
    AppSettings s = SettingsManager::instance().load();
    m_client.setHost(s.ollamaHost);
    if (m_models->findText(s.ollamaModel) < 0) m_models->addItem(s.ollamaModel);
    m_models->setCurrentText(s.ollamaModel);
    m_stream->setChecked(s.aiStreaming);
    int ctxIdx = (s.contextMode == "selection") ? 1 : (s.contextMode == "rag") ? 2 : (s.contextMode == "none") ? 3 : 0;
    m_ctxMode->setCurrentIndex(ctxIdx);
    m_client.setKeepAlive(s.aiKeepAlive); // Stage 33: modeli sıcak tut
    // Stage 35: sağlayıcı
    {
        const QString active = ProviderPrefs::activeProvider();
        const int i = m_provider ? m_provider->findData(active) : -1;
        if (i >= 0) {
            m_provider->blockSignals(true);
            m_provider->setCurrentIndex(i);
            m_provider->blockSignals(false);
        }
        configureProvider();
    }
    if (!s.ollamaModel.isEmpty()) {
        m_caps[s.ollamaModel] = ModelCapabilities::fromName(s.ollamaModel);
        m_client.showModel(s.ollamaModel);
    }
    refreshProfileBox();
    refreshModels();
}

// Stage 35: aktif sağlayıcıyı uygular ve model listesini getirir
void AiPanel::configureProvider() {
    if (!m_provider) return;
    const ProviderSpec spec = ProviderPrefs::resolve(m_provider->currentData().toString());
    const bool local = spec.kind == ProviderKind::Ollama;
    // Yerel sağlayıcıda eski Ollama yolu; bulutta LlmClient
    m_llm.setProvider(spec);
    m_llm.setSecretStore(&m_secrets);
    m_llm.loadKeyForProvider();
    if (local) {
        AppSettings s = SettingsManager::instance().load();
        m_client.setHost(local ? ProviderPrefs::urlFor(spec.id) : s.ollamaHost);
        refreshModels();
    } else {
        if (m_models->count() <= 1) m_llm.fetchModels();
    }
    // Stage 36: gömme rotası ayrı olabilir (ör. sohbet Ollama, gömme NIM)
    refreshEmbedRoute();
}

bool AiPanel::isCloudProvider() const {
    if (!m_provider) return false;
    return ProviderPrefs::resolve(m_provider->currentData().toString()).kind !=
           ProviderKind::Ollama;
}

// Stage 36: gömme sağlayıcısı/modelini yönlendirme tercihine göre günceller.
// Sohbet sağlayıcısı gömme desteklemiyorsa destekleyen ilkine geçilir.
void AiPanel::refreshEmbedRoute() {
    const TaskRouter::Prefs p = TaskRouter::prefs();
    m_embed.setProvider(TaskRouter::embedProvider(p));
    const QString model = TaskRouter::embedModel(p);
    if (!model.isEmpty()) m_embed.setModel(model);
}

QJsonObject AiPanel::currentOptions() const {
    AppSettings s = SettingsManager::instance().load();
    int numGpu = (s.gpuBackend == "CPU") ? 0 : s.gpuLayers;
    QJsonObject opts;
    opts["num_ctx"] = s.contextWindow;
    opts["temperature"] = s.temperature;
    opts["num_thread"] = s.cpuThreads;
    opts["num_gpu"] = numGpu;
    return opts;
}

QString AiPanel::buildContext(const QString& question) {
    int mode = m_ctxMode->currentIndex(); // 0 dosya, 1 seçim, 2 rag, 3 yok
    QString base;
    if (mode == 1) {
        QString sel; int st = 0, ln = 0;
        emit selectionRequested(sel, st, ln);
        base = sel.isEmpty() ? QString("\n(Not: seçili kod yok.)\n")
                             : QString("\n[Seçili kod]\n```\n%1\n```\n").arg(sel.left(12000));
    } else if (mode == 2) {
        QString root;
        emit projectRootRequested(root);
        if (m_rag.isEmpty()) m_rag.indexProject(root);
        QString q = question.isEmpty() ? m_input->text() : question;
        if (q.isEmpty()) q = "main";
        // Stage 33: hibrit (anahtar kelime + anlamsal) RAG
        const AppSettings rs = SettingsManager::instance().load();
        QList<RagChunk> hits;
        bool semantic = false;
        if (m_rag.vectorCount() > 0 && !m_embed.model().isEmpty()) {
            const EmbedBridge::Result er = m_embed.embedOne(q, 8000);
            if (er.ok()) {
                hits = m_rag.queryHybrid(q, er.vectors.first(), 4);
                semantic = true;
            }
        }
        if (hits.isEmpty() && !semantic) hits = m_rag.query(q, 4);
        if (hits.isEmpty()) {
            base = "\n(Not: RAG indeksinde ilgili parça bulunamadı.)\n";
        } else {
            m_ragLabel->setText(QString("indeks: %1 dosya, %2 parça (%3)")
                                    .arg(m_rag.fileCount()).arg(m_rag.chunkCount())
                                    .arg(semantic ? "hibrit" : "RAG kullanıldı"));
            base = "\n[Proje bağlamı (RAG)]\n" + RagIndexer::formatContext(hits) + "\n";
        }
    } else if (mode == 0) {
        QString path, content;
        emit currentFileRequested(path, content);
        base = content.isEmpty() ? QString("\n(Not: açık dosya yok.)\n")
                                 : QString("\n[Dosya: %1]\n```\n%2\n```\n").arg(path, content.left(12000));
    }

    // Stage 7: @-bağlam etiketleri
    if (!ContextResolver::hasMentions(question)) return base + conversationContext();
    QString root;
    emit projectRootRequested(root);
    QString curPath, curContent;
    emit currentFileRequested(curPath, curContent);
    QString sel; int st = 0, ln = 0;
    emit selectionRequested(sel, st, ln);
    m_ctx.projectSearcher = [root](const QString& q) {
        AgentTools t(root);
        return t.search(q).output;
    };
    auto rr = m_ctx.resolve(question, root, curPath, sel, problemsText());
    if (!rr.warnings.isEmpty())
        m_view->append("<i style='color:#cca700'>" + rr.warnings.join("<br>") + "</i>");
    return base + rr.text + conversationContext();
}

QString AiPanel::problemsText() {
    QString p;
    emit problemsTextRequested(p);
    return p;
}

void AiPanel::appendUser(const QString& q) {
    m_view->append("<b>Sen:</b> " + q.toHtmlEscaped().replace("\n", "<br>"));
}

void AiPanel::setBusy(bool b) {
    m_send->setEnabled(!b);
    m_stop->setEnabled(b);
    if (!b) { m_streamActive = false; }
}

void AiPanel::send(const QString& preset) {
    QString extra = m_input->text().trimmed();
    if (preset.isEmpty() && extra.isEmpty()) return;
    m_budgetWarned = false; // Stage 25
    AppSettings s = SettingsManager::instance().load();
    s.aiStreaming = m_stream->isChecked();
    static const char* modes[] = {"file", "selection", "rag", "none"};
    s.contextMode = modes[m_ctxMode->currentIndex()];
    SettingsManager::instance().save(s);
    m_client.setHost(s.ollamaHost);
    QString model = m_models->currentText().trimmed();
    if (model.isEmpty()) { m_view->append("<i>Önce model seç.</i>"); return; }
    m_activeModel = model;

    QString prompt;
    if (preset.isEmpty())
        prompt = extra + "\n" + buildContext(extra);
    else
        prompt = preset + "\n" + buildContext(extra) + (extra.isEmpty() ? "" : "\nEk not: " + extra);
    const QString shown = preset.isEmpty() ? extra : preset.split('\n').first();
    appendUser(extra.isEmpty() || preset.isEmpty() ? shown : shown + "\n" + extra);
    persistMessage("user", extra.isEmpty() ? shown : shown + "\n" + extra);
    m_input->clear();

    if (m_agentMode->isChecked()) { runAgent(prompt); return; }
    beginAnswer(model, s, prompt);
}

void AiPanel::beginAnswer(const QString& model, const AppSettings& s, const QString& prompt) {
    setBusy(true);
    m_lastResponse.clear();

    // --- Stage 35: bulut sağlayıcıları (Ollama yolundan ayrı) ---
    if (isCloudProvider()) {
        const ProviderSpec spec = ProviderPrefs::resolve();
        AiChatRequest r;
        r.model = model;
        r.systemPrompt = s.systemPrompt;
        r.temperature = s.temperature;
        const QStringList imgs = currentImageBase64();
        if (!imgs.isEmpty() && spec.supportsVision) {
            AiMessage m;
            m.role = AiRole::User;
            m.texts = QStringList{prompt};
            for (const QString& b : imgs) {
                AiImage im;
                im.mime = "image/jpeg";
                im.bytes = QByteArray::fromBase64(b.toLatin1());
                m.images << im;
            }
            r.messages << m;
        } else {
            if (!imgs.isEmpty())
                m_view->append(QString("<i style='color:#cca700'>%1</i>")
                                   .arg(AiRunner::imageBlockReason(spec.id, imgs.size())));
            r.messages << AiMessage::user(prompt);
        }
        if (m_stream->isChecked()) {
            m_view->append("<hr><b>AI:</b><br>");
            m_streamCursor = QTextCursor(m_view->document());
            m_streamCursor.movePosition(QTextCursor::End);
            m_streamActive = true;
            m_llm.chatStream(r);
        } else {
            m_view->append("<i>yazıyor...</i>");
            m_llm.chat(r);
        }
        if (!imgs.isEmpty()) clearImages();
        return;
    }

    const QStringList imgs = currentImageBase64(); // Stage 33
    bool vision = false;
    if (!imgs.isEmpty()) {
        vision = s.aiVisionEnabled && capsFor(model).vision;
        if (!vision)
            m_view->append("<i style='color:#cca700'>Bu model görsel desteklemiyor; "
                           "ekler yok sayıldı.</i>");
    }
    if (m_stream->isChecked()) {
        m_view->append("<hr><b>AI:</b><br>");
        m_streamCursor = QTextCursor(m_view->document());
        m_streamCursor.movePosition(QTextCursor::End);
        m_streamActive = true;
        if (vision)
            m_client.chatStreamWithImages(model, s.systemPrompt, prompt, currentOptions(), imgs);
        else
            m_client.chatStream(model, s.systemPrompt, prompt, currentOptions());
    } else {
        m_view->append("<i>yazıyor...</i>");
        if (vision)
            m_client.chatWithImages(model, s.systemPrompt, prompt, currentOptions(), imgs);
        else
            m_client.chat(model, s.systemPrompt, prompt, currentOptions());
    }
    if (!imgs.isEmpty()) clearImages();
}

void AiPanel::stop() {
    m_client.cancelStream();
    m_streamActive = false;
    m_view->append("<i>(durduruldu)</i>");
    setBusy(false);
}

// --- Sohbet oturumları (Stage 7: çoklu sohbet) ---
void AiPanel::saveChat() {
    if (!m_chatStore) return;
    if (m_activeSessionId.isEmpty()) {
        const QString txt = m_view->toPlainText().trimmed();
        if (txt.isEmpty()) return;
        m_activeSessionId = m_chatStore->createSession(txt.left(40).replace('\n', ' '));
        persistMessage("ai", txt);
        refreshHistory();
        return;
    }
    bool ok = false;
    const QString cur = m_chatStore->titleFor(m_activeSessionId);
    QString name = QInputDialog::getText(this, "Oturumu yeniden adlandır", "Başlık:",
                                         QLineEdit::Normal, cur, &ok);
    if (ok && !name.trimmed().isEmpty()) {
        m_chatStore->rename(m_activeSessionId, name.trimmed());
        refreshHistory();
    }
}

void AiPanel::loadChat(const QString& file) {
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QString t = QString::fromUtf8(f.readAll());
    m_view->setHtml(t.toHtmlEscaped().replace("\n", "<br>"));
}

// Stage 25: iki modelle karşılaştırma — A akar, bitince B (akışsız) koşar
void AiPanel::sendCompare() {
    const QString extra = m_input->text().trimmed();
    if (extra.isEmpty()) {
        m_view->append("<i>Önce soruyu yazın.</i>");
        return;
    }
    if (m_models->count() < 2) {
        m_view->append("<i>Karşılaştırma için en az 2 model gerekli (⟳ ile yükleyin).</i>");
        return;
    }
    const QString a = m_models->currentText().trimmed();
    QString b;
    for (int i = 0; i < m_models->count(); ++i)
        if (m_models->itemText(i) != a) { b = m_models->itemText(i); break; }
    if (a.isEmpty() || b.isEmpty()) return;
    AppSettings s = SettingsManager::instance().load();
    m_compareModel = b;
    m_compareSys = s.systemPrompt;
    m_comparePrompt = extra + "\n" + buildContext(extra);
    m_compareOpts = currentOptions();
    m_compareArmed = true;
    m_view->append(QString("<hr><b>⇄ Karşılaştırma:</b> %1 vs %2").arg(a.toHtmlEscaped(), b.toHtmlEscaped()));
    send();
}

// Stage 30: eski sohbetlerde metin bul
void AiPanel::searchChats(const QString& query) {
    if (!m_chatStore || query.isEmpty()) return;
    struct Hit { QString id; QString title; QString snippet; };
    QList<Hit> hits;
    for (const ChatSession& s : m_chatStore->sessions()) {
        const ChatSession full = m_chatStore->load(s.id);
        for (const ChatMessage& m : full.messages) {
            const int at = m.text.indexOf(query, 0, Qt::CaseInsensitive);
            if (at < 0) continue;
            const int from = qMax(0, at - 40);
            hits << Hit{s.id, full.title.isEmpty() ? s.id : full.title,
                        m.text.mid(from, 120).replace('\n', ' ')};
            break;
        }
        if (hits.size() >= 50) break;
    }
    if (hits.isEmpty()) {
        m_view->append("<i>Sohbetlerde bulunamadı: " + query.toHtmlEscaped() + "</i>");
        return;
    }
    QString html = "<b>Sohbet arama sonuçları:</b><br>";
    for (int i = 0; i < hits.size(); ++i)
        html += QString("%1. <a href='chat:%2'>%3</a> — %4<br>")
                    .arg(i + 1)
                    .arg(hits[i].id.toHtmlEscaped(), hits[i].title.toHtmlEscaped(),
                         hits[i].snippet.toHtmlEscaped());
    m_view->append(html);
}

// Stage 25: son AI yanıtını geri al — soru girişe döner (dallanma)
void AiPanel::undoLastAnswer() {
    if (!m_chatStore || m_activeSessionId.isEmpty()) return;
    ChatSession sess = m_chatStore->load(m_activeSessionId);
    if (sess.messages.isEmpty()) return;
    QString lastUser;
    while (!sess.messages.isEmpty() && sess.messages.last().role != "user")
        sess.messages.removeLast();
    if (!sess.messages.isEmpty()) lastUser = sess.messages.last().text;
    m_chatStore->save(sess);
    m_input->setText(lastUser);
    m_input->setFocus();
    m_convSummary.clear();     // Stage 33: özeti geçersiz kıl
    m_activeBranch.clear();    // yeni gönderim doğrusal devam etsin
    m_view->append("<hr><i>↩ Son yanıt geri alındı — soru yukarıda, değiştirip tekrar gönderin.</i>");
}

// Uygulama içinden Ollama başlatma (arayüzü dondurmaz)
void AiPanel::ensureServerAsync() {
    AppSettings s = SettingsManager::instance().load();
    m_client.setHost(s.ollamaHost);
    const QString host = s.ollamaHost;
    m_view->append("<i>Ollama denetleniyor/bağlatılıyor...</i>");
    QFutureWatcher<bool>* w = new QFutureWatcher<bool>(this);
    connect(w, &QFutureWatcher<bool>::finished, this, [this, w, host]() {
        const bool ok = w->result();
        w->deleteLater();
        if (ok) {
            m_view->append("<i>Ollama hazır ✓ — modeller yükleniyor...</i>");
            emit aiModelsChanged(true);
            refreshModels();
        } else {
            m_view->append("<i>Ollama başlatılamadı (ikili bulunamadı ya da port dolu). "
                           "El ile: <code>ollama serve</code></i>");
        }
    });
    w->setFuture(QtConcurrent::run([host]() { return OllamaClient::ensureServer(host); }));
}

void AiPanel::refreshModels() {
    AppSettings s = SettingsManager::instance().load();
    m_client.setHost(s.ollamaHost);
    m_modelsPending = true;
    m_client.fetchModels();
    // Yanıt gelmezse takılı kalma: 10 sn sonra uyar
    QTimer::singleShot(10000, this, [this]() {
        if (m_modelsPending) {
            m_modelsPending = false;
            m_view->append("<i>Model listesi yanıt vermedi — ⟳ ile tekrar deneyin.</i>");
        }
    });
}

// Stage 21: etkin oturumu markdown olarak dışa aktar
void AiPanel::exportChatMarkdown() {
    QString md = "# Verso AI Sohbeti\n\n";
    bool has = false;
    if (m_chatStore && !m_activeSessionId.isEmpty()) {
        const ChatSession s = m_chatStore->load(m_activeSessionId);
        if (!s.title.isEmpty()) md += "## " + s.title + "\n\n";
        for (const ChatMessage& m : s.messages) {
            const QString who = (m.role == "user") ? "Sen" : "AI";
            md += "### " + who + "\n\n" + m.text + "\n\n";
            has = true;
        }
    }
    if (!has) {
        const QString t = m_view->toPlainText().trimmed();
        if (t.isEmpty()) return;
        md += t + "\n";
    }
    const QString p = QFileDialog::getSaveFileName(this, "Sohbeti Dışa Aktar", "",
                                                   "Markdown (*.md)");
    if (p.isEmpty()) return;
    QFile f(p);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) f.write(md.toUtf8());
}

void AiPanel::refreshHistory() {
    if (!m_chatStore) return;
    m_history->blockSignals(true);
    m_history->clear();
    m_history->addItem("＋ (yeni sohbet)");
    int select = 0;
    for (const ChatSession& s : m_chatStore->sessions()) {
        m_history->addItem(s.title.isEmpty() ? s.id : s.title);
        m_history->setItemData(m_history->count() - 1, s.id);
        if (s.id == m_activeSessionId) select = m_history->count() - 1;
    }
    m_history->setCurrentIndex(select);
    m_history->blockSignals(false);
}

void AiPanel::loadSession(const QString& id) {
    if (!m_chatStore) return;
    ChatSession s = m_chatStore->load(id);
    if (s.id.isEmpty()) return;
    m_activeSessionId = id;
    m_view->clear();
    for (const ChatMessage& m : s.messages) {
        if (m.role == "user")
            m_view->append("<b>Sen:</b> " + m.text.toHtmlEscaped().replace("\n", "<br>"));
        else
            m_view->append("<hr><b>AI:</b><br>" + m.text.toHtmlEscaped().replace("\n", "<br>"));
    }
    if (!s.messages.isEmpty()) m_lastResponse = s.messages.last().text;
    // Stage 33: dal ve özet durumunu sıfırla
    m_activeBranch = s.messages.isEmpty() ? QString() : s.messages.last().id;
    m_convSummary.clear();
}

void AiPanel::openSessionMenu(const QPoint& pos) {
    if (!m_chatStore) return;
    const int idx = m_history->currentIndex();
    if (idx <= 0) return;
    const QString id = m_history->itemData(idx).toString();
    if (id.isEmpty()) return;
    QMenu menu(this);
    QAction* aOpen = menu.addAction("Aç");
    QAction* aRename = menu.addAction("Yeniden adlandır");
    QAction* aDelete = menu.addAction("Sil");
    QAction* chosen = menu.exec(m_history->mapToGlobal(pos));
    if (chosen == aOpen) {
        loadSession(id);
    } else if (chosen == aRename) {
        bool ok = false;
        QString name = QInputDialog::getText(this, "Yeniden adlandır", "Başlık:",
                                             QLineEdit::Normal, m_history->currentText(), &ok);
        if (ok && !name.trimmed().isEmpty()) {
            m_chatStore->rename(id, name.trimmed());
            refreshHistory();
        }
    } else if (chosen == aDelete) {
        m_chatStore->remove(id);
        if (m_activeSessionId == id) m_activeSessionId.clear();
        refreshHistory();
    }
}

void AiPanel::persistMessage(const QString& role, const QString& text) {
    if (!m_chatStore || text.trimmed().isEmpty()) return;
    if (m_activeSessionId.isEmpty()) {
        QString title = text.trimmed().left(40).replace('\n', ' ');
        m_activeSessionId = m_chatStore->createSession(title);
    }
    ChatMessage m;
    m.role = role;
    m.text = text;
    m.whenMs = QDateTime::currentMSecsSinceEpoch();
    // Stage 33: etkin dala bağla (yoksa doğrusal)
    if (m_activeBranch.isEmpty())
        m_chatStore->append(m_activeSessionId, m);
    else
        m_activeBranch = m_chatStore->appendMsg(m_activeSessionId, m_activeBranch, m);
    refreshHistory();
}

// --- Stage 7: araç kullanan ajan ---
bool AiPanel::approveTool(const ToolCall& c) {
    if (c.name != "write_file" && c.name != "run_command" && c.name != "run_tests") return true;
    QString detail;
    if (c.name == "write_file") {
        const QString path = c.args.value("path").toString();
        const QString content = c.args.value("content").toString();
        detail = QString("Dosya: %1\nBoyut: %2 bayt\n\n--- ilk satırlar ---\n%3")
                     .arg(path).arg(content.toUtf8().size()).arg(content.left(800));
    } else {
        detail = "Komut:\n" + c.args.value("command").toString();
    }
    QMessageBox box(this);
    box.setWindowTitle("Ajan onayı");
    box.setIcon(QMessageBox::Question);
    box.setText(QString("Ajan '%1' aracını kullanmak istiyor. Onaylıyor musun?").arg(c.name));
    box.setDetailedText(detail);
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.setDefaultButton(QMessageBox::No);
    return box.exec() == QMessageBox::Yes;
}

void AiPanel::showPatchReview() {
    PatchReviewDialog d(this);
    QSettings st;
    const int threshold = st.value("agent/queueReviewThreshold", 5).toInt();
    d.setBulkWarning(threshold);
    d.setEdits(m_patchQueue.edits());
    const int n = m_patchQueue.count();
    if (d.exec() != QDialog::Accepted) {
        m_view->append(QString("<i>Ajan düzenlemeleri reddedildi (%1 dosya).</i>").arg(n));
        m_patchQueue.clear();
        return;
    }
    QList<QueuedEdit> sel = d.selectedEdits();
    for (const QueuedEdit& e : sel) emit applyFileContentRequested(e.path, e.newText);
    m_view->append(QString("<i>%1 dosya düzenlemesi uygulandı.</i>").arg(int(sel.size())));
    m_patchQueue.clear();
}

void AiPanel::runAgent(const QString& task) {
    if (m_agentRunning) return;
    AppSettings s = SettingsManager::instance().load();
    m_client.setHost(s.ollamaHost);
    QString model = m_models->currentText().trimmed();
    if (model.isEmpty()) { m_view->append("<i>Önce model seç.</i>"); return; }
    m_activeModel = model;
    QString root;
    emit projectRootRequested(root);
    if (root.isEmpty()) {
        m_view->append("<i>Ajan için önce bir proje klasörü aç.</i>");
        return;
    }

    // Stage 36: göreve göre sağlayıcı/model yönlendirmesi
    const TaskRouter::Route route = TaskRouter::route(task);
    const QString agentProvider = route.providerId.isEmpty()
                                      ? ProviderPrefs::activeProvider()
                                      : route.providerId;
    if (!route.model.isEmpty() && !route.forceLocked) model = route.model;
    const ProviderSpec agentSpec = ProviderPrefs::resolve(agentProvider);
    if (model.isEmpty())
        model = ProviderPrefs::modelFor(agentProvider,
                                        ProviderRegistry::sampleModels(agentProvider).value(0));
    if (model.isEmpty()) { m_view->append("<i>Önce model seç.</i>"); return; }
    if (agentProvider != m_provider->currentData().toString()) {
        m_view->append(QString("<i>Yönlendirme: %1 görev → %2 · %3</i>")
                           .arg(TaskRouter::label(route.cls), agentSpec.label,
                                route.reason.toHtmlEscaped()));
    }
    // Kota denetimi: aşılıysa ajan hiç başlamaz
    {
        QString why;
        if (UsageLedger::instance().quotaExceeded(agentProvider, why)) {
            m_view->append("<i style='color:#f44747'>" + why.toHtmlEscaped() +
                           " — Ayarlar → AI Sağlayıcıları → Kota</i>");
            return;
        }
    }

    // Stage 25: plan kapısı — çalıştırmadan önce onay
    {
        const int steps = m_agentSteps ? m_agentSteps->value() : 10;
        const double estUsd = AgentLlmAdapter::estimateUsd(agentProvider, model, steps);
        QString plan = QString("Görev: %1\n\nEn çok %2 adım, %3 araç kullanılabilir.\n"
                               "Sağlayıcı: %4 · %5\nMaliyet: %6\n\nBaşlansın mı?")
                           .arg(task.left(300))
                           .arg(steps)
                           .arg("dosya/ara/komut/yama")
                           .arg(agentSpec.label, model,
                                AgentLlmAdapter::costPreview(agentProvider, model, steps));
        if (TokenStats::needsConfirmation(estUsd))
            plan += QString("\n\n⚠ Bu koşu maliyet tavanını ($%1) aşabilir.")
                        .arg(TokenStats::costGuardUsd(), 0, 'f', 2);
        auto r = QMessageBox::question(this, "Ajan Planı", plan,
                                       QMessageBox::Yes | QMessageBox::Cancel);
        if (r != QMessageBox::Yes) {
            m_view->append("<i>Ajan iptal edildi (plan onaylanmadı).</i>");
            return;
        }
        m_agentMaxSteps = steps;
    }
    AgentTools tools(root);
    // Stage 38: komut güvenliği ayarlarını uygula
    {
        QSettings st;
        tools.setShellMode(st.value("agent/shellSecure", true).toBool()
                               ? AgentTools::ShellMode::Secure
                               : AgentTools::ShellMode::Legacy);
        tools.setAuditEnabled(st.value("agent/audit", true).toBool());
        tools.setQueueReviewThreshold(st.value("agent/queueReviewThreshold", 5).toInt());
        // Mutlak yol içeren komut varsa kullanıcıya önceden haber ver
        const QString warn = AgentTools::pathWarning(QString());
        Q_UNUSED(warn);
    }
    tools.setProblemsProvider([this]() { return problemsText(); });
    tools.setAllowCommand(m_agentCmd->isChecked());
    tools.setWriteMode(AgentTools::Queue);
    m_patchQueue.clear();
    tools.setPatchQueue(&m_patchQueue);
    tools.setGitRepo(QFileInfo(root + "/.git").exists());
    tools.setTestCommand(s.agentTestCommand.trimmed().isEmpty()
                             ? AgentTools::guessTestCommand(root)
                             : s.agentTestCommand.trimmed());

    // --- Stage 34: politika + bütçe + bellek + beceri zinciri ---
    AgentRunContext ctx;
    ctx.goal = task;
    ctx.memory = s.agentMemory ? m_agentMemory : nullptr;
    if (s.agentAutonomous || m_forceAutonomous) {
        ctx.policy = AgentPolicy::autonomousReadOnly(m_agentMaxSteps);
        m_view->append("<i style='color:#cca700'>Otonom mod: yalnızca okuma araçları kullanılacak.</i>");
    } else {
        ctx.policy = AgentPolicy::safeDefault();
        ctx.policy.allowWrite = true;  // yazma onay kuyruğuna
        ctx.policy.allowCommand = m_agentCmd->isChecked();
        ctx.policy.allowTests = s.agentTestCommand.trimmed().isEmpty() ||
                                !AgentTools::guessTestCommand(root).isEmpty();
        ctx.policy.maxSteps = m_agentMaxSteps;
    }
    ctx.budget.maxSteps = m_agentMaxSteps;
    ctx.budget.maxToolCalls = s.agentMaxToolCalls;
    ctx.budget.maxWrites = s.agentMaxWrites;
    ctx.budget.maxTokens = s.agentMaxTokens;
    ctx.budget.maxMs = qint64(s.agentMaxMinutes) * 60000LL;
    if (s.agentSkills) {
        m_chain = SkillChain::forGoal(task, SkillRegistry::all(), 4);
        ctx.skillChainPrompt = m_chain.toPrompt(task);
        if (!m_chain.isEmpty())
            m_view->append("<i>Beceri zinciri: " + m_chain.skillNames().join(" → ") + "</i>");
    } else {
        m_chain.clear();
        ctx.skillChainPrompt.clear();
    }
    m_view->append(QString("<i>Politika: %1 · %2</i>").arg(ctx.policy.summary(), ctx.budget.summary()));

    m_agentRunning = true;
    setBusy(true);
    m_view->append("<hr><b>🤖 Ajan:</b> <i>görev işleniyor…</i>");
    const QString system = s.systemPrompt;
    const QJsonObject opts = currentOptions();
    // Stage 36: tek adaptör — Ollama dahil her sağlayıcıda aynı yol.
    // Sağlayıcı yönlendirmesi, kota denetimi, sağlık/kullanım kaydı ve
    // native araç çağırma → metin protokolü dönüşümü burada toplanır.
    AgentTools toolDefs(root);
    const auto toolSchemas = toolDefs.toolSchemas();
    AgentLlmAdapter adapter;
    AgentLlmAdapter::Options aopt;
    aopt.providerId = agentProvider;
    aopt.model = model;
    aopt.recordHealth = true;
    aopt.recordUsage = true;
    AppSettings ls = SettingsManager::instance().load();
    aopt.temperature = ls.temperature;
    adapter.setOptions(aopt);
    adapter.setSecretStore(&m_secrets);
    adapter.resolve();
    auto llm = adapter.toFn(toolSchemas);
    auto progress = [this](const QString& msg) {
        m_view->append("<span style='color:#569cd6;font-family:Consolas,monospace'>" +
                       msg.toHtmlEscaped() + "</span>");
    };

    AgentLoop::Result r = AgentLoop::run(tools, system, task, m_agentMaxSteps, llm,
                                         [this](const ToolCall& c) { return approveTool(c); },
                                         progress, &ctx,
                                         [&adapter](const QString&, const QList<ToolCall>&) {
                                             return adapter.lastMeta();
                                         });

    // --- Stage 34: koşu günlüğü + zincir ilerlemesi ---
    if (m_runStore) {
        AgentRun run;
        run.task = task;
        run.elapsedMs = r.budget.elapsedMs();
        run.tokens = r.budget.tokens;
        run.ok = r.ok;
        run.finalText = r.finalText;
        run.toolCalls = r.toolCalls;
        run.changedFiles = r.changedFiles;
        for (const AgentStep& st : r.steps) {
            AgentRunStep rs;
            rs.assistant = st.assistant.left(2000);
            for (const ToolCall& c : st.calls) rs.toolNames << c.name;
            rs.observations = st.observations;
            run.stepLog << rs;
        }
        m_runStore->add(run);
    }
    if (!m_chain.isEmpty()) {
        m_chain.completeCurrent(r.ok ? "tamam" : "kısmi");
    }
    m_view->append("<i>" + AgentLoop::summarizeRun(r).toHtmlEscaped().replace("\n", "<br>") + "</i>");

    if (!r.finalText.isEmpty()) {
        m_lastResponse = r.finalText;
        m_view->append("<hr><b>AI:</b><br>" + r.finalText.toHtmlEscaped().replace("\n", "<br>"));
        persistMessage("ai", r.finalText);
    }
    if (!r.error.isEmpty())
        m_view->append("<i style='color:#ce9178'>" + r.error.toHtmlEscaped() + "</i>");
    m_agentRunning = false;
    setBusy(false);
    if (!m_patchQueue.isEmpty()) showPatchReview();
}

// --- Stage 33: görsel ekler / arena / dallar / özet ---

ModelCapabilities AiPanel::capsFor(const QString& model) const {
    if (m_caps.contains(model)) return m_caps.value(model);
    return ModelCapabilities::fromName(model);
}

QStringList AiPanel::currentImageBase64() const {
    QStringList out;
    for (const PreparedImage& p : m_images)
        if (p.valid()) out << QString::fromLatin1(ImageUtil::base64(p.bytes));
    return out;
}

void AiPanel::attachImages() {
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, "Görsel ekle", QString(), "Resimler (*.png *.jpg *.jpeg *.bmp *.webp *.gif)");
    if (paths.isEmpty()) return;
    const AppSettings s = SettingsManager::instance().load();
    const QString model = m_models->currentText().trimmed();
    // Stage 37: kesin karar — sağlayıcı+model birlikte değerlendirilir
    const QString provId = ProviderPrefs::resolve().id;
    if (!s.aiVisionEnabled || !capsFor(model).vision ||
        !AiRunner::canRunWithImages(provId, model, 1)) {
        m_view->append(QString("<i style='color:#cca700'>%1</i>")
                           .arg(!s.aiVisionEnabled
                                    ? QStringLiteral("Görü kapalı; ekler gönderimde yok sayılır.")
                                    : AiRunner::imageBlockReason(provId, 1)));
    }
    int added = 0;
    for (const QString& p : paths) {
        const QImage img(p);
        if (img.isNull()) continue;
        PreparedImage pi = ImageUtil::prepare(img, 1024, 80);
        if (!pi.valid()) continue;
        if (!ImageUtil::withinBudget(pi.bytes, 4LL * 1024 * 1024)) {
            m_view->append("<i style='color:#cca700'>Görsel çok büyük, atlandı.</i>");
            continue;
        }
        m_images << pi;
        ++added;
    }
    if (added) refreshImageChips();
}

void AiPanel::clearImages() {
    if (m_images.isEmpty()) return;
    m_images.clear();
    refreshImageChips();
}

void AiPanel::refreshImageChips() {
    if (!m_chipBar) return;
    auto* lay = qobject_cast<QHBoxLayout*>(m_chipBar->layout());
    if (!lay) return;
    while (QLayoutItem* it = lay->takeAt(0)) {
        if (QWidget* w = it->widget()) w->deleteLater();
        delete it;
    }
    if (m_images.isEmpty()) {
        m_chipBar->setVisible(false);
        return;
    }
    auto* title = new QLabel(QString("🖼 %1 görsel eklendi").arg(m_images.size()), m_chipBar);
    title->setStyleSheet("color:#858585;font-size:11px;");
    auto* bClear = new QPushButton("temizle", m_chipBar);
    bClear->setFixedHeight(20);
    connect(bClear, &QPushButton::clicked, this, &AiPanel::clearImages);
    lay->addWidget(title);
    lay->addWidget(bClear);
    lay->addStretch(1);
    m_chipBar->setVisible(true);
}

// Sohbet geçmişini sıkıştır: eski turlar yerel özet, son turlar tam metin.
// Stage 37: konuşma özetini sağlayıcıdan ister (AiRunner, arka planda).
// Aynı anda tek istek; sonuç m_convSummary'ye yazılır, sonraki turda kullanılır.
void AiPanel::requestLlmSummary(const QList<ConvTurn>& turns) {
    if (m_summaryBusy) return; // aynı anda tek özet isteği
    const QList<ConvTurn> older = ConversationSummarizer::olderTurns(turns, 6);
    if (older.isEmpty()) return;
    m_summaryBusy = true;
    if (!m_summaryRunner) {
        m_summaryRunner = new AiRunner(this);
        m_summaryRunner->setSecretStore(&m_secrets);
    }
    const QString prompt = ConversationSummarizer::summarizePrompt(older, 6000);
    AiRunner::Options o = AiRunner::optionsFor(AiTask::Summarize);
    o.allowFailover = true;
    connect(m_summaryRunner, &AiRunner::finished, this,
            [this](const AiRunner::Result& r) {
                if (r.ok && !r.text.trimmed().isEmpty()) m_convSummary = r.text.trimmed();
                m_summaryBusy = false;
            },
            Qt::SingleShotConnection);
    m_summaryRunner->runAsync(o, prompt);
}

QString AiPanel::conversationContext() {
    const AppSettings s = SettingsManager::instance().load();
    if (s.aiSummaryTokens <= 0 || m_activeSessionId.isEmpty() || !m_chatStore) return {};
    ChatSession sess = m_chatStore->load(m_activeSessionId);
    QList<ConvTurn> turns;
    for (const ChatMessage& m : sess.messages)
        if (m.role == "user" || m.role == "ai") turns << ConvTurn{m.role, m.text};
    if (turns.size() < 4) return {}; // kısa sohbet: geçmiş eklemeye gerek yok

    const bool needSummary = ConversationSummarizer::needed(turns, m_convSummary,
                                                            s.aiSummaryTokens, 6);
    // Stage 37: özet gerekiyorsa sağlayıcıdan arka planda iste (bloklamaz);
    // gelene kadar yerel sezgisel özet kullanılır.
    if (needSummary) requestLlmSummary(turns);
    if (needSummary && m_convSummary.isEmpty()) {
        QString summary;
        for (const ConvTurn& t : ConversationSummarizer::olderTurns(turns, 6)) {
            const QString line = (t.role == "ai" ? "AI: " : "Sen: ") +
                                 t.text.section('\n', 0, 0).left(160);
            if (summary.size() + line.size() > 2000) break;
            summary += line + "\n";
        }
        m_convSummary = summary.trimmed();
    }
    if (m_convSummary.isEmpty()) return {};
    QList<ConvTurn> recent;
    for (int i = qMax(0, turns.size() - 6); i < turns.size(); ++i) recent << turns[i];
    const QString merged = ConversationSummarizer::merge(m_convSummary, recent, 6000);
    return merged.isEmpty() ? QString() : "\n[Konuşma geçmişi]\n" + merged + "\n";
}

void AiPanel::showBranches() {
    if (!m_chatStore || m_activeSessionId.isEmpty()) {
        m_view->append("<i>Dallanacak etkin sohbet yok.</i>");
        return;
    }
    const ChatSession s = m_chatStore->load(m_activeSessionId);
    const QStringList leaves = ChatStore::leafIds(s);
    if (leaves.size() <= 1) {
        m_view->append("<i>Bu sohbette tek dal var.</i>");
        return;
    }
    QStringList labels;
    for (const QString& leaf : leaves) {
        const auto path = ChatStore::pathTo(s, leaf);
        const QString last = path.isEmpty() ? QString() : path.last().text.section('\n', 0, 0).left(50);
        labels << QString("%1 mesaj · …%2").arg(path.size()).arg(last);
    }
    bool ok = false;
    const QString pick =
        QInputDialog::getItem(this, "Sohbet Dalları", "Dal seç:", labels, 0, false, &ok);
    if (!ok) return;
    const int idx = labels.indexOf(pick);
    if (idx >= 0) renderBranch(s, leaves[idx]);
}

void AiPanel::renderBranch(const ChatSession& s, const QString& leafId) {
    const auto path = ChatStore::pathTo(s, leafId);
    m_view->clear();
    m_convSummary.clear();
    m_activeBranch = leafId;
    for (const ChatMessage& m : path) {
        if (m.role == "user")
            m_view->append("<b>Sen:</b> " + m.text.toHtmlEscaped().replace("\n", "<br>"));
        else
            m_view->append("<b>AI:</b> " + m.text.toHtmlEscaped().replace("\n", "<br>"));
    }
    m_view->append(QString("<i>⑂ Dal görüntüleniyor (%1 mesaj). Yeni mesajlar bu dala eklenir.</i>")
                       .arg(path.size()));
}

void AiPanel::sendArena() {
    const AppSettings s = SettingsManager::instance().load();
    m_client.setHost(s.ollamaHost);
    QStringList models;
    for (int i = 0; i < m_models->count(); ++i) {
        const QString t = m_models->itemText(i).trimmed();
        if (!t.isEmpty() && !models.contains(t)) models << t;
    }
    const QString cur = m_models->currentText().trimmed();
    if (!cur.isEmpty() && !models.contains(cur)) models.prepend(cur);
    if (models.isEmpty()) {
        m_view->append("<i>Arena için önce model yükleyin (⟳).</i>");
        return;
    }
    QString prompt = m_input->text().trimmed();
    if (prompt.isEmpty()) {
        m_view->append("<i>Arena için bir istem yazın.</i>");
        return;
    }
    prompt += "\n" + buildContext(prompt);
    // Stage 37: arena artık (sağlayıcı, model) hedefleriyle çalışır; yapılandırılmış
    // sağlayıcılar da (anahtarı varsa) havuza otomatik katılır.
    QList<ArenaTarget> targets = ModelArenaDialog::defaultTargets(
        models, m_provider ? m_provider->currentData().toString() : ProviderPrefs::activeProvider());
    if (targets.isEmpty()) targets << ArenaTarget{m_provider->currentData().toString(),
                                                 m_models->currentText().trimmed()};
    auto* d = new ModelArenaDialog(targets, s.systemPrompt, prompt, currentOptions(), this);
    connect(d, &ModelArenaDialog::adoptRequested, this, [this](const QString& text) {
        m_view->append("<hr><b>AI (arena kazananı):</b><br>" +
                       text.toHtmlEscaped().replace("\n", "<br>"));
        m_lastResponse = text;
        persistMessage("ai", text);
    });
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->show();
}

void AiPanel::openAgentPanel() {
    QString root;
    emit projectRootRequested(root);
    auto* d = new AgentPanelDialog(root, this);
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->show();
}

// Stage 34: otonom ama SALT-OKUNUR proje denetimi (varsayılan ayar gerektirmez,
// çünkü hiçbir şeyi değiştiremez).
void AiPanel::runAutonomousAudit() {
    if (m_agentRunning) return;
    QString root;
    emit projectRootRequested(root);
    if (root.isEmpty()) {
        m_view->append("<i>Denetim için önce bir proje klasörü aç.</i>");
        return;
    }
    const ProjectHealth h = ProjectHealth::scan(root);
    const auto ans = QMessageBox::question(
        this, "Otonom denetim",
        QString("Salt-okunur proje denetimi yapılacak.\n\n"
                "Mevcut ölçüm: %1\n\n"
                "Ajan yalnızca okuma araçlarını kullanır; hiçbir dosya değişmez, "
                "komut çalıştırmaz. Devam edilsin mi?")
            .arg(h.summary()),
        QMessageBox::Yes | QMessageBox::Cancel);
    if (ans != QMessageBox::Yes) return;

    const QString task =
        QString("Projeyi salt-okunur olarak denetle ve kısa bir rapor yaz. Şunlara bak: "
                "1) health_scan ile sağlık skorunu ve önerileri al. "
                "2) git_status ve git_diff ile çalışma ağacındaki değişiklikleri incele. "
                "3) En büyük 3 riski önem sırasıyla yaz. "
                "Hiçbir dosyayı değiştirme, komut çalıştırma.");
    m_input->setText("Projeyi denetle");
    m_forceAutonomous = true;
    runAgent(task);
    m_forceAutonomous = false;
}

void AiPanel::fixProblem(const QString& path, int line, const QString& message, const QString& code) {
    AppSettings s = SettingsManager::instance().load();
    m_client.setHost(s.ollamaHost);
    QString model = m_models->currentText().trimmed();
    if (model.isEmpty()) { m_view->append("<i>Önce model seç.</i>"); return; }
    m_activeModel = model;
    const QString shown = QString("Sorunu düzelt: %1:%2").arg(QFileInfo(path).fileName()).arg(line);
    appendUser(shown);
    persistMessage("user", shown);
    const QString prompt =
        QString("Aşağıdaki dosyada bir sorun var.\n\nDosya: %1\nSatır: %2\nSorun: %3\n\n"
                "İlgili kod:\n```\n%4\n```\n\n"
                "Hatayı kısaca açıkla ve düzeltilmiş tam kodu ``` blok içinde ver.")
            .arg(path).arg(line).arg(message, code.left(8000));
    beginAnswer(model, s, prompt);
}

// --- Ajan: son kod bloğunu diff onayıyla uygula ---
QString AiPanel::extractLastCodeBlock(const QString& text) {
    // Son ```...``` bloğunu al (dil etiketi opsiyonel)
    static const QRegularExpression rx("```\\w*\\n(.*?)```",
        QRegularExpression::DotMatchesEverythingOption);
    QString last;
    for (auto it = rx.globalMatch(text); it.hasNext();) last = it.next().captured(1);
    return last.trimmed();
}

void AiPanel::applyLastCodeBlock() {
    QString code = extractLastCodeBlock(m_lastResponse);
    if (code.isEmpty()) { m_view->append("<i>Uygulanacak kod bloğu yok (son AI yanıtında ``` yok).</i>"); return; }
    QString path, content;
    emit currentFileRequested(path, content);
    if (path.isEmpty()) { m_view->append("<i>Açık dosya yok.</i>"); return; }
    QString sel; int st = 0, ln = 0;
    emit selectionRequested(sel, st, ln);
    bool wholeFile = sel.isEmpty();
    QString oldText = wholeFile ? content.left(12000) : sel.left(12000);
    ApplyEditDialog d(this);
    d.setTexts(QString("%1 (%2)").arg(path, wholeFile ? "tüm dosya" : "seçim"), oldText, code);
    if (d.exec() == QDialog::Accepted)
        emit applyToEditorRequested(d.newText(), wholeFile, st, ln);
}
