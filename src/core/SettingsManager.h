#pragma once
#include <QMap>
#include <QString>
#include <QStringList>

// Merkezi ayar deposu (QSettings tabanlı). Tüm UI buradan okur/yazar.
struct AppSettings {
    QString language = "tr";          // "tr" | "en"
    QString theme = "dark";           // "dark" | "light"

    QString ollamaHost = "http://localhost:11434";
    QString ollamaModel = "llama3.1";
    int     contextWindow = 4096;     // num_ctx
    double  temperature = 0.7;
    int     cpuThreads = 4;           // num_thread
    int     gpuLayers = 999;          // Ollama: num_gpu (0 = CPU)
    QString gpuBackend = "CUDA";      // CUDA | ROCm | Vulkan | CPU
    QString systemPrompt = "You are a helpful coding assistant. Answer concisely with code examples.";

    // --- Stage 3: AI ---
    bool aiStreaming = true;
    QString contextMode = "file";       // file | selection | rag | none
    QString activeProfile = "default";
    QString profilesJson = "[]";        // [{name,model,system,temperature,num_ctx}]
    // --- Stage 15: AI hattı ---
    bool aiGhost = false;               // hayalet tamamlama (Tab kabul)

    // --- Stage 1: editör çekirdeği ---
    int  fontSize = 11;                // 8..24
    int  tabWidth = 4;                 // 2..8
    bool autoSave = true;              // odak kaybı + periyodik
    int  autoSaveIntervalMs = 2000;
    bool restoreSession = true;        // açılışta sekmeleri geri yükle
    QString lastRoot;                  // gezgin kökü
    QStringList sessionFiles;          // açık dosya yolları
    int  sessionActive = 0;            // aktif sekme
    QStringList sessionCursors;        // "path\x1Foffset" listesi
    QStringList sessionFiles2;         // 2. grup sekmeler
    int  sessionActive2 = 0;
    QStringList sessionCursors2;
    QStringList sessionFolds;          // "path\x1Fl1,l2,..." katlı başlıklar
    // --- Stage 4: platform ---
    int  largeFileMb = 2;             // üstü: renklendirme kapalı
    bool lspEnabled = true;           // clangd/pylsp
    QString terminalShell = "bash";
    QMap<QString, QString> shortcuts; // eylem-id -> "Ctrl+P" (boşsa varsayılan)
    // --- Stage 14: hata ayıklama ---
    QString gdbPath = "gdb";          // GDB çalıştırılabilir yolu
    // --- Stage 5: editör gücü ---
    bool useEditorConfig = true;
    QString spellLang = "auto";       // auto | tr_TR | en_US | off

    // --- Stage 9: görünüm / tasarım sistemi ---
    QString accentColor;              // "" = tema rengi | "#rrggbb"
    QString uiFontFamily;             // "" = varsayılan UI fontu
    int     uiFontSize = 13;          // px
    double  lineHeight = 1.0;         // 1.0..2.0 (editör)
    double  letterSpacing = 0.0;      // yüzde (ör. 5 = +%5)
    bool    ligatures = true;         // OpenType ligature
    int     cursorWidth = 2;          // px
    bool    lineHighlightOn = true;   // aktif satır vurgusu

    // --- Stage 10: mikro-etkileşim ---
    bool    reducedMotion = false;    // animasyonları kapat
    QString layoutPreset = "standart"; // standart | editor | zen

    // --- Stage 11: editör görsel derinliği ---
    bool    showWhitespace = false;   // sekme/boşluk glifleri
    int     rulerColumn = 0;          // sütun cetveli (0 = kapalı)
    bool    stickyScroll = true;      // yapışkan kapsam şeridi

    // --- Stage 12: uyarlanabilir arayüz ---
    QString density = "comfortable";  // compact | comfortable | spacious
    QString autoThemeMode = "off";    // off | system | schedule
    QString dayTheme = "light";
    QString nightTheme = "dark";
    QString colorVision = "none";     // none | deuteranopia | protanopia | tritanopia
    bool    focusMode = false;        // daktilo modu
    bool    customTitleBar = false;   // frameless özel başlık
    bool    chipGit = true;
    bool    chipProblems = true;
    bool    chipCursor = true;
    bool    chipLang = true;
    bool    chipEol = true;
    bool    chipEnc = true;
    QStringList sidePages = {"explorer", "search", "git", "ai", "problems"};
    QStringList bottomOrder;          // boş = varsayılan sıra
    QString activeUiProfile;          // Stage 12: etkin görünüm profili ("": yok)

    // --- Stage 13: dil zekâsı ---
    bool    formatOnSave = false;     // kaydetmede clangd/pylsp biçimlendirme
    bool    inlayHints = false;       // satır içi tür/parametre ipuçları
    bool    semanticHighlight = true; // LSP semantik renklendirme katmanı
    bool    autoComplete = true;      // yazarken otomatik tamamlama önerisi
    bool    autoClose = true;         // Stage 17: otomatik çift kapatma
    int     historyKeep = 50;         // Stage 17: dosya başına anlık görüntü
    QString version;                // Stage 18: uygulama sürümü
    bool  m_firstRun               = true; // Stage 18: ilk çalıştırma kontrolü
};

class SettingsManager {
public:
    static SettingsManager& instance();
    AppSettings load() const;
    void save(const AppSettings& s) const;
private:
    SettingsManager() = default;
};
