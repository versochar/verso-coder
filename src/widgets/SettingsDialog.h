#pragma once
#include <QDialog>

class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QLineEdit;
class QTextEdit;
class QSlider;
class QLabel;

class QCheckBox;
class QTableWidget;
class QTabWidget;
class QPushButton;
class QFontComboBox;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

signals:
    void applied(); // dil/tema/AI ayarları değişti
    void themeGalleryRequested(); // Stage 9: tema galerisini aç

private slots:
    void saveAll();
    void updateKeyWarn(); // Stage 21: çakışan kısayolları listele

private:
    QComboBox* m_lang;
    QComboBox* m_theme;
    QLineEdit* m_host;
    QComboBox* m_backend;   // CUDA | ROCm | Vulkan | CPU
    QComboBox* m_model;
    QSpinBox* m_ctx;        // context window
    QSpinBox* m_budget = nullptr; // Stage 25: token bütçesi
    // Stage 33: görü + gömme + hız
    QCheckBox* m_aiVision = nullptr;
    QLineEdit* m_embed = nullptr;
    QSpinBox* m_keepAlive = nullptr;
    QSpinBox* m_parallel = nullptr;
    QSpinBox* m_summary = nullptr;
    // Stage 34: ajan otonomisi
    QCheckBox* m_agentAutonomous = nullptr;
    QSpinBox* m_agentToolCalls = nullptr;
    QSpinBox* m_agentWrites = nullptr;
    QSpinBox* m_agentTokens = nullptr;
    QSpinBox* m_agentMinutes = nullptr;
    QCheckBox* m_agentMemory = nullptr;
    QCheckBox* m_agentSkills = nullptr;
    QLineEdit* m_agentTestCmd = nullptr;
    // Stage 38: ajan komut güvenliği
    QCheckBox* m_agentShellSecure = nullptr;
    QCheckBox* m_agentAudit = nullptr;
    class QSpinBox* m_agentQueueThreshold = nullptr; // Stage 39
    QLabel* m_agentSandboxInfo = nullptr;
    // Stage 35: sağlayıcılar
    void buildProviderTab();
    void onProviderChanged();
    void loadProviderForm();
    void storeProviderForm();
    void onProviderTest();
    void onProviderFetchModels();
    void onProviderBench();       // Stage 36: sağlayıcı karşılaştırma
    void onHealthReset();         // Stage 36
    void loadStage36();           // Stage 36: yönlendirme/kota/maliyet bölümü
    void storeStage36();          // Stage 36
    QWidget* m_provPage = nullptr;
    QComboBox* m_provCombo = nullptr;
    QLineEdit* m_provUrl = nullptr;
    QLineEdit* m_provKey = nullptr;
    QComboBox* m_provModel = nullptr;
    QLabel* m_provHint = nullptr;
    QLabel* m_provStatus = nullptr;
    QCheckBox* m_provShowKey = nullptr;
    // Stage 36
    QCheckBox* m_routeEnabled = nullptr;
    QCheckBox* m_routeFreeFirst = nullptr;
    QCheckBox* m_modelFailover = nullptr;      // Stage 37: yoğun modele yedek
    QCheckBox* m_modelFailoverPaid = nullptr;  // ücretliye geçmeye izin
    QComboBox* m_quickProv = nullptr;
    QLineEdit* m_quickModel = nullptr;
    QComboBox* m_strongProv = nullptr;
    QLineEdit* m_strongModel = nullptr;
    QComboBox* m_embedProv = nullptr;
    QLineEdit* m_embedModel = nullptr;
    QDoubleSpinBox* m_costGuard = nullptr;
    QSpinBox* m_quotaCalls = nullptr;
    QSpinBox* m_quotaTokens = nullptr;
    QLabel* m_usageLabel = nullptr;
    QLabel* m_healthLabel = nullptr;
    // Stage 37: kullanım paneli
    void buildUsageTab();
    void loadUsageTab();
    QWidget* m_usagePage = nullptr;
    QTableWidget* m_usageDays = nullptr;
    QTableWidget* m_usageTable = nullptr;
    QLabel* m_usageSummary = nullptr;
    QLabel* m_cacheLabel = nullptr;   // Stage 38: AI önbellek boyutu
    QLabel* m_advisorLabel = nullptr; // Stage 38: eylem listesi
    void loadAdvisor();
    QSpinBox* m_threads;    // cpu threads
    QSpinBox* m_gpu;        // num_gpu
    QSlider* m_tempSlider;
    QLabel* m_tempLabel;
    QTextEdit* m_system;
    // Stage 1: editör
    QSpinBox* m_fontSize;
    QSpinBox* m_tabWidth;
    QCheckBox* m_autoSave;
    QCheckBox* m_autoReload = nullptr; // Stage 28
    QCheckBox* m_crashReport = nullptr; // Stage 31
    QCheckBox* m_restore;
    // Stage 3: AI
    QCheckBox* m_streaming;
    QComboBox* m_ctxMode;
    QCheckBox* m_ghost; // Stage 15: hayalet tamamlama
    // Stage 4: editör + tema + kısayollar
    QSpinBox* m_largeFile;
    QCheckBox* m_lsp;
    QLineEdit* m_shell;
    QLineEdit* m_gdbPath; // Stage 14
    QTableWidget* m_keys;
    QLabel* m_keyWarn = nullptr; // Stage 21: kısayol çakışma uyarısı
    QPushButton* m_themeImport;
    // Stage 5
    QCheckBox* m_editorConfig;
    QComboBox* m_spellLang;
    // Stage 9: görünüm
    QComboBox* m_accent;
    QFontComboBox* m_uiFont;
    QSpinBox* m_uiFontSize;
    QSlider* m_lineHeight;
    QLabel* m_lineHeightVal = nullptr;
    QSlider* m_letterSpacing;
    QLabel* m_letterSpacingVal = nullptr;
    QCheckBox* m_ligatures;
    QSpinBox* m_cursorWidth;
    QCheckBox* m_lineHighlight;
    QColor m_customAccent; // "Özel..." seçildiyse
    // Stage 10
    QCheckBox* m_reducedMotion;
    QLineEdit* m_search;
    QTabWidget* m_tabs = nullptr;
    void filterPages(const QString& text); // ayar arama
    // Stage 11
    QCheckBox* m_showWhitespace;
    // Stage 23
    QComboBox* m_cursorStyle;
    QSpinBox* m_cursorBlink;
    QCheckBox* m_smoothScroll;
    QSlider* m_lineHiOpacity;
    QLabel* m_lineHiOpacityVal = nullptr;
    QSlider* m_selOpacity;
    QLabel* m_selOpacityVal = nullptr;
    QComboBox* m_bracketStyle;
    QCheckBox* m_showLineEnds;
    QComboBox* m_foldGutter;
    QSpinBox* m_minimapWidth;
    QSpinBox* m_ruler;
    QCheckBox* m_stickyScroll;
    // Stage 12
    QComboBox* m_density;
    QComboBox* m_autoTheme;
    QComboBox* m_dayTheme;
    QComboBox* m_nightTheme;
    QComboBox* m_vision;
    QCheckBox* m_focusMode;
    QCheckBox* m_titleBar;
    QCheckBox* m_chipGit;
    QCheckBox* m_chipProblems;
    QCheckBox* m_chipCursor;
    QCheckBox* m_chipLang;
    QCheckBox* m_chipEol;
    QCheckBox* m_chipEnc;
    QCheckBox* m_pageExplorer;
    QCheckBox* m_pageSearch;
    QCheckBox* m_pageGit;
    QCheckBox* m_pageAi;
    QCheckBox* m_pageProblems;
    // Stage 13: dil zekâsı
    QCheckBox* m_formatOnSave;
    QCheckBox* m_inlayHints;
    QCheckBox* m_semanticHl;
    QCheckBox* m_autoComplete;
    QCheckBox* m_autoClose; // Stage 17
};
