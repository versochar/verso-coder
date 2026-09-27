#include "SettingsDialog.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QMessageBox>
#include "../core/AccentColor.h"
#include "../core/Commands.h"
#include "../core/KeymapPresets.h"
#include "../core/ShortcutCheck.h"
#include "../core/OllamaClient.h"
#include "../core/ai/LlmClient.h"
#include "../core/ai/LlmProvider.h"
#include "../core/ai/ProviderPrefs.h"
#include "../core/ai/AgentLlmAdapter.h"
#include "../core/ai/ProviderBench.h"
#include "../core/ai/ProviderHealth.h"
#include "../core/ai/ProviderPricing.h"
#include "../core/ai/SecretStore.h"
#include "../core/ai/TaskRouter.h"
#include "../core/ai/UsageLedger.h"
#include "../core/ModelCapabilities.h"
#include "../core/CacheCleaner.h"
#include "../core/SetupAdvisor.h"
#include "../core/TokenStats.h"
#include "../core/SettingsIO.h"
#include "../core/SettingsManager.h"
#include "../core/ThemeManager.h"
#include "../core/ThemeStore.h"
#include "../core/Typography.h"
#include <QColorDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFontComboBox>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QSettings>
#include <QTableWidget>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <algorithm>
#include <numeric>
#include <QTextEdit>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Ayarlar / Settings");
    resize(520, 560);
    AppSettings s = SettingsManager::instance().load();

    auto* tabs = new QTabWidget(this);
    // --- Genel ---
    auto* general = new QWidget(this);
    auto* gf = new QFormLayout(general);
    m_lang = new QComboBox(general);
    m_lang->addItems({"Türkçe", "English"});
    m_lang->setCurrentIndex(s.language == "en" ? 1 : 0);
    m_theme = new QComboBox(general);
    m_theme->addItems(ThemeManager::instance().availableThemes());
    m_theme->setCurrentText(s.theme);
    m_themeImport = new QPushButton("QSS İçe Aktar...", general);
    m_themeImport->setToolTip("Özel .qss dosyası seç (custom tema olarak kaydedilir)");
    connect(m_themeImport, &QPushButton::clicked, this, [this]() {
        QString f = QFileDialog::getOpenFileName(this, "QSS seç", QDir::homePath(), "*.qss");
        if (f.isEmpty()) return;
        if (ThemeManager::instance().importCustomTheme(f)) {
            if (m_theme->findText("custom") < 0) m_theme->addItem("custom");
            m_theme->setCurrentText("custom");
        }
    });
    auto* themeRow = new QHBoxLayout();
    themeRow->addWidget(m_theme, 1);
    themeRow->addWidget(m_themeImport);
    gf->addRow("Dil / Language:", m_lang);
    gf->addRow("Tema / Theme:", themeRow);
    m_fontSize = new QSpinBox(general);
    m_fontSize->setRange(8, 24);
    m_fontSize->setValue(s.fontSize);
    m_tabWidth = new QSpinBox(general);
    m_tabWidth->setRange(2, 8);
    m_tabWidth->setValue(s.tabWidth);
    m_autoSave = new QCheckBox("Otomatik kaydet (2 sn)", general);
    m_autoSave->setChecked(s.autoSave);
    m_restore = new QCheckBox("Açılışta oturumu geri yükle", general);
    m_restore->setChecked(s.restoreSession);
    gf->addRow("Font boyutu:", m_fontSize);
    gf->addRow("Sekme genişliği:", m_tabWidth);
    m_largeFile = new QSpinBox(general);
    m_largeFile->setRange(1, 64);
    m_largeFile->setSuffix(" MB");
    m_largeFile->setValue(s.largeFileMb);
    m_largeFile->setToolTip("Üstündeki dosyalar büyük-dosya modunda açılır (renklendirme kapalı)");
    gf->addRow("Büyük dosya eşiği:", m_largeFile);
    m_lsp = new QCheckBox("LSP etkin (clangd / pylsp)", general);
    m_lsp->setChecked(s.lspEnabled);
    gf->addRow(m_lsp);
    // Stage 13: dil zekâsı
    m_formatOnSave = new QCheckBox("Kaydetmede biçimlendir (LSP)", general);
    m_formatOnSave->setChecked(s.formatOnSave);
    gf->addRow(m_formatOnSave);
    m_inlayHints = new QCheckBox("Satır içi ipuçları (tür/parametre)", general);
    m_inlayHints->setChecked(s.inlayHints);
    gf->addRow(m_inlayHints);
    m_semanticHl = new QCheckBox("Semantik renklendirme (LSP)", general);
    m_semanticHl->setChecked(s.semanticHighlight);
    gf->addRow(m_semanticHl);
    m_autoComplete = new QCheckBox("Otomatik tamamlama önerisi", general);
    m_autoComplete->setChecked(s.autoComplete);
    gf->addRow(m_autoComplete);
    m_autoClose = new QCheckBox("Otomatik çift kapatma ([{\"", general);
    m_autoClose->setChecked(s.autoClose);
    gf->addRow(m_autoClose);
    m_shell = new QLineEdit(s.terminalShell, general);
    gf->addRow("Terminal kabuğu:", m_shell);
    // Stage 14: hata ayıklayıcı
    m_gdbPath = new QLineEdit(s.gdbPath, general);
    m_gdbPath->setPlaceholderText("gdb");
    gf->addRow("GDB yolu:", m_gdbPath);
    m_editorConfig = new QCheckBox(".editorconfig uygula", general);
    m_editorConfig->setChecked(s.useEditorConfig);
    gf->addRow(m_editorConfig);
    m_spellLang = new QComboBox(general);
    m_spellLang->addItems({"auto", "tr_TR", "en_US", "off"});
    m_spellLang->setCurrentText(s.spellLang);
    gf->addRow("Yazım dili:", m_spellLang);
    gf->addRow(m_autoSave);
    m_autoReload = new QCheckBox("Harici değişiklikte otomatik yeniden yükle", general);
    m_autoReload->setChecked(s.autoReload);
    gf->addRow(m_autoReload); // Stage 28
    m_crashReport = new QCheckBox("Çökme izi yaz (hata bildirimine yardımcı olur)", general);
    m_crashReport->setChecked(s.crashReport);
    gf->addRow(m_crashReport); // Stage 31
    gf->addRow(m_restore);
    auto* note = new QLabel("Dil ve tema Kaydet'e basınca anında uygulanır.", general);
    note->setWordWrap(true);
    gf->addRow(note);

    // --- AI Bağlantısı (Ollama) ---
    auto* ai = new QWidget(this);
    auto* af = new QFormLayout(ai);
    m_host = new QLineEdit(s.ollamaHost, ai);
    m_host->setPlaceholderText("http://localhost:11434");
    m_model = new QComboBox(ai);
    m_model->setEditable(true);
    m_model->addItem(s.ollamaModel);
    m_model->setCurrentText(s.ollamaModel);
    auto* bModels = new QPushButton("Modelleri Getir", ai);
    auto* hostRow = new QHBoxLayout();
    hostRow->addWidget(m_host, 1);
    auto* bTest = new QPushButton("Test Et", ai);
    hostRow->addWidget(bTest);

    m_ctx = new QSpinBox(ai);
    m_ctx->setRange(512, 131072);
    m_ctx->setSingleStep(1024);
    m_ctx->setValue(s.contextWindow);
    // Stage 25: token bütçesi (0 = kapalı)
    m_budget = new QSpinBox(ai);
    m_budget->setRange(0, 10000000);
    m_budget->setSingleStep(10000);
    m_budget->setSpecialValueText("Kapalı");
    m_budget->setValue(s.aiTokenBudget);
    m_budget->setToolTip("Oturum token tavanı; aşınca uyarı verilir");

    m_backend = new QComboBox(ai);
    m_backend->addItems({"CUDA", "ROCm", "Vulkan", "CPU"});
    m_backend->setCurrentText(s.gpuBackend);
    m_backend->setToolTip("Ollama GPU kütüphanesi. CUDA=NVIDIA, ROCm=AMD Linux, Vulkan=genel, CPU=sadece işlemci.");

    m_gpu = new QSpinBox(ai);
    m_gpu->setRange(0, 999);
    m_gpu->setValue(s.gpuLayers);
    m_gpu->setToolTip("Ollama num_gpu: GPU'ya offload edilecek katman. 0=CPU, 999=tümünü offload.");

    m_threads = new QSpinBox(ai);
    m_threads->setRange(1, 128);
    m_threads->setValue(s.cpuThreads);

    m_tempSlider = new QSlider(Qt::Horizontal, ai);
    m_tempSlider->setRange(0, 200);
    m_tempSlider->setValue(int(s.temperature * 100));
    m_tempLabel = new QLabel(QString::number(s.temperature, 'f', 2), ai);
    connect(m_tempSlider, &QSlider::valueChanged, this, [this](int v) {
        m_tempLabel->setText(QString::number(v / 100.0, 'f', 2));
    });
    auto* tempRow = new QHBoxLayout();
    tempRow->addWidget(m_tempSlider, 1);
    tempRow->addWidget(m_tempLabel);

    m_system = new QTextEdit(ai);
    m_system->setPlainText(s.systemPrompt);
    m_system->setMaximumHeight(90);

    af->addRow("Ollama Adresi:", hostRow);
    af->addRow("Model:", m_model);
    af->addRow("", bModels);
    m_streaming = new QCheckBox("Streaming (canlı yanıt)", ai);
    m_streaming->setChecked(s.aiStreaming);
    // Stage 15: hayalet tamamlama
    m_ghost = new QCheckBox("Hayalet tamamlama (Tab ile kabul)", ai);
    m_ghost->setChecked(s.aiGhost);
    m_ghost->setToolTip("Yazarken Ollama'dan satır içi öneri; Tab kabul, Esc vazgeç");
    m_ctxMode = new QComboBox(ai);
    m_ctxMode->addItems({"file", "selection", "rag", "none"});
    m_ctxMode->setCurrentText(s.contextMode);
    m_ctxMode->setToolTip("Varsayılan AI bağlam modu");
    af->addRow("Varsayılan bağlam:", m_ctxMode);
    af->addRow("", m_streaming);
    af->addRow("", m_ghost);
    af->addRow("Context Window:", m_ctx);
    af->addRow("Token bütçesi:", m_budget);
    // Stage 33: görü + anlamsal RAG + sıcak tutma
    m_aiVision = new QCheckBox("Görsel girdi (vision) etkin", ai);
    m_aiVision->setChecked(s.aiVisionEnabled);
    m_aiVision->setToolTip("Görü destekli modellerde resim eki göndermeyi açar");
    af->addRow("", m_aiVision);
    m_embed = new QLineEdit(s.aiEmbedModel, ai);
    m_embed->setPlaceholderText("örn. nomic-embed-text (boş = anlamsal RAG kapalı)");
    m_embed->setToolTip("Anlamsal RAG için gömme modeli; boş bırakılırsa anahtar kelime araması kullanılır");
    af->addRow("Gömme modeli:", m_embed);
    m_keepAlive = new QSpinBox(ai);
    m_keepAlive->setRange(-1, 120);
    m_keepAlive->setSuffix(" dk");
    m_keepAlive->setSpecialValueText("Süresiz");
    m_keepAlive->setValue(s.aiKeepAlive);
    m_keepAlive->setToolTip("-1 = model bellekte süresiz kalsın; 0 = her istekten sonra boşalt");
    af->addRow("Modeli sıcak tut:", m_keepAlive);
    m_parallel = new QSpinBox(ai);
    m_parallel->setRange(1, 8);
    m_parallel->setValue(s.aiParallel);
    m_parallel->setToolTip("Bilgi amaçlı: Ollama sunucu OLLAMA_NUM_PARALLEL değeri");
    af->addRow("Eşzamanlı istek:", m_parallel);
    m_summary = new QSpinBox(ai);
    m_summary->setRange(0, 200000);
    m_summary->setSingleStep(500);
    m_summary->setSpecialValueText("Kapalı");
    m_summary->setValue(s.aiSummaryTokens);
    m_summary->setToolTip("Sohbet geçmişi bu token eşiğini aşınca eski turlar özetlenir (0 = kapalı)");
    af->addRow("Sohbet özeti eşiği:", m_summary);
    // --- Stage 34: ajan otonomisi ---
    m_agentAutonomous = new QCheckBox("Otonom ajan (varsayılan kapalı)", ai);
    m_agentAutonomous->setChecked(s.agentAutonomous);
    m_agentAutonomous->setToolTip(
        "Açıkken ajan yalnızca okuma araçlarını kullanır ve rapor üretir; dosya değiştirmez.");
    af->addRow("", m_agentAutonomous);
    m_agentToolCalls = new QSpinBox(ai);
    m_agentToolCalls->setRange(1, 200);
    m_agentToolCalls->setValue(s.agentMaxToolCalls);
    af->addRow("Ajan araç tavanı:", m_agentToolCalls);
    m_agentWrites = new QSpinBox(ai);
    m_agentWrites->setRange(0, 50);
    m_agentWrites->setValue(s.agentMaxWrites);
    m_agentWrites->setToolTip("Bir koşuda en fazla kaç dosya yazılabilir");
    af->addRow("Ajan yazma tavanı:", m_agentWrites);
    m_agentTokens = new QSpinBox(ai);
    m_agentTokens->setRange(0, 5000000);
    m_agentTokens->setSingleStep(1000);
    m_agentTokens->setSpecialValueText("Sınırsız");
    m_agentTokens->setValue(s.agentMaxTokens);
    af->addRow("Ajan token tavanı:", m_agentTokens);
    m_agentMinutes = new QSpinBox(ai);
    m_agentMinutes->setRange(1, 240);
    m_agentMinutes->setSuffix(" dk");
    m_agentMinutes->setValue(s.agentMaxMinutes);
    af->addRow("Ajan süre tavanı:", m_agentMinutes);
    m_agentMemory = new QCheckBox("Ajan belleği (öğrenilen notlar)", ai);
    m_agentMemory->setChecked(s.agentMemory);
    af->addRow("", m_agentMemory);
    m_agentSkills = new QCheckBox("Beceri zinciri", ai);
    m_agentSkills->setChecked(s.agentSkills);
    m_agentSkills->setToolTip("Hedefe uygun çok adımlı beceri planı üret");
    af->addRow("", m_agentSkills);
    m_agentTestCmd = new QLineEdit(s.agentTestCommand, ai);
    m_agentTestCmd->setPlaceholderText("boş = otomatik (ctest / npm test / pytest …)");
    af->addRow("Ajan test komutu:", m_agentTestCmd);

    // --- Stage 38: komut güvenliği ---
    auto* sec = new QLabel("Komut güvenliği", ai);
    sec->setStyleSheet("color:#569cd6;font-weight:600;margin-top:8px;");
    af->insertRow(af->rowCount(), sec);
    m_agentShellSecure = new QCheckBox("Güvenli kabuk (profil yüklenmesin, anahtarlar temizlensin)", ai);
    m_agentShellSecure->setToolTip(
        QStringLiteral("Kapalıyken \"bash -l\" (giriş kabuğu) kullanılır: .bashrc'deki "
                        "alias ve fonksiyonlar komutu sessizce değiştirebilir. Açıkken "
                        "\"bash -c\" ve API anahtarları komutun ortamından temizlenir."));
    af->addRow("", m_agentShellSecure);
    m_agentAudit = new QCheckBox("Komut denetim kaydı tut", ai);
    m_agentAudit->setToolTip("Ajanın çalıştırdığı her komut, onay/ret ve çıkış koduyla günlüğe yazılır.");
    af->addRow("", m_agentAudit);
    m_agentSandboxInfo = new QLabel(ai);
    m_agentSandboxInfo->setWordWrap(true);
    m_agentSandboxInfo->setStyleSheet("color:#858585;font-size:11px;");
    af->addRow("", m_agentSandboxInfo);
    af->addRow("GPU Backend:", m_backend);
    af->addRow("GPU Offload (num_gpu):", m_gpu);
    af->addRow("Temperature:", tempRow);
    af->addRow("CPU Threads:", m_threads);
    af->addRow("Sistem Promptu:", m_system);

    auto* backendNote = new QLabel(
        "Not: Ollama CUDA/ROCm/Vulkan derlemesini otomatik seçer.\n"
        "Backend burada bilgi + num_gpu=0 (CPU) anahtarı olarak tutulur.\n"
        "NVIDIA için CUDA, AMD Linux için ROCm, genel GPU için Vulkan seç.", ai);
    backendNote->setWordWrap(true);
    backendNote->setStyleSheet("color:#858585;font-size:11px;");
    af->addRow(backendNote);

    // CPU seçilirse num_gpu'yu otomatik 0 yap
    connect(m_backend, &QComboBox::currentTextChanged, this, [this](const QString& b) {
        if (b == "CPU") m_gpu->setValue(0);
        else if (m_gpu->value() == 0) m_gpu->setValue(999);
    });

    OllamaClient* probe = new OllamaClient(this);
    connect(bModels, &QPushButton::clicked, this, [this, probe]() {
        probe->setHost(m_host->text());
        probe->fetchModels();
    });
    connect(probe, &OllamaClient::modelsReady, this, [this](const QStringList& ms) {
        QString cur = m_model->currentText();
        m_model->clear();
        m_model->addItems(ms);
        if (!cur.isEmpty() && !ms.contains(cur)) m_model->addItem(cur);
        m_model->setCurrentText(ms.isEmpty() ? cur : ms.first());
        if (ms.isEmpty()) QMessageBox::warning(this, "Ollama", "Model listesi boş. ollama serve çalışıyor mu?");
    });
    connect(probe, &OllamaClient::error, this, [this](const QString& e) {
        QMessageBox::warning(this, "Ollama", "Bağlantı hatası: " + e);
    });
    connect(bTest, &QPushButton::clicked, this, [this, probe]() {
        probe->setHost(m_host->text());
        probe->fetchModels();
    });

    tabs->addTab(general, "Genel");
    tabs->addTab(ai, "AI Bağlantısı (Ollama)");

    // --- Stage 35: AI Sağlayıcıları (çok sağlayıcı) ---
    buildProviderTab();
    tabs->addTab(m_provPage, "AI Sağlayıcıları");
    // --- Stage 37: kullanım paneli ---
    buildUsageTab();
    tabs->addTab(m_usagePage, "AI Kullanımı");

    // --- Stage 9: Görünüm (tasarım sistemi) ---
    auto* look = new QWidget(this);
    auto* lf = new QFormLayout(look);
    auto* bGallery = new QPushButton("🎨  Tema Galerisini Aç...", look);
    bGallery->setToolTip("Hazır temaları önizlemeli seç, vurgu rengini belirle");
    connect(bGallery, &QPushButton::clicked, this, &SettingsDialog::themeGalleryRequested);
    lf->addRow(bGallery);
    m_accent = new QComboBox(look);
    m_accent->addItem("Tema rengi");
    m_accent->addItems(AccentColor::presetNames());
    m_accent->addItem("Özel...");
    if (s.accentColor.isEmpty()) m_accent->setCurrentText("Tema rengi");
    else {
        const QString presetName = [s]() {
            for (const QString& n : AccentColor::presetNames())
                if (AccentColor::preset(n).name().compare(s.accentColor, Qt::CaseInsensitive) == 0)
                    return n;
            return QString();
        }();
        if (!presetName.isEmpty()) m_accent->setCurrentText(presetName);
        else { m_accent->setCurrentText("Özel..."); m_customAccent = QColor(s.accentColor); }
    }
    connect(m_accent, &QComboBox::currentTextChanged, this, [this](const QString& t) {
        if (t != "Özel...") return;
        const QColor c = QColorDialog::getColor(
            m_customAccent.isValid() ? m_customAccent : QColor("#007acc"),
            this, "Vurgu Rengi Seç");
        if (c.isValid()) m_customAccent = c;
    });
    lf->addRow("Vurgu rengi:", m_accent);
    m_uiFont = new QFontComboBox(look);
    m_uiFont->setCurrentFont(QFont(s.uiFontFamily.isEmpty() ? "Segoe UI" : s.uiFontFamily));
    m_uiFont->lineEdit()->setClearButtonEnabled(true);
    m_uiFontSize = new QSpinBox(look);
    m_uiFontSize->setRange(9, 20);
    m_uiFontSize->setSuffix(" px");
    m_uiFontSize->setValue(s.uiFontSize);
    auto* fontRow = new QHBoxLayout();
    fontRow->addWidget(m_uiFont, 1);
    fontRow->addWidget(m_uiFontSize);
    lf->addRow("Arayüz fontu:", fontRow);
    // Stage 23: satır yüksekliği kaydırıcısı (1.0–2.0)
    m_lineHeight = new QSlider(Qt::Horizontal, look);
    m_lineHeight->setRange(10, 20);
    m_lineHeight->setValue(int(s.lineHeight * 10 + 0.5));
    m_lineHeight->setToolTip("Editörde satırlar arası boşluk çarpanı");
    m_lineHeightVal = new QLabel(QString::number(s.lineHeight, 'f', 1) + "×", look);
    connect(m_lineHeight, &QSlider::valueChanged, this, [this](int v) {
        m_lineHeightVal->setText(QString::number(v / 10.0, 'f', 1) + "×");
    });
    auto* lhRow = new QHBoxLayout();
    lhRow->addWidget(m_lineHeight, 1);
    lhRow->addWidget(m_lineHeightVal);
    lf->addRow("Satır yüksekliği:", lhRow);
    // Stage 23: harf aralığı kaydırıcısı (-5…+25 %)
    m_letterSpacing = new QSlider(Qt::Horizontal, look);
    m_letterSpacing->setRange(-5, 25);
    m_letterSpacing->setValue(int(s.letterSpacing));
    m_letterSpacingVal = new QLabel(QString::number(int(s.letterSpacing)) + " %", look);
    connect(m_letterSpacing, &QSlider::valueChanged, this, [this](int v) {
        m_letterSpacingVal->setText(QString::number(v) + " %");
    });
    auto* lsRow = new QHBoxLayout();
    lsRow->addWidget(m_letterSpacing, 1);
    lsRow->addWidget(m_letterSpacingVal);
    lf->addRow("Harf aralığı:", lsRow);
    m_ligatures = new QCheckBox("Ligature (birleşik glifler, örn. => ve !=)", look);
    m_ligatures->setChecked(s.ligatures);
    lf->addRow(m_ligatures);
    m_cursorWidth = new QSpinBox(look);
    m_cursorWidth->setRange(1, 6);
    m_cursorWidth->setSuffix(" px");
    m_cursorWidth->setValue(s.cursorWidth);
    lf->addRow("İmleç genişliği:", m_cursorWidth);
    // Stage 23: imleç stili + yanıp sönme
    m_cursorStyle = new QComboBox(look);
    m_cursorStyle->addItems({"bar: Çubuk", "block: Blok", "underline: Alt çizgi"});
    for (int i = 0; i < m_cursorStyle->count(); ++i)
        if (m_cursorStyle->itemText(i).startsWith(s.cursorStyle + ":")) {
            m_cursorStyle->setCurrentIndex(i);
            break;
        }
    lf->addRow("İmleç stili:", m_cursorStyle);
    m_cursorBlink = new QSpinBox(look);
    m_cursorBlink->setRange(0, 2000);
    m_cursorBlink->setSingleStep(100);
    m_cursorBlink->setSpecialValueText("Sistem");
    m_cursorBlink->setSuffix(" ms");
    m_cursorBlink->setValue(s.cursorBlink);
    m_cursorBlink->setToolTip("0 = sistem varsayılanı");
    lf->addRow("İmleç yanıp sönme:", m_cursorBlink);
    m_smoothScroll = new QCheckBox("Yumuşak kaydırma", look);
    m_smoothScroll->setChecked(s.smoothScroll);
    lf->addRow(m_smoothScroll);
    m_lineHighlight = new QCheckBox("Aktif satır vurgusu", look);
    m_lineHighlight->setChecked(s.lineHighlightOn);
    lf->addRow(m_lineHighlight);
    // Stage 23: vurgu opaklıkları + parantez stili
    m_lineHiOpacity = new QSlider(Qt::Horizontal, look);
    m_lineHiOpacity->setRange(10, 100);
    m_lineHiOpacity->setValue(int(s.lineHighlightOpacity * 100 + 0.5));
    m_lineHiOpacityVal = new QLabel(QString::number(int(s.lineHighlightOpacity * 100)) + " %", look);
    connect(m_lineHiOpacity, &QSlider::valueChanged, this, [this](int v) {
        m_lineHiOpacityVal->setText(QString::number(v) + " %");
    });
    auto* lhoRow = new QHBoxLayout();
    lhoRow->addWidget(m_lineHiOpacity, 1);
    lhoRow->addWidget(m_lineHiOpacityVal);
    lf->addRow("Satır vurgusu:", lhoRow);
    m_selOpacity = new QSlider(Qt::Horizontal, look);
    m_selOpacity->setRange(20, 100);
    m_selOpacity->setValue(int(s.selectionOpacity * 100 + 0.5));
    m_selOpacityVal = new QLabel(QString::number(int(s.selectionOpacity * 100)) + " %", look);
    connect(m_selOpacity, &QSlider::valueChanged, this, [this](int v) {
        m_selOpacityVal->setText(QString::number(v) + " %");
    });
    auto* soRow = new QHBoxLayout();
    soRow->addWidget(m_selOpacity, 1);
    soRow->addWidget(m_selOpacityVal);
    lf->addRow("Seçim opaklığı:", soRow);
    m_bracketStyle = new QComboBox(look);
    m_bracketStyle->addItems({"renk: Renkli", "zemin: Zemin", "altcizgi: Alt çizgi"});
    for (int i = 0; i < m_bracketStyle->count(); ++i)
        if (m_bracketStyle->itemText(i).startsWith(s.bracketStyle + ":")) {
            m_bracketStyle->setCurrentIndex(i);
            break;
        }
    lf->addRow("Parantez vurgusu:", m_bracketStyle);
    // Stage 10: azaltılmış hareket (erişilebilirlik)
    m_reducedMotion = new QCheckBox("Azaltılmış hareket (animasyonları kapat)", look);
    m_reducedMotion->setToolTip("Animasyonlu geçişler anında uygulanır");
    m_reducedMotion->setChecked(s.reducedMotion);
    lf->addRow(m_reducedMotion);
    // Stage 11: editör görsel derinliği
    m_showWhitespace = new QCheckBox("Boşluk karakterlerini göster ( · → ¶ )", look);
    m_showWhitespace->setChecked(s.showWhitespace);
    lf->addRow(m_showWhitespace);
    // Stage 23: satır sonu işaretleri + katlama oku konumu + minimap genişliği
    m_showLineEnds = new QCheckBox("Satır sonu işaretlerini göster (¶)", look);
    m_showLineEnds->setChecked(s.showLineEnds);
    lf->addRow(m_showLineEnds);
    m_foldGutter = new QComboBox(look);
    m_foldGutter->addItems({"sol: Sol", "sag: Sağ", "gizli: Gizli"});
    for (int i = 0; i < m_foldGutter->count(); ++i)
        if (m_foldGutter->itemText(i).startsWith(s.foldGutter + ":")) {
            m_foldGutter->setCurrentIndex(i);
            break;
        }
    lf->addRow("Katlama oku:", m_foldGutter);
    m_minimapWidth = new QSpinBox(look);
    m_minimapWidth->setRange(40, 220);
    m_minimapWidth->setSuffix(" px");
    m_minimapWidth->setValue(s.minimapWidth);
    lf->addRow("Minimap genişliği:", m_minimapWidth);
    m_ruler = new QSpinBox(look);
    m_ruler->setRange(0, 240);
    m_ruler->setSpecialValueText("Kapalı");
    m_ruler->setValue(s.rulerColumn);
    m_ruler->setToolTip("Sütun cetveli (örn. 80 veya 100; 0 = kapalı)");
    lf->addRow("Sütun cetveli:", m_ruler);
    m_stickyScroll = new QCheckBox("Yapışkan kaydırma (kapsam şeridi)", look);
    m_stickyScroll->setChecked(s.stickyScroll);
    lf->addRow(m_stickyScroll);
    // Stage 12: uyarlanabilir arayüz
    m_density = new QComboBox(look);
    m_density->addItems({"compact: Kompakt", "comfortable: Rahat", "spacious: Geniş"});
    for (int i = 0; i < m_density->count(); ++i)
        if (m_density->itemText(i).startsWith(s.density + ":")) { m_density->setCurrentIndex(i); break; }
    m_density->setToolTip("Arayüz yoğunluğu: boşluklar ve font ölçeklenir");
    lf->addRow("Yoğunluk:", m_density);
    m_autoTheme = new QComboBox(look);
    m_autoTheme->addItems({"off: Kapalı", "system: Sistemi izle", "schedule: Zamanlanmış (07–19 gündüz)"});
    for (int i = 0; i < m_autoTheme->count(); ++i)
        if (m_autoTheme->itemText(i).startsWith(s.autoThemeMode + ":")) { m_autoTheme->setCurrentIndex(i); break; }
    lf->addRow("Otomatik tema:", m_autoTheme);
    m_dayTheme = new QComboBox(look);
    m_nightTheme = new QComboBox(look);
    m_dayTheme->addItems(ThemeManager::instance().availableThemes());
    m_nightTheme->addItems(ThemeManager::instance().availableThemes());
    m_dayTheme->setCurrentText(s.dayTheme);
    m_nightTheme->setCurrentText(s.nightTheme);
    auto* dnRow = new QHBoxLayout();
    dnRow->addWidget(new QLabel("Gündüz:", look));
    dnRow->addWidget(m_dayTheme, 1);
    dnRow->addWidget(new QLabel("Gece:", look));
    dnRow->addWidget(m_nightTheme, 1);
    lf->addRow("Gündüz/Gece:", dnRow);
    m_vision = new QComboBox(look);
    m_vision->addItems({"none: Normal", "deuteranopia: Deuteranopi", "protanopia: Protanopi",
                        "tritanopia: Tritanopi"});
    for (int i = 0; i < m_vision->count(); ++i)
        if (m_vision->itemText(i).startsWith(s.colorVision + ":")) { m_vision->setCurrentIndex(i); break; }
    m_vision->setToolTip("Renk körü dostu palet dönüşümü");
    lf->addRow("Renkli görme:", m_vision);
    m_focusMode = new QCheckBox("Odak modu (daktilo: imleç satırı ortalanır)", look);
    m_focusMode->setChecked(s.focusMode);
    lf->addRow(m_focusMode);
    m_titleBar = new QCheckBox("Özel pencere başlığı (frameless; sorun olursa kapatın)", look);
    m_titleBar->setChecked(s.customTitleBar);
    lf->addRow(m_titleBar);
    // Durum çubuğu çipleri
    auto* chipRow = new QHBoxLayout();
    m_chipGit = new QCheckBox("Git", look);
    m_chipProblems = new QCheckBox("Sorunlar", look);
    m_chipCursor = new QCheckBox("Ln:Col", look);
    m_chipLang = new QCheckBox("Dil", look);
    m_chipEol = new QCheckBox("EOL", look);
    m_chipEnc = new QCheckBox("Kodlama", look);
    m_chipGit->setChecked(s.chipGit);
    m_chipProblems->setChecked(s.chipProblems);
    m_chipCursor->setChecked(s.chipCursor);
    m_chipLang->setChecked(s.chipLang);
    m_chipEol->setChecked(s.chipEol);
    m_chipEnc->setChecked(s.chipEnc);
    chipRow->addWidget(m_chipGit);
    chipRow->addWidget(m_chipProblems);
    chipRow->addWidget(m_chipCursor);
    chipRow->addWidget(m_chipLang);
    chipRow->addWidget(m_chipEol);
    chipRow->addWidget(m_chipEnc);
    lf->addRow("Durum çipleri:", chipRow);
    // Yan paneller
    auto* pageRow = new QHBoxLayout();
    m_pageExplorer = new QCheckBox("Gezgin", look);
    m_pageSearch = new QCheckBox("Ara", look);
    m_pageGit = new QCheckBox("Git", look);
    m_pageAi = new QCheckBox("AI", look);
    m_pageProblems = new QCheckBox("Sorunlar", look);
    m_pageExplorer->setChecked(s.sidePages.contains("explorer"));
    m_pageSearch->setChecked(s.sidePages.contains("search"));
    m_pageGit->setChecked(s.sidePages.contains("git"));
    m_pageAi->setChecked(s.sidePages.contains("ai"));
    m_pageProblems->setChecked(s.sidePages.contains("problems"));
    pageRow->addWidget(m_pageExplorer);
    pageRow->addWidget(m_pageSearch);
    pageRow->addWidget(m_pageGit);
    pageRow->addWidget(m_pageAi);
    pageRow->addWidget(m_pageProblems);
    lf->addRow("Yan paneller:", pageRow);
    auto* lookNote = new QLabel(
        "Vurgu rengi ve tema anında; diğerleri Kaydet ile uygulanır.\n"
        "Editör fontu Genel sekmesindeki 'Font boyutu' ile birleşir.", look);
    lookNote->setWordWrap(true);
    lf->addRow(lookNote);
    tabs->addTab(look, "Görünüm");

    // --- Kısayollar ---
    auto* keys = new QWidget(this);
    auto* kl = new QVBoxLayout(keys);
    m_keys = new QTableWidget(keys);
    auto cmds = defaultCommands();
    m_keys->setRowCount(cmds.size());
    m_keys->setColumnCount(2);
    m_keys->setHorizontalHeaderLabels({"Eylem", "Tuş"});
    m_keys->horizontalHeader()->setStretchLastSection(true);
    m_keys->verticalHeader()->setVisible(false);
    for (int i = 0; i < cmds.size(); ++i) {
        auto* name = new QTableWidgetItem(cmds[i].title);
        name->setFlags(name->flags() & ~Qt::ItemIsEditable);
        name->setData(Qt::UserRole, cmds[i].id);
        m_keys->setItem(i, 0, name);
        auto* edit = new QKeySequenceEdit(effectiveKeys(s.shortcuts, cmds[i]));
        edit->setClearButtonEnabled(true);
        m_keys->setCellWidget(i, 1, edit);
        connect(edit, &QKeySequenceEdit::editingFinished, this,
                &SettingsDialog::updateKeyWarn);
    }
    kl->addWidget(m_keys);
    // Stage 21: çakışma uyarısı
    m_keyWarn = new QLabel(keys);
    m_keyWarn->setWordWrap(true);
    m_keyWarn->setStyleSheet("color:#f44747;");
    m_keyWarn->setVisible(false);
    kl->addWidget(m_keyWarn);
    auto* ktop = new QHBoxLayout();
    ktop->addWidget(new QLabel("Kısayol profili:", keys));
    auto* presetBox = new QComboBox(keys);
    presetBox->addItems(KeymapPresets::names());
    auto* bApplyPreset = new QPushButton("Profili Uygula", keys);
    ktop->addWidget(presetBox, 1);
    ktop->addWidget(bApplyPreset);
    kl->insertLayout(0, ktop);
    connect(bApplyPreset, &QPushButton::clicked, this, [this, presetBox]() {
        const QMap<QString, QString> p = KeymapPresets::preset(presetBox->currentText());
        for (int i = 0; i < m_keys->rowCount(); ++i) {
            const QString id = m_keys->item(i, 0)->data(Qt::UserRole).toString();
            auto* edit = qobject_cast<QKeySequenceEdit*>(m_keys->cellWidget(i, 1));
            if (!edit) continue;
            if (p.contains(id)) edit->setKeySequence(QKeySequence(p.value(id)));
        }
        updateKeyWarn();
    });
    updateKeyWarn();
    tabs->addTab(keys, "Kısayollar");

    auto* buttons = new QHBoxLayout();
    auto* bExport = new QPushButton("Ayarları Dışa...", this);
    auto* bImport = new QPushButton("Ayarları İçe...", this);
    bExport->setToolTip("Tüm ayarları JSON dosyasına aktar");
    bImport->setToolTip("JSON dosyasından ayarları yükle");
    auto* bSave = new QPushButton("Kaydet", this);
    auto* bCancel = new QPushButton("Vazgeç", this);
    bSave->setDefault(true);
    buttons->addWidget(bExport);
    buttons->addWidget(bImport);
    buttons->addStretch(1);
    buttons->addWidget(bCancel);
    buttons->addWidget(bSave);
    connect(bSave, &QPushButton::clicked, this, &SettingsDialog::saveAll);
    connect(bCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(bExport, &QPushButton::clicked, this, [this]() {
        const QString f = QFileDialog::getSaveFileName(this, "Ayarları Dışa Aktar",
                                                       "verso-settings.json", "JSON (*.json)");
        if (f.isEmpty()) return;
        QSettings q("Verso", "VersoCoder");
        QString err;
        if (!SettingsIO::writeFile(f, q, &err))
            QMessageBox::warning(this, "Dışa aktarma", err);
        else
            QMessageBox::information(this, "Dışa aktarma", "Ayarlar kaydedildi:\n" + f);
    });
    connect(bImport, &QPushButton::clicked, this, [this]() {
        const QString f = QFileDialog::getOpenFileName(this, "Ayarları İçe Aktar",
                                                       QDir::homePath(), "JSON (*.json)");
        if (f.isEmpty()) return;
        QSettings q("Verso", "VersoCoder");
        QString err;
        if (!SettingsIO::readFile(f, q, &err)) {
            QMessageBox::warning(this, "İçe aktarma", err);
            return;
        }
        emit applied();
        accept();
    });

    auto* lay = new QVBoxLayout(this);
    // Stage 10: ayarlarda arama
    m_tabs = tabs;
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText("Ayarlarında ara... (örn. tema, font, animasyon)");
    m_search->setClearButtonEnabled(true);
    connect(m_search, &QLineEdit::textChanged, this, &SettingsDialog::filterPages);
    lay->addWidget(m_search);
    lay->addWidget(tabs, 1);
    lay->addLayout(buttons);
}

