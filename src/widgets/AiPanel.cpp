#include "AiPanel.h"
#include "../core/SettingsManager.h"
#include "ApplyEditDialog.h"
#include "PatchReviewDialog.h"
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFile>
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
#include <QVBoxLayout>
#include <QtConcurrent>

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
    m_models = new QComboBox(this);
    m_models->setEditable(true);
    auto* bReload = new QPushButton("⟳", this);
    bReload->setFixedWidth(32);
    bReload->setToolTip("Modelleri yenile (GET /api/tags)");
    top->addWidget(m_models, 1);
    top->addWidget(bReload);

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
    aRow->addStretch(1);

    m_ragLabel = new QLabel("indeks: yok", this);
    m_ragLabel->setStyleSheet("color:#858585;font-size:11px;");
    m_tokenLabel = new QLabel("token: 0", this);
    m_tokenLabel->setStyleSheet("color:#858585;font-size:11px;");
    m_tokenLabel->setToolTip("Bu oturumdaki token kullanımı ve yaklaşık maliyet");
    auto* infoRow = new QHBoxLayout();
    infoRow->addWidget(m_ragLabel, 1);
    infoRow->addWidget(m_tokenLabel);

    m_view = new QTextEdit(this);
    m_view->setReadOnly(true);
    m_view->setPlaceholderText("Ollama AI asistan. Bağlam modunu seç, sor ve Gönder'e bas.");

    // Hazır komutlar
    auto* qRow = new QHBoxLayout();
    auto* bExplain = new QPushButton("Açıkla", this);
    auto* bFix = new QPushButton("Düzelt", this);
    auto* bTest = new QPushButton("Test yaz", this);
    auto* bCommit = new QPushButton("Commit msg", this);
    auto* bApply = new QPushButton("Dosyaya uygula", this);
    bApply->setToolTip("Son AI yanıtındaki kod bloğunu diff onayıyla editöre uygula");
    for (auto* b : {bExplain, bFix, bTest, bCommit, bApply}) qRow->addWidget(b);

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

    auto* hint = new QLabel("Ollama çalışmıyorsa:  `ollama serve`  +  `ollama pull llama3.1`", this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color:#858585;font-size:11px;");

    lay->addLayout(pRow);
    lay->addLayout(top);
    lay->addLayout(cRow);
    lay->addLayout(aRow);
    lay->addLayout(infoRow);
    lay->addWidget(m_view, 1);
    lay->addLayout(qRow);
    lay->addLayout(row);
    lay->addLayout(hRow);
    lay->addWidget(hint);

    connect(bReload, &QPushButton::clicked, this, [this]() {
        AppSettings s = SettingsManager::instance().load();
        m_client.setHost(s.ollamaHost);
        m_client.fetchModels();
    });
    connect(m_models, &QComboBox::currentTextChanged, this, [this](const QString& t) {
        AppSettings s = SettingsManager::instance().load();
        s.ollamaModel = t;
        SettingsManager::instance().save(s);
    });
    connect(m_send, &QPushButton::clicked, this, [this]() { send(); });
    connect(m_input, &QLineEdit::returnPressed, this, [this]() { send(); });
    connect(m_stop, &QPushButton::clicked, this, &AiPanel::stop);
    connect(bProfSave, &QPushButton::clicked, this, &AiPanel::saveProfile);
    connect(bProfDel, &QPushButton::clicked, this, &AiPanel::deleteProfile);
    connect(m_profiles, &QComboBox::currentTextChanged, this, &AiPanel::onProfileChanged);
    connect(bIndex, &QPushButton::clicked, this, [this, bIndex]() {
        if (m_ragWatcher.isRunning()) return;
        QString root;
        emit projectRootRequested(root);
        m_ragLabel->setText("indeksleniyor (arka plan)...");
        bIndex->setEnabled(false);
        m_rag.clear();
        m_ragWatcher.setFuture(QtConcurrent::run([this, root]() {
            int n = m_rag.indexProject(root);
            return qMakePair(m_rag.fileCount(), n);
        }));
    });
    connect(&m_ragWatcher, &QFutureWatcher<QPair<int, int>>::finished, this, [this, bIndex]() {
        bIndex->setEnabled(true);
        if (m_ragWatcher.isCanceled()) return;
        auto r = m_ragWatcher.result();
        m_ragLabel->setText(QString("indeks: %1 dosya, %2 parça").arg(r.first).arg(r.second));
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

    connect(&m_client, &OllamaClient::modelsReady, this, [this](const QStringList& ms) {
        QString cur = m_models->currentText();
        m_models->clear();
        m_models->addItems(ms);
        if (!cur.isEmpty()) m_models->setCurrentText(cur);
        if (ms.isEmpty()) m_view->append("<i>Model bulunamadı. Ollama çalışıyor mu?</i>");
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
    });
    connect(&m_client, &OllamaClient::tokensUsed, this, [this](int p, int e) {
        m_tokens.add(p, e);
        m_tokenLabel->setText(m_tokens.summary(m_activeModel));
    });
    connect(&m_client, &OllamaClient::error, this, [this](const QString& e) {
        m_streamActive = false;
        m_view->append("<i style='color:red'>Hata: " + e.toHtmlEscaped() + "</i>");
        setBusy(false);
    });

    m_chatStore = new ChatStore(chatDir());
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
    AppSettings s = SettingsManager::instance().load();
    m_client.setHost(s.ollamaHost);
    if (m_models->findText(s.ollamaModel) < 0) m_models->addItem(s.ollamaModel);
    m_models->setCurrentText(s.ollamaModel);
    m_stream->setChecked(s.aiStreaming);
    int ctxIdx = (s.contextMode == "selection") ? 1 : (s.contextMode == "rag") ? 2 : (s.contextMode == "none") ? 3 : 0;
    m_ctxMode->setCurrentIndex(ctxIdx);
    refreshProfileBox();
    m_client.fetchModels();
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
        auto hits = m_rag.query(q.isEmpty() ? "main" : q, 4);
        if (hits.isEmpty()) {
            base = "\n(Not: RAG indeksinde ilgili parça bulunamadı.)\n";
        } else {
            m_ragLabel->setText(QString("indeks: %1 dosya, %2 parça (RAG kullanıldı)")
                                    .arg(m_rag.fileCount()).arg(m_rag.chunkCount()));
            base = "\n[Proje bağlamı (RAG)]\n" + RagIndexer::formatContext(hits) + "\n";
        }
    } else if (mode == 0) {
        QString path, content;
        emit currentFileRequested(path, content);
        base = content.isEmpty() ? QString("\n(Not: açık dosya yok.)\n")
                                 : QString("\n[Dosya: %1]\n```\n%2\n```\n").arg(path, content.left(12000));
    }

    // Stage 7: @-bağlam etiketleri
    if (!ContextResolver::hasMentions(question)) return base;
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
    return base + rr.text;
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
    if (m_stream->isChecked()) {
        m_view->append("<hr><b>AI:</b><br>");
        m_streamCursor = QTextCursor(m_view->document());
        m_streamCursor.movePosition(QTextCursor::End);
        m_streamActive = true;
        m_client.chatStream(model, s.systemPrompt, prompt, currentOptions());
    } else {
        m_view->append("<i>yazıyor...</i>");
        m_client.chat(model, s.systemPrompt, prompt, currentOptions());
    }
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
    m_chatStore->append(m_activeSessionId, m);
    refreshHistory();
}

// --- Stage 7: araç kullanan ajan ---
bool AiPanel::approveTool(const ToolCall& c) {
    if (c.name != "write_file" && c.name != "run_command") return true;
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

    AgentTools tools(root);
    tools.setProblemsProvider([this]() { return problemsText(); });
    tools.setAllowCommand(m_agentCmd->isChecked());
    tools.setWriteMode(AgentTools::Queue);
    m_patchQueue.clear();
    tools.setPatchQueue(&m_patchQueue);

    m_agentRunning = true;
    setBusy(true);
    m_view->append("<hr><b>🤖 Ajan:</b> <i>görev işleniyor…</i>");
    const QString system = s.systemPrompt;
    const QJsonObject opts = currentOptions();
    auto llm = [this, model, opts](const QString& sys, const QString& user, QString& err) -> QString {
        return m_client.chatSync(model, sys, user, opts, err);
    };
    auto progress = [this](const QString& msg) {
        m_view->append("<span style='color:#569cd6;font-family:Consolas,monospace'>" +
                       msg.toHtmlEscaped() + "</span>");
    };

    AgentLoop::Result r = AgentLoop::run(tools, system, task, m_agentSteps->value(), llm,
                                         [this](const ToolCall& c) { return approveTool(c); }, progress);

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
