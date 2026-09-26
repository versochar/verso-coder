#include "SettingsDialog.h"
#include "../core/AccentColor.h"
#include "../core/Commands.h"
#include "../core/KeymapPresets.h"
#include "../core/ShortcutCheck.h"
#include "../core/OllamaClient.h"
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