// Stage 10: arama sorgusuna göre sekme/form satırlarını filtrele
static void collectWidgetText(QWidget* w, QString& out) {
    if (!w) return;
    const QMetaObject* mo = w->metaObject();
    const int ti = mo->indexOfProperty("text");
    if (ti >= 0) out += " " + w->property("text").toString();
    const int pi = mo->indexOfProperty("toolTip");
    if (pi >= 0) out += " " + w->property("toolTip").toString();
    if (auto* cb = qobject_cast<QComboBox*>(w))
        for (int i = 0; i < cb->count(); ++i) out += " " + cb->itemText(i);
    for (QObject* c : w->children())
        if (c->isWidgetType()) collectWidgetText(static_cast<QWidget*>(c), out);
}

void SettingsDialog::filterPages(const QString& text) {
    if (!m_tabs) return;
    const QString q = text.trimmed();
    for (int t = 0; t < m_tabs->count(); ++t) {
        QWidget* page = m_tabs->widget(t);
        const bool tabNameHit = m_tabs->tabText(t).contains(q, Qt::CaseInsensitive);
        const auto forms = page->findChildren<QFormLayout*>();
        bool any = q.isEmpty();
        if (!q.isEmpty() && tabNameHit) any = true;
        for (QFormLayout* f : forms) {
            for (int i = 0; i < f->rowCount(); ++i) {
                bool vis = q.isEmpty() || tabNameHit;
                if (!vis) {
                    QString rowText;
                    QLayoutItem* li = f->itemAt(i, QFormLayout::LabelRole);
                    QLayoutItem* fi = f->itemAt(i, QFormLayout::FieldRole);
                    if (li && li->widget()) collectWidgetText(li->widget(), rowText);
                    if (fi) {
                        if (fi->widget()) collectWidgetText(fi->widget(), rowText);
                        else if (fi->layout())
                            for (int c = 0; c < fi->layout()->count(); ++c) {
                                QLayoutItem* ci = fi->layout()->itemAt(c);
                                if (ci && ci->widget()) collectWidgetText(ci->widget(), rowText);
                            }
                    }
                    vis = rowText.contains(q, Qt::CaseInsensitive);
                }
                f->setRowVisible(i, vis);
                if (vis) any = true;
            }
        }
        // form içermeyen sekmeler (örn. Kısayollar): yalnız sekme adı eşleşince görünür
        m_tabs->setTabVisible(t, any);
    }
}

// Stage 21: tablodaki kısayolları topla, çakışmaları uyarı satırında göster
void SettingsDialog::updateKeyWarn() {
    QMap<QString, QString> m;
    for (int i = 0; i < m_keys->rowCount(); ++i) {
        auto* it = m_keys->item(i, 0);
        auto* edit = qobject_cast<QKeySequenceEdit*>(m_keys->cellWidget(i, 1));
        if (!it || !edit) continue;
        m[it->data(Qt::UserRole).toString()] = edit->keySequence().toString();
    }
    const QStringList bad = ShortcutCheck::findConflicts(m);
    m_keyWarn->setVisible(!bad.isEmpty());
    if (!bad.isEmpty())
        m_keyWarn->setText("⚠ Çakışan kısayollar: " + bad.join("  ·  "));
}

// ============================================================
// Stage 35: AI Sağlayıcıları sekmesi
// ============================================================
void SettingsDialog::buildProviderTab() {
    m_provPage = new QWidget(this);
    auto* root = new QVBoxLayout(m_provPage);
    auto* f = new QFormLayout;
    root->addLayout(f);

    auto note = new QLabel(
        "Tek arayüzden Ollama, NVIDIA NIM, UnoRouter, OpenAI, Claude, Gemini ve "
        "OpenAI-uyumlu her sunucu. API anahtarları düz metin ayara yazılmaz; "
        "varsa işletim sistemi anahtar deposuna, yoksa yalnız sahibi okuyabilen "
        "(0600) dosyaya saklanır. Alternatif: VERSO_AI_KEY_<SAĞLAYICI> ortam değişkeni.",
        m_provPage);
    note->setWordWrap(true);
    note->setStyleSheet("color:#858585;font-size:11px;");
    root->addWidget(note);

    m_provCombo = new QComboBox(m_provPage);
    for (const ProviderSpec& spec : ProviderRegistry::all())
        m_provCombo->addItem(QString("%1%2")
                                 .arg(spec.label,
                                      spec.badge().isEmpty() ? QString()
                                                             : "  ·  " + spec.badge()),
                             QVariant(spec.id));
    m_provCombo->setToolTip("Hangi sağlayıcı kullanılacak?");
    f->addRow("Sağlayıcı:", m_provCombo);

    m_provUrl = new QLineEdit(m_provPage);
    m_provUrl->setPlaceholderText("varsayılan kullanılır");
    m_provUrl->setToolTip("Kendi sunucunuz varsa base URL (örn. http://localhost:1234/v1)");
    f->addRow("Base URL:", m_provUrl);

    m_provKey = new QLineEdit(m_provPage);
    m_provKey->setEchoMode(QLineEdit::Password);
    m_provKey->setPlaceholderText("gerekli değilse boş bırakın");
    f->addRow("API anahtarı:", m_provKey);

    m_provShowKey = new QCheckBox("Anahtarı göster", m_provPage);
    connect(m_provShowKey, &QCheckBox::toggled, this, [this](bool on) {
        if (m_provKey) m_provKey->setEchoMode(on ? QLineEdit::Normal : QLineEdit::Password);
    });
    f->addRow("", m_provShowKey);

    m_provModel = new QComboBox(m_provPage);
    m_provModel->setEditable(true);
    f->addRow("Model:", m_provModel);

    m_provHint = new QLabel(m_provPage);
    m_provHint->setWordWrap(true);
    m_provHint->setStyleSheet("color:#569cd6;font-size:11px;");
    f->addRow("", m_provHint);

    m_provStatus = new QLabel(m_provPage);
    m_provStatus->setWordWrap(true);
    m_provStatus->setStyleSheet("color:#858585;font-size:11px;");
    f->addRow("Durum:", m_provStatus);

    auto* row = new QHBoxLayout;
    auto* bFetch = new QPushButton("Modelleri Getir", m_provPage);
    auto* bTest = new QPushButton("Bağlantıyı Test Et", m_provPage);
    row->addWidget(bFetch);
    row->addWidget(bTest);
    row->addStretch(1);
    root->addLayout(row);
    root->addStretch(1);

    // --- Stage 36: maliyet, kota, yönlendirme, sağlık ---
    auto* sep = new QLabel(QString("— %1 —").arg("Maliyet ve kota"), m_provPage);
    sep->setStyleSheet("color:#569cd6;font-weight:600;margin-top:10px;");
    root->addWidget(sep);

    m_costGuard = new QDoubleSpinBox(m_provPage);
    m_costGuard->setRange(0.0, 100.0);
    m_costGuard->setDecimals(2);
    m_costGuard->setSingleStep(0.25);
    m_costGuard->setSuffix(" $");
    m_costGuard->setSpecialValueText("tavan yok");
    m_costGuard->setToolTip("Bir koşunun tahmini maliyeti bu değeri aşarsa onay istenir.");
    f->addRow("Koşu maliyet tavanı:", m_costGuard);

    m_quotaCalls = new QSpinBox(m_provPage);
    m_quotaCalls->setRange(0, 100000);
    m_quotaCalls->setSpecialValueText("sınırsız");
    m_quotaCalls->setToolTip("Seçili sağlayıcı için günlük istek sınırı.");
    f->addRow("Günlük istek kotası:", m_quotaCalls);

    m_quotaTokens = new QSpinBox(m_provPage);
    m_quotaTokens->setRange(0, 100000000);
    m_quotaTokens->setSingleStep(10000);
    m_quotaTokens->setSpecialValueText("sınırsız");
    m_quotaTokens->setToolTip("Seçili sağlayıcı için günlük token sınırı.");
    f->addRow("Günlük token kotası:", m_quotaTokens);

    m_usageLabel = new QLabel(m_provPage);
    m_usageLabel->setWordWrap(true);
    m_usageLabel->setStyleSheet("color:#858585;font-size:11px;");
    f->addRow("Bugünkü kullanım:", m_usageLabel);

    m_healthLabel = new QLabel(m_provPage);
    m_healthLabel->setWordWrap(true);
    m_healthLabel->setStyleSheet("color:#858585;font-size:11px;");
    f->addRow("Sağlık:", m_healthLabel);

    auto* hrow = new QHBoxLayout;
    auto* bHealthReset = new QPushButton("Sağlık Verisini Sıfırla", m_provPage);
    auto* bBench = new QPushButton("Sağlayıcıları Karşılaştır...", m_provPage);
    bBench->setToolTip("2 kısa görevi birkaç sağlayıcıda çalıştırıp puan/Gecikme/maliyet tablosu üretir.");
    hrow->addWidget(bHealthReset);
    hrow->addWidget(bBench);
    hrow->addStretch(1);
    root->addLayout(hrow);

    sep = new QLabel(QString("— %1 —").arg("Görev yönlendirme"), m_provPage);
    sep->setStyleSheet("color:#569cd6;font-weight:600;margin-top:10px;");
    root->addWidget(sep);

    m_routeEnabled = new QCheckBox("Göreve göre sağlayıcı seç (kapalıysa hep seçili sağlayıcı)", m_provPage);
    root->addWidget(m_routeEnabled);
    m_routeFreeFirst = new QCheckBox("Ücretsiz/hızlı modeli tercih et", m_provPage);
    root->addWidget(m_routeFreeFirst);

    m_modelFailover = new QCheckBox("Model yoğunsa aynı sağlayıcıda yedeğe geç", m_provPage);
    m_modelFailover->setToolTip(
        "Seçili model hız sınırı alırsa veya meşgulse (429/403/503), aynı "
        "sağlayıcıda benzer fiyatlı başka bir modele otomatik geçilir.");
    root->addWidget(m_modelFailover);
    m_modelFailoverPaid = new QCheckBox("…geçerken ücretli modele de izin ver", m_provPage);
    m_modelFailoverPaid->setToolTip(
        "Kapalıyken :free bir modelden ücretli bir modele geçilmez (faturayı "
        "şaşırtmamak için varsayılan kapalıdır).");
    root->addWidget(m_modelFailoverPaid);

    auto provBox = [this](QComboBox*& box) {
        box = new QComboBox(m_provPage);
        box->addItem("(seçili sağlayıcı)", QString());
        for (const ProviderSpec& spec : ProviderRegistry::all())
            box->addItem(spec.label, spec.id);
        return box;
    };
    m_quickProv = provBox(m_quickProv);
    m_quickModel = new QLineEdit(m_provPage);
    m_quickModel->setPlaceholderText("model (boş = sağlayıcı varsayılanı)");
    auto* qr = new QHBoxLayout;
    qr->addWidget(m_quickProv);
    qr->addWidget(m_quickModel, 1);
    f->addRow("Basit görevler:", qr);

    m_strongProv = provBox(m_strongProv);
    m_strongModel = new QLineEdit(m_provPage);
    m_strongModel->setPlaceholderText("model (boş = sağlayıcı varsayılanı)");
    auto* sr = new QHBoxLayout;
    sr->addWidget(m_strongProv);
    sr->addWidget(m_strongModel, 1);
    f->addRow("Karmaşık/ajan görevleri:", sr);

    m_embedProv = provBox(m_embedProv);
    m_embedModel = new QLineEdit(m_provPage);
    m_embedModel->setPlaceholderText("örn. text-embedding-3-small");
    auto* er = new QHBoxLayout;
    er->addWidget(m_embedProv);
    er->addWidget(m_embedModel, 1);
    f->addRow("Gömme (RAG):", er);

    connect(bFetch, &QPushButton::clicked, this, &SettingsDialog::onProviderFetchModels);
    connect(bTest, &QPushButton::clicked, this, &SettingsDialog::onProviderTest);
    connect(bHealthReset, &QPushButton::clicked, this, &SettingsDialog::onHealthReset);
    connect(bBench, &QPushButton::clicked, this, &SettingsDialog::onProviderBench);
    connect(m_provCombo, &QComboBox::currentIndexChanged, this,
            [this](int) { loadStage36(); });
    connect(m_provCombo, &QComboBox::currentIndexChanged, this,
            &SettingsDialog::onProviderChanged);

    loadProviderForm(); // etkin sağlayıcı + kayıtlı anahtar/model gelsin
    // Stage 38: ajan komut güvenliği ayarları yüklenir
    {
        QSettings stg;
        if (m_agentShellSecure)
            m_agentShellSecure->setChecked(stg.value("agent/shellSecure", true).toBool());
        if (m_agentAudit) m_agentAudit->setChecked(stg.value("agent/audit", true).toBool());
    }
    // Stage 38: sandbox durumu bilgisi
    if (m_agentSandboxInfo) {
        const QString ro = AgentTools::readOnlyAvailable()
                               ? QString("salt-okunur kip mevcut: %1").arg(AgentTools::readOnlyWrapper())
                               : QString("salt-okunur kip yok (bwrap/unshare bulunamadı) — "
                                         "dosya yazma onayına güveniliyor");
        const QString guard = AgentTools::readOnlyWrapper().isEmpty()
                                  ? QString("yol denetimi: sembolik bağlantı çözülerek kök dışı reddedilir")
                                  : QString("yol denetimi: sembolik bağlantı çözülür");
        m_agentSandboxInfo->setText(QString("Komut ortamı: %1\n%2")
                                        .arg(ro, guard));
    }
}

void SettingsDialog::onProviderChanged() {
    if (!m_provCombo) return;
    const QString id = m_provCombo->currentData().toString();
    const ProviderSpec spec = ProviderRegistry::byId(id);
    if (spec.id.isEmpty()) return;
    m_provUrl->setText(ProviderPrefs::urlFor(spec.id));
    m_provUrl->setPlaceholderText(spec.baseUrl.isEmpty() ? "örn. http://localhost:1234/v1"
                                                        : spec.baseUrl);
    m_provHint->setText(spec.hint.isEmpty()
                            ? QString("%1 · tür: %2")
                                  .arg(spec.label)
                                  .arg(int(spec.kind))
                            : spec.hint);
    SecretStore store;
    m_provKey->setText(store.get(spec.id));
    m_provKey->setEnabled(spec.requiresKey());
    if (!spec.requiresKey()) m_provKey->setPlaceholderText("bu sağlayıcı anahtar istemez");
    // Kayıtlı model ya da örnekler
    const QString saved = ProviderPrefs::modelFor(spec.id);
    QStringList models;
    if (!saved.isEmpty()) models << saved;
    const QStringList samples = ProviderRegistry::sampleModels(spec.id);
    for (const QString& m : samples)
        if (!models.contains(m)) models << m;
    m_provModel->clear();
    m_provModel->addItems(models);
    if (!saved.isEmpty()) m_provModel->setCurrentText(saved);
    m_provStatus->setText(
        QString("Yetenekler: görü %1 · araç %2 · gömme %3")
            .arg(spec.supportsVision ? "✓" : "×")
            .arg(spec.supportsTools ? "✓" : "×")
            .arg(spec.supportsEmbed ? "✓" : "×"));
}

void SettingsDialog::loadProviderForm() {
    if (!m_provCombo) return;
    const int i = m_provCombo->findData(ProviderPrefs::activeProvider());
    m_provCombo->setCurrentIndex(i >= 0 ? i : 0);
    onProviderChanged();
}

void SettingsDialog::storeProviderForm() {
    if (!m_provCombo) return;
    const QString id = m_provCombo->currentData().toString();
    if (id.isEmpty()) return;
    ProviderPrefs::setActiveProvider(id);
    ProviderPrefs::setModel(id, m_provModel->currentText().trimmed());
    ProviderPrefs::setUrl(id, m_provUrl->text().trimmed());
    const ProviderSpec spec = ProviderRegistry::byId(id);
    if (spec.requiresKey()) {
        SecretStore store;
        store.set(id, m_provKey->text());
    }
    m_provKey->clear();
}

void SettingsDialog::onProviderFetchModels() {
    if (!m_provCombo) return;
    const QString id = m_provCombo->currentData().toString();
    ProviderSpec spec = ProviderPrefs::resolve(id);
    if (m_provUrl->text().trimmed().isEmpty()) spec.baseUrl = ProviderRegistry::byId(id).baseUrl;
    LlmClient client;
    client.setProvider(spec);
    if (spec.requiresKey()) client.setApiKey(m_provKey->text().trimmed());
    m_provStatus->setText("Modeller alınıyor...");
    connect(&client, &LlmClient::modelsReady, &client, [this, &client](const QStringList& ms) {
        m_provModel->clear();
        m_provModel->addItems(ms);
        m_provStatus->setText(QString("%1 model bulundu.").arg(ms.size()));
        if (!ms.isEmpty()) m_provModel->setCurrentText(ms.first());
    });
    connect(&client, &LlmClient::error, &client,
            [this](const QString& e) { m_provStatus->setText("Hata: " + e); });
    client.fetchModels();
}

void SettingsDialog::onProviderTest() {
    if (!m_provCombo) return;
    const QString id = m_provCombo->currentData().toString();
    ProviderSpec spec = ProviderPrefs::resolve(id);
    if (m_provUrl->text().trimmed().isEmpty()) spec.baseUrl = ProviderRegistry::byId(id).baseUrl;
    LlmClient client;
    client.setProvider(spec);
    if (spec.requiresKey()) client.setApiKey(m_provKey->text().trimmed());
    m_provStatus->setText("Bağlantı deneniyor...");
    QApplication::processEvents();
    QString err;
    const bool ok = client.testConnection(err, 10000);
    m_provStatus->setText(ok ? QString("✓ Bağlantı başarılı (%1)").arg(spec.label)
                             : "✗ " + err);
}

// ============================================================
// Stage 36: maliyet / kota / yönlendirme / sağlık yükleme-kaydetme
// ============================================================
void SettingsDialog::loadStage36() {
    if (!m_costGuard) return;
    TokenStats::setCostGuardUsd(0.0); // değer aşağıda tek seferde okunur
    QSettings st;
    m_costGuard->setValue(st.value("ai/costGuardUsd", 0.0).toDouble());

    const QString id = m_provCombo ? m_provCombo->currentData().toString() : QString();
    const UsageLedger::Quota q = UsageLedger::instance().quota(id);
    m_quotaCalls->setValue(q.maxCalls);
    m_quotaTokens->setValue(int(q.maxTokens));

    const UsageLedger::Day today = UsageLedger::instance().today();
    m_usageLabel->setText(
        QString("%1 çağrı · %2 token%s")
            .arg(today.calls)
            .arg(today.total())
            .arg(today.usd > 0.0 ? QString(" · ~$%1").arg(today.usd, 0, 'f', 4)
                                : QString(" · ücretsiz/yerel")));
    if (!id.isEmpty()) {
        const ProviderSpec spec = ProviderRegistry::byId(id);
        m_healthLabel->setText(QString("%1 · %2 · fiyat: %3")
                                   .arg(spec.label,
                                        ProviderHealth::instance().statusLine(id),
                                        ProviderPricing::explain(
                                            spec, ProviderPrefs::modelFor(id))));
        QString why;
        if (UsageLedger::instance().quotaExceeded(id, why))
            m_healthLabel->setText(m_healthLabel->text() + " · ⚠ " + why);
    }

    const TaskRouter::Prefs p = TaskRouter::readSettings();
    m_routeEnabled->setChecked(p.enabled);
    m_routeFreeFirst->setChecked(p.useFreeFirst);
    m_modelFailover->setChecked(QSettings().value("ai/modelFailover", true).toBool());
    m_modelFailoverPaid->setChecked(
        QSettings().value("ai/modelFailoverAllowPaid", false).toBool());
    m_modelFailoverPaid->setEnabled(m_modelFailover->isChecked());
    connect(m_modelFailover, &QCheckBox::toggled, this,
            [this](bool on) { m_modelFailoverPaid->setEnabled(on); });
    m_quickProv->setCurrentIndex(qMax(0, m_quickProv->findData(p.quickProvider)));
    m_quickModel->setText(p.quickModel);
    m_strongProv->setCurrentIndex(qMax(0, m_strongProv->findData(p.strongProvider)));
    m_strongModel->setText(p.strongModel);
    m_embedProv->setCurrentIndex(qMax(0, m_embedProv->findData(p.embedProvider)));
    m_embedModel->setText(p.embedModel);
}

void SettingsDialog::storeStage36() {
    if (!m_costGuard) return;
    TokenStats::setCostGuardUsd(m_costGuard->value());

    const QString id = m_provCombo ? m_provCombo->currentData().toString() : QString();
    if (!id.isEmpty()) {
        UsageLedger::Quota q;
        q.maxCalls = m_quotaCalls->value();
        q.maxTokens = m_quotaTokens->value();
        UsageLedger::instance().setQuota(id, q);
        UsageLedger::instance().prune(90);
    }

    TaskRouter::Prefs p = TaskRouter::readSettings();
    p.enabled = m_routeEnabled->isChecked();
    p.useFreeFirst = m_routeFreeFirst->isChecked();
    p.quickProvider = m_quickProv->currentData().toString();
    p.quickModel = m_quickModel->text().trimmed();
    p.strongProvider = m_strongProv->currentData().toString();
    p.strongModel = m_strongModel->text().trimmed();
    p.embedProvider = m_embedProv->currentData().toString();
    p.embedModel = m_embedModel->text().trimmed();
    TaskRouter::writeSettings(p);
    QSettings st;
    st.setValue("ai/modelFailover", m_modelFailover->isChecked());
    st.setValue("ai/modelFailoverAllowPaid", m_modelFailoverPaid->isChecked());
}

void SettingsDialog::onHealthReset() {
    ProviderHealth::instance().reset();
    loadStage36();
    if (m_provStatus) m_provStatus->setText("Sağlık verileri sıfırlandı.");
}

void SettingsDialog::onProviderBench() {
    // Adaylar: anahtarı olan ya da yerel (anahtarsız) sağlayıcılar
    SecretStore store;
    QList<ProviderSpec> pool;
    const QString active = m_provCombo ? m_provCombo->currentData().toString() : QString();
    for (const ProviderSpec& spec : ProviderRegistry::all()) {
        if (spec.requiresKey() && store.effectiveKey(spec.id).isEmpty()) continue;
        if (spec.id == active) continue;
        pool << spec;
    }
    if (pool.size() > 3) pool = pool.mid(0, 3);
    if (pool.isEmpty()) {
        QMessageBox::information(this, "Karşılaştırma",
                                 "Karşılaştırılacak sağlayıcı yok. Önce bir API anahtarı girin "
                                 "(ya da Ollama'yı etkin bırakın).");
        return;
    }
    const QList<BenchTask> tasks = ProviderBench::tasks();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();

    QList<BenchResult> results;
    for (const ProviderSpec& spec : pool) {
        const QString model = ProviderPrefs::modelFor(
            spec.id, ProviderRegistry::sampleModels(spec.id).value(0));
        if (model.isEmpty()) continue;
        if (ModelCapabilities::isEmbeddingModel(model)) continue; // gömme modeli yarışmaz
        LlmClient client;
        client.setProvider(spec);
        client.setSecretStore(&store);
        QList<QPair<QString, QString>> answers;
        QList<int> latencies;
        for (int i = 0; i < qMin(2, tasks.size()); ++i) {
            AiChatRequest req;
            req.model = model;
            req.systemPrompt = "Kısa ve doğru cevap ver.";
            req.messages << AiMessage::user(tasks.at(i).prompt);
            req.maxTokens = 400;
            QElapsedTimer timer;
            timer.start();
            const AiReply rep = client.chatSync(req, 25000);
            latencies << int(timer.elapsed());
            answers.append(qMakePair(tasks.at(i).id, rep.ok ? rep.text : QString()));
        }
        results << ProviderBench::score(spec.id, model, answers, latencies);
        QApplication::processEvents();
    }
    if (results.isEmpty()) {
        QMessageBox::warning(this, "Karşılaştırma", "Hiçbir sağlayıcı yanıt vermedi.");
        return;
    }
    QApplication::restoreOverrideCursor();
    std::sort(results.begin(), results.end(),
              [](const BenchResult& a, const BenchResult& b) { return a.score > b.score; });
    QString text = "Sağlayıcı          Puan  Kalite  Gecikme  Maliyet\n";
    for (const BenchResult& r : results) {
        text += QString("%1 %2  %3  %4 ms  $5\n")
                    .arg(r.providerId, -16)
                    .arg(r.score, 5, 'f', 0)
                    .arg(r.quality, 6, 'f', 0)
                    .arg(r.latencyMs, 7)
                    .arg(r.usd, 0, 'f', 4);
    }
    text += QString("\nÖneri: %1").arg(ProviderBench::recommend(results));
    QMessageBox box(this);
    box.setWindowTitle("Sağlayıcı Karşılaştırması");
    box.setText(text);
    box.setDetailedText(
        std::accumulate(results.begin(), results.end(), QString(),
                        [](const QString& acc, const BenchResult& r) {
                            return acc + ProviderBench::explain(r) + "\n";
                        }));
    box.exec();
}

// ============================================================
// Stage 37: AI Kullanımı sekmesi — günlük token/maliyet, sağlayıcı dağılımı,
// kota durumu, sağlık tablosu
// ============================================================
void SettingsDialog::buildUsageTab() {
    m_usagePage = new QWidget(this);
    auto* root = new QVBoxLayout(m_usagePage);

    m_usageSummary = new QLabel(m_usagePage);
    m_usageSummary->setWordWrap(true);
    m_usageSummary->setStyleSheet("color:#858585;");
    root->addWidget(m_usageSummary);

    auto* daysLabel = new QLabel("Son 14 gün", m_usagePage);
    daysLabel->setStyleSheet("color:#569cd6;font-weight:600;");
    root->addWidget(daysLabel);
    m_usageDays = new QTableWidget(0, 5, m_usagePage);
    auto* days = m_usageDays;
    days->setHorizontalHeaderLabels({"Tarih", "Çağrı", "Token", "Maliyet", "Not"});
    days->horizontalHeader()->setStretchLastSection(true);
    days->setEditTriggers(QAbstractItemView::NoEditTriggers);
    days->setSelectionBehavior(QAbstractItemView::SelectRows);
    days->setMaximumHeight(190);
    root->addWidget(days);

    auto* provLabel = new QLabel("Sağlayıcı dağılımı (bugün)", m_usagePage);
    provLabel->setStyleSheet("color:#569cd6;font-weight:600;");
    root->addWidget(provLabel);
    m_usageTable = new QTableWidget(0, 6, m_usagePage);
    m_usageTable->setHorizontalHeaderLabels({"Sağlayıcı", "Çağrı", "Giriş", "Çıkış", "Maliyet",
                                            "Sağlık"});
    m_usageTable->horizontalHeader()->setStretchLastSection(true);
    m_usageTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_usageTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    root->addWidget(m_usageTable, 1);

    // --- Stage 38: yönlendirme rehberi + AI önbelleği ---
    auto* advLabel = new QLabel("Yapılacaklar", m_usagePage);
    advLabel->setStyleSheet("color:#569cd6;font-weight:600;");
    root->addWidget(advLabel);
    m_advisorLabel = new QLabel(m_usagePage);
    m_advisorLabel->setWordWrap(true);
    m_advisorLabel->setTextFormat(Qt::RichText);
    m_advisorLabel->setStyleSheet("color:#c9c9c9;font-size:11px;");
    root->addWidget(m_advisorLabel);

    m_cacheLabel = new QLabel(m_usagePage);
    m_cacheLabel->setStyleSheet("color:#858585;font-size:11px;");
    root->addWidget(m_cacheLabel);
    auto* cacheRow = new QHBoxLayout;
    auto* bCacheClean = new QPushButton("AI Önbelleğini Temizle", m_usagePage);
    bCacheClean->setToolTip("Model kataloğu, sağlık geçmişi ve komut denetimi silinir.\n"
                            "Kullanım geçmişi ve API anahtarları KORUNUR.");
    cacheRow->addWidget(bCacheClean);
    cacheRow->addStretch(1);
    root->addLayout(cacheRow);
    connect(bCacheClean, &QPushButton::clicked, this, [this]() {
        CacheCleaner cc(this);
        if (cc.cleanAiCache(false)) loadUsageTab();
    });

    auto* row = new QHBoxLayout;
    auto* bRefresh = new QPushButton("Yenile", m_usagePage);
    auto* bClear = new QPushButton("Geçmişi Temizle", m_usagePage);
    bClear->setToolTip("Tüm kullanım geçmişi silinir (kotalar korunur).");
    row->addWidget(bRefresh);
    row->addWidget(bClear);
    row->addStretch(1);
    root->addLayout(row);

    connect(bRefresh, &QPushButton::clicked, this, [this]() { loadUsageTab(); });
    connect(bClear, &QPushButton::clicked, this, [this]() {
        const auto r = QMessageBox::question(this, "Kullanım geçmişi",
                                            "Tüm token kullanım geçmişi silinsin mi?",
                                            QMessageBox::Yes | QMessageBox::No);
        if (r != QMessageBox::Yes) return;
        UsageLedger::instance().reset();
        loadUsageTab();
    });
    loadUsageTab();
}

void SettingsDialog::loadUsageTab() {
    if (!m_usageTable) return;
    UsageLedger& u = UsageLedger::instance();
    const UsageLedger::Day today = u.today();
    m_usageSummary->setText(
        QString("Bugün: %1 çağrı · %2 token (giriş %3 / çıkış %4) · %5")
            .arg(today.calls)
            .arg(today.total())
            .arg(today.prompt)
            .arg(today.eval)
            .arg(today.usd > 0.0 ? QString("~$%1").arg(today.usd, 0, 'f', 4)
                                  : QString("ücretsiz")));
    // Gün tablosu
    if (m_usageDays) {
        auto* days = m_usageDays;
        const auto hist = u.lastDays(14);
        days->setRowCount(int(hist.size()));
        for (int i = 0; i < hist.size(); ++i) {
            const UsageLedger::Day& d = hist.at(i).second;
            days->setItem(i, 0, new QTableWidgetItem(hist.at(i).first));
            days->setItem(i, 1, new QTableWidgetItem(QString::number(d.calls)));
            days->setItem(i, 2, new QTableWidgetItem(QString::number(d.total())));
            days->setItem(i, 3, new QTableWidgetItem(
                                   d.usd > 0.0 ? QString("$%1").arg(d.usd, 0, 'f', 4)
                                               : QString("—")));
            days->setItem(i, 4, new QTableWidgetItem(QString()));
        }
    }
    // Sağlayıcı tablosu
    QStringList ids = u.providers();
    const QString active = m_provCombo ? m_provCombo->currentData().toString() : QString();
    if (!active.isEmpty() && !ids.contains(active)) ids.prepend(active);
    m_usageTable->setRowCount(int(ids.size()));
    for (int i = 0; i < ids.size(); ++i) {
        const QString id = ids.at(i);
        const UsageLedger::Day d = u.today(id);
        const ProviderSpec spec = ProviderRegistry::byId(id);
        m_usageTable->setItem(i, 0, new QTableWidgetItem(spec.label.isEmpty() ? id : spec.label));
        m_usageTable->setItem(i, 1, new QTableWidgetItem(QString::number(d.calls)));
        m_usageTable->setItem(i, 2, new QTableWidgetItem(QString::number(d.prompt)));
        m_usageTable->setItem(i, 3, new QTableWidgetItem(QString::number(d.eval)));
        m_usageTable->setItem(i, 4, new QTableWidgetItem(
                                       d.usd > 0.0 ? QString("$%1").arg(d.usd, 0, 'f', 4)
                                                   : QString("—")));
        const UsageLedger::Quota q = u.quota(id);
        QString health = ProviderHealth::instance().statusLine(id);
        if (q.limited())
            health += QString(" · kota %1/%2")
                          .arg(d.calls)
                          .arg(q.maxCalls > 0 ? q.maxCalls : q.maxTokens);
        QString why;
        if (u.quotaExceeded(id, why)) health += " · ⚠ " + why;
        m_usageTable->setItem(i, 5, new QTableWidgetItem(health));
    }
    m_usageTable->resizeColumnsToContents();
    loadAdvisor();
}

// Stage 38: "neden çalışmıyor?" rehberi — sağlık verisi değil, eylem listesi
void SettingsDialog::loadAdvisor() {
    if (!m_advisorLabel) return;
    const auto actions = SetupAdvisor::analyze();
    if (actions.isEmpty()) {
        m_advisorLabel->setText(QStringLiteral("<span style='color:#4ec9b0'>"
                                              "✓ Her şey hazır — etkin sağlayıcı kullanılabilir.</span>"));
    } else {
        QStringList lines;
        int shown = 0;
        for (const SetupAction& a : actions) {
            if (shown++ >= 4) {
                lines << QStringLiteral("<i>… ve %1 eylem daha</i>").arg(actions.size() - 4);
                break;
            }
            const QString color = a.severity == SetupAction::Critical
                                      ? QStringLiteral("#f44747")
                                      : (a.severity == SetupAction::Warning
                                             ? QStringLiteral("#cca700")
                                             : QStringLiteral("#858585"));
            lines << QStringLiteral("<span style='color:%1'>● %2</span><br>%3")
                         .arg(color, a.title.toHtmlEscaped(),
                              a.action.isEmpty() ? QString()
                                                 : QStringLiteral("<span style='color:#9a9a9a'>→ %1</span>")
                                                       .arg(a.action.toHtmlEscaped()));
        }
        m_advisorLabel->setText(lines.join(QStringLiteral("<br><br>")));
    }
    if (m_cacheLabel) {
        CacheCleaner cc(this);
        m_cacheLabel->setText(QStringLiteral("%1 · %2 KB")
                                  .arg(cc.totalSummary())
                                  .arg(CacheCleaner::aiCacheBytes() / 1024));
    }
}

void SettingsDialog::saveAll() {    AppSettings cur = SettingsManager::instance().load(); // oturum alanlarını koru
    AppSettings s = cur;
    s.language = (m_lang->currentIndex() == 1) ? "en" : "tr";
    s.theme = m_theme->currentText();
    s.ollamaHost = m_host->text().trimmed();
    s.ollamaModel = m_model->currentText().trimmed();
    s.contextWindow = m_ctx->value();
    s.aiTokenBudget = m_budget ? m_budget->value() : 0; // Stage 25
    // Stage 33
    if (m_aiVision) s.aiVisionEnabled = m_aiVision->isChecked();
    if (m_embed) s.aiEmbedModel = m_embed->text().trimmed();
    if (m_keepAlive) s.aiKeepAlive = m_keepAlive->value();
    if (m_parallel) s.aiParallel = m_parallel->value();
    if (m_summary) s.aiSummaryTokens = m_summary->value();
    // Stage 34
    if (m_agentAutonomous) s.agentAutonomous = m_agentAutonomous->isChecked();
    if (m_agentToolCalls) s.agentMaxToolCalls = m_agentToolCalls->value();
    if (m_agentWrites) s.agentMaxWrites = m_agentWrites->value();
    if (m_agentTokens) s.agentMaxTokens = m_agentTokens->value();
    if (m_agentMinutes) s.agentMaxMinutes = m_agentMinutes->value();
    if (m_agentMemory) s.agentMemory = m_agentMemory->isChecked();
    if (m_agentSkills) s.agentSkills = m_agentSkills->isChecked();
    if (m_agentTestCmd) s.agentTestCommand = m_agentTestCmd->text().trimmed();
    // Stage 35: sağlayıcı tercihleri + API anahtarları
    storeProviderForm();
    // Stage 36: maliyet tavanı + kota + yönlendirme
    storeStage36();
    // Stage 38: ajan komut güvenliği ayarları
    {
        QSettings stg;
        if (m_agentShellSecure)
            stg.setValue("agent/shellSecure", m_agentShellSecure->isChecked());
        if (m_agentAudit) stg.setValue("agent/audit", m_agentAudit->isChecked());
    }
    s.gpuBackend = m_backend->currentText();
    s.gpuLayers = (s.gpuBackend == "CPU") ? 0 : m_gpu->value();
    s.temperature = m_tempSlider->value() / 100.0;
    s.cpuThreads = m_threads->value();
    s.systemPrompt = m_system->toPlainText();
    s.fontSize = m_fontSize->value();
    s.tabWidth = m_tabWidth->value();
    s.autoSave = m_autoSave->isChecked();
    s.autoReload = m_autoReload->isChecked(); // Stage 28
    s.crashReport = m_crashReport->isChecked(); // Stage 31
    s.restoreSession = m_restore->isChecked();
    s.aiStreaming = m_streaming->isChecked();
    s.aiGhost = m_ghost->isChecked(); // Stage 15
    s.contextMode = m_ctxMode->currentText();
    s.largeFileMb = m_largeFile->value();
    s.lspEnabled = m_lsp->isChecked();
    s.gdbPath = m_gdbPath->text().trimmed().isEmpty() ? "gdb" : m_gdbPath->text().trimmed();
    // Stage 13: dil zekâsı
    s.formatOnSave = m_formatOnSave->isChecked();
    s.inlayHints = m_inlayHints->isChecked();
    s.semanticHighlight = m_semanticHl->isChecked();
    s.autoComplete = m_autoComplete->isChecked();
    s.autoClose = m_autoClose->isChecked();
    s.terminalShell = m_shell->text().trimmed().isEmpty() ? "bash" : m_shell->text().trimmed();
    s.useEditorConfig = m_editorConfig->isChecked();
    s.spellLang = m_spellLang->currentText();
    // Stage 9: görünüm
    const QString acc = m_accent->currentText();
    s.accentColor = (acc == "Tema rengi") ? QString()
                  : (acc == "Özel...")   ? (m_customAccent.isValid() ? m_customAccent.name() : QString())
                                         : AccentColor::preset(acc).name();
    s.uiFontFamily = m_uiFont->currentFont().family();
    s.uiFontSize = m_uiFontSize->value();
    s.lineHeight = m_lineHeight->value() / 10.0;
    s.letterSpacing = m_letterSpacing->value();
    s.ligatures = m_ligatures->isChecked();
    s.cursorWidth = m_cursorWidth->value();
    // Stage 23
    s.cursorStyle = m_cursorStyle->currentText().section(':', 0, 0);
    s.cursorBlink = m_cursorBlink->value();
    s.smoothScroll = m_smoothScroll->isChecked();
    s.lineHighlightOpacity = m_lineHiOpacity->value() / 100.0;
    s.selectionOpacity = m_selOpacity->value() / 100.0;
    s.bracketStyle = m_bracketStyle->currentText().section(':', 0, 0);
    s.showLineEnds = m_showLineEnds->isChecked();
    s.foldGutter = m_foldGutter->currentText().section(':', 0, 0);
    s.minimapWidth = m_minimapWidth->value();
    s.lineHighlightOn = m_lineHighlight->isChecked();
    s.reducedMotion = m_reducedMotion->isChecked(); // Stage 10
    s.showWhitespace = m_showWhitespace->isChecked(); // Stage 11
    s.rulerColumn = m_ruler->value();
    s.stickyScroll = m_stickyScroll->isChecked();
    // Stage 12: uyarlanabilir arayüz
    s.density = m_density->currentText().section(':', 0, 0);
    s.autoThemeMode = m_autoTheme->currentText().section(':', 0, 0);
    s.dayTheme = m_dayTheme->currentText();
    s.nightTheme = m_nightTheme->currentText();
    s.colorVision = m_vision->currentText().section(':', 0, 0);
    s.focusMode = m_focusMode->isChecked();
    s.customTitleBar = m_titleBar->isChecked();
    s.chipGit = m_chipGit->isChecked();
    s.chipProblems = m_chipProblems->isChecked();
    s.chipCursor = m_chipCursor->isChecked();
    s.chipLang = m_chipLang->isChecked();
    s.chipEol = m_chipEol->isChecked();
    s.chipEnc = m_chipEnc->isChecked();
    s.sidePages.clear();
    if (m_pageExplorer->isChecked()) s.sidePages << "explorer";
    if (m_pageSearch->isChecked()) s.sidePages << "search";
    if (m_pageGit->isChecked()) s.sidePages << "git";
    if (m_pageAi->isChecked()) s.sidePages << "ai";
    if (m_pageProblems->isChecked()) s.sidePages << "problems";
    for (int i = 0; i < m_keys->rowCount(); ++i) {
        QString id = m_keys->item(i, 0)->data(Qt::UserRole).toString();
        auto* edit = qobject_cast<QKeySequenceEdit*>(m_keys->cellWidget(i, 1));
        if (edit) s.shortcuts[id] = edit->keySequence().toString();
    }
    SettingsManager::instance().save(s);
    emit applied();
    accept();
}
