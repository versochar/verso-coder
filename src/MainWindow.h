#pragma once
#include "core/ClipboardRing.h"
#include "core/GridCheck.h"
#include "core/LspServers.h"
#include "core/SignatureHelp.h"
#include "core/MacroRecorder.h"
#include "core/BookmarkStore.h"
#include "core/LaunchConfig.h"
#include "core/Stability.h"
#include "core/CrashHandler.h"
#include "core/PerfTools.h"
#include "core/ExternalTools.h"
#include "core/ConnectionProfile.h"
#include "core/GdbDriver.h"
#include "core/SnippetManager.h"
#include "core/LspClient.h"
#include "core/SearchMarks.h"
#include "core/TestDiscovery.h"
#include "core/TestParser.h"
#include <QLabel>
#include <QMainWindow>
class CollabSession;
#include <QMap>
#include <QFileSystemWatcher>
#include <QPointer>
#include <QSet>
#include <QElapsedTimer>
#include <QPoint>
#include <QPushButton>
#include <functional>

class QStackedWidget;
class QTabWidget;
class SshSession;
class PortForwarder;
class RemoteExplorer;
class RemoteTerminal;
class PortForwardPanel;
class QLabel;
class TaskRunner;
class QTimer;
class QAction;
class QDockWidget;
class QSettings;
class ExplorerPanel;
class SearchPanel;
class GitPanel;
class AiPanel;
class CodeEditor;
class BreadcrumbBar;
class Minimap;
class ProblemsPanel;
class TerminalPanel;
class TodoPanel;
class HistoryTab;
class StashTab;
class RemoteTab;
class GitIgnore;
class TrashManager;
class ProjectSessions;
class TaskPanel;
class BackupManager;
struct DocSession;
struct StartupOptions;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void applyStartupOptions(const StartupOptions& opts); // Stage 8: CLI / tek örnek
    void checkStartupBudget(int budgetMs); // Stage 32: başlangıç bütçesi bildirimi
    // Stage 46: test edilebilirlik (tembel sekme sayımı)
    int pendingTabCount() const;
    int editorTabCount() const;
    DocSession captureSession() const;
    void applyDocSession(const DocSession& s);

protected:
    void closeEvent(QCloseEvent* e) override;
    bool eventFilter(QObject* o, QEvent* e) override; // Stage 9: çip tıklamaları

private slots:
    void openFile(const QString& path);
    void openFileAt(const QString& path, int line);
    void saveCurrent();
    void saveAll();
    void openSettings();
    void applySettings();
    void retranslate();
    void showQuickOpen();
    void showPalette();
    void runCommand(const QString& id);
    void gotoLineDialog();
    void lspDefinition();
    void lspHover();
    void checkSpelling();
    void loadFullCurrent();
    void splitActiveToOther();
    void moveActiveTab();
    void autoSaveTick();
    void updateCursorStatus();
    void updateBreadcrumb();
    void applyAiEdit(const QString& newText, bool wholeFile, int selStart, int selLen);
    void runBuildForCurrent();
    void onExternalChange(CodeEditor* editor);
    void onDiagnostics(const QString& path, const QList<LspDiag>& diags);
    void diffPath(const QString& path);                 // dosyayı git ile karşılaştır
    void diffExternal(const QString& fileA, const QString& fileB, const QString& title);
    void onPathRenamed(const QString& oldPath, const QString& newPath);
    void saveSnapshot();
    void loadSnapshot();
    void showDiagnostics(); // Stage 8
    void backupEditor(CodeEditor* e);
    // Stage 9: görünüm
    void openThemeGallery();
    void refreshActivityIcons();
    void refreshGitBranch();
    void updateTabMarks();
    QLabel* makeChip(const QString& toolTip, const QString& cmd);
    void showSidePanel(int index);
    // Stage 10: mikro-etkileşim
    void applyLayoutPreset(const QString& name);
    void toggleZen();
    void zoom(int delta);          // -1 uzaklaştır, +1 yakınlaştır, 0 sıfırla
    void exportUiProfile();
    void importUiProfile();
    void ensureWelcome(QTabWidget* tabs);
    void removeWelcome(QTabWidget* tabs);
    void toast(int type, const QString& msg); // 0 info 1 success 2 warning 3 error
    // Stage 11: editör-içi bulma + git gutter
    void showFindBar();
    void applyFindToEditor();
    void findNavigate(int dir);
    void refreshGitMarks(CodeEditor* e);
    // Stage 12: uyarlanabilir arayüz
    void openThemeEditor();
    void cycleDensity();
    void toggleFocusMode();
    void toggleTitleBar();
    void setCustomTitleBar(bool on);
    void exportPng();
    void showProfileMenu();
    void saveProfileAs();
    void applyProfile(const QString& name);
    void evaluateAutoTheme();   // zamanlayıcı + sistem sinyali
    void applyChips();
    void applySidePages();
    void applyBottomOrder();
    void saveBottomOrder();
    void updateProfileChip();
    // Stage 13: dil zekâsı
    void requestCompletion(bool autoTrigger = false);
    void showReferences();
    void renameSymbol();
    void showCodeActions();
    void showSignatureHelp(bool autoTrigger = false);
    void showDocSymbols();
    void showWorkspaceSymbols();
    void formatDocument();
    bool formatOnSaveIfEnabled(CodeEditor* e);
    void applyCodeAction(CodeEditor* e, const struct CodeActionItem& it);
    void toggleInlayHints();
    void toggleSemantic();
    void refreshInlayHints(CodeEditor* e);
    void refreshSemantic(CodeEditor* e);
    void showCallHierarchy();
    void onCompletionChosen(const struct CompletionItem& item);
    // Stage 13+: sunucu hazır değilse isteği hazır olana ertele (tek seferlik)
    void withLspReady(LspClient* c, std::function<void()> fn);
    // Stage 14: hata ayıklama & test
    void debugStart();
    void debugStop();
    void debugContinue();
    void debugNext();
    void debugStep();
    void debugFinish();
    void toggleBreakpoint(const QString& file, int line);
    void toggleBreakpointAtCursor();
    void onDebugStopped(const QString& reason, const struct DebugFrame& frame);
    void onDebugExited(int code);
    void syncBreakpointsToGdb();
    void applyBpMarks(CodeEditor* e);
    void applyBpMarksAll();
    void applyCoverageMarks(CodeEditor* e);
    void clearFrameMarks();
    void debugEvaluate(const QString& expr);
    void debugConsole(const QString& cmd);
    void refreshDebugVars();
    void refreshDebugVars(int frame);
    void discoverTests();
    void runAllTests();
    void runOneTest(const QString& testId);
    void stopTests();
    QString testLog() const;
    void runCoverage();
    void collectCoverage();
    void editLaunchConfig();
    void applyBuildProblems(const QString& output, const QString& source);
    // Stage 16: uzaktan geliştirme
    void remoteConnectDialog();
    void remoteConnect(const ConnectionProfile& p);
    void remoteDisconnect();
    bool remoteConnected() const;
    void remoteOpenFile(const QString& remotePath);
    bool remoteSaveEditor(class CodeEditor* e, bool interactive = true);
    void remoteOpenDialog();
    void remoteTerminalShow();
    void remoteLspStart();
    void remoteBuild();
    void remoteGitStatus();
    void remoteDebugStart();
    void remoteForwardShow();
    void debugStartRemote(const QString& localSymbols, const QString& host, int port);
    void updateRemoteStatus();
    // Stage 17: editör deneyimi
    void refreshDiagMinimap();
    void snippetPalette(bool manage = false);
    void insertSnippetDef(const SnippetDef& s);
    void outlineShow();
    void refreshOutline();
    void timelineShow();
    void timelineRestorePrev();
    void pasteAs();
    void hunkMenuAtCursor();
    void wordComplete();
    void foldingRefreshLsp();
    void searchExcludeFocus();
    void snapshotEditor(class CodeEditor* e); // Stage 17: kaydetmede anlık görüntü
    // Stage 15: AI hattı
    void toggleGhost();
    void requestGhost();
    void inlineEditSelection();
    void fixSelectionAi();
    void documentFunction();
    void genTestForCurrent();
    void commitMessageAi();
    void explainSymbol();
    void promptLibrary();
    void applyLastAi();
    void pullModel();
    void askAi(const QString& system, const QString& prompt, const QJsonObject& opts,
               std::function<void(QString, QString)> done);
    // Stage 37: görev bilgisiyle tek AI cephesi (AiRunner)
    void askAiTask(int taskId, const QString& system, const QString& prompt,
                   const QJsonObject& opts, std::function<void(QString, QString)> done);
    void cancelAi(); // Stage 37: ortak iptal
    void updateProviderChip();  // Stage 37: sağlayıcı çipini tazele
    void showProviderMenu();   // Stage 37: hızlı sağlayıcı değiştir

private:
    QString sideTitleFor(int i) const;
    CodeEditor* currentEditor() const;
    QList<CodeEditor*> allEditors() const;
    void refreshProblemView();
    QTabWidget* activeTabs() const;
    QTabWidget* makeTabWidget(int group);
    void closeTabIn(QTabWidget* tabs, int i);
    void onGroupCurrentChanged(int group);
    CodeEditor* openEditorFor(const QString& path, int group = -1);
    // Stage 46: tembel sekme geri yükleme
    void materializePending(QWidget* w);
    bool isPendingTab(QWidget* w) const;
    QMap<QString, int> m_pendingCursors;
    QMap<QString, QList<int>> m_pendingFolds;
    CodeEditor* addEditorTab(CodeEditor* e, const QString& title, const QString& tip,
                             int group = -1);
    void newUntitledFile(); // dil sorar + iskelet koyar (Kaydet'te ad önerir)
    void changeFileLanguage(); // açık sekmenin dilini değiştirir
    // Stage 21
    bool isPinnedTab(QTabWidget* tabs, int i) const;
    void togglePinTab(QTabWidget* tabs, int i);
    void onClipboardChanged();
    void pasteCyclePrevious();
    void multiCaretLineEnds();
    void toggleEol();
    void showEncodingInfo();
    // Stage 22
    void showShortcutDialog();
    void openPluginsDir();
    void loadPluginsDeferred();
    void runPluginCommand(const QString& id);
    void factoryReset();
    QString pluginDir() const;
    // Stage 29 v2
    QMap<QString, QString> m_pluginViews; // view cmdId → başlık
    void showPluginView(const QString& cmdId);
    void installPluginTheme(const QString& pid, const QString& name, const QString& json);
    void installPluginKeybinding(const QString& pid, const QString& cmdId, const QString& keys);
    void showPluginProblems(const QString& pid, const QString& json);
    void showPluginManager();
    // Stage 30
    void openMergeEditor();
    void importSshConfig();
    void searchAiChats();
    void showClipboardManager();
    void newProjectWizard();
    void newFileFromTemplate();
    QString fileTemplateDir() const;
    QStringList fileTemplateNames() const;
    void startAiSchedule();
    void remoteReconnect();
    void formatWithClangFormat(CodeEditor* e);
    QTimer* m_aiTimer = nullptr;
    // Stage 23
    void checkEditorGrid(CodeEditor* e);
    void chooseEditorFont();
    void checkCrashDumps(); // Stage 31
    // Stage 24
    void openPreview(const QString& path);
    // Stage 28
    struct AiUndo {
        QString path;
        QString oldText;
        QPointer<CodeEditor> editor;
        bool wholeFile = true;
        int selStart = 0;
        int selLen = 0;
    };
    void checkWorkspaceTrust();
    bool requireTrusted(const QString& what);
    bool m_trusted = true;
    void applyBmMarks(CodeEditor* e);
    void toggleBookmark();
    void jumpBookmark(int dir);
    void showBookmarks();
    void toggleComment();
    bool openSpecial(const QString& path);
    void showMarkdownPreview();
    void showHexView();
    void copyPath(int mode);
    void organizeImports();
    void showGitHistory();
    void showGitTags();
    void terminalFind(bool forward);
    void showNotifications();
    void pushRecentProject(const QString& root);
    void openRoot(const QString& root);
    void debugRunFile();
    void undoAiEdit();
    void pushAiUndo(const QString& path, const QString& oldText, CodeEditor* editor,
                    bool whole = true, int selStart = 0, int selLen = 0);
    BookmarkStore m_marks;
    QList<AiUndo> m_aiUndo;
    bool m_debugOverride = false;
    LaunchConfig m_overrideLc;
    void openProjectNotes();
    void showSymbolSearch();
    void showMetricsRadar();
    // Stage 25
    void bulkRenameHere();
    void reviewStagedDiff();
    void runRelatedTest(const QString& sourcePath);
    void startScheduledTasks();
    void loadExternalTools();
    void runExternalTool(const QString& id);
    void toggleMacroRecord();
    void playMacro();
    // Stage 26: hata ayıklama derinliği
    void debugSmartStep();
    void onDebugSmartCheck();
    void refreshWatches();
    void debugAddWatch(const QString& expr);
    void debugRemoveWatch(const QString& expr);
    void debugReadMemory(const QString& addr);
    void debugSelectThread(const QString& id);
    void debugSetVariable(const QString& expr);
    void editBreakpoint(const QString& file, int line);
    void debugFunctionBp();
    void debugWatchpoint();
    void debugAttach();
    void debugOpenCore();
    void debugSubstitutePath();
    // Stage 27: imza overload
    void showSigTooltip();
    void cycleSignature(int dir);
    // Stage 27: LSP derinliği
    void expandSelectionSmart();
    void openDocLink();
    void refreshDocColors(CodeEditor* e);
    void editColorBox(CodeEditor* e, int line0);
    void refreshCodeLens(CodeEditor* e);
    void showTypeHierarchy();
    void pullDiagnostics();
    void formatSelection();
    void showLspLog();
    void showLspServers();
    QTimer* m_lensTimer = nullptr;
    QMap<LspClient*, QList<qint64>> m_lspRestarts; // Stage 31: fırtına koruması
    void watchLspConfig(); // Stage 27
    QFileSystemWatcher* m_lspWatcher = nullptr;
    qint64 m_lspBurstMs = 0; // Stage 31
    int m_lspBurstCount = 0;
    // Stage 30: görev zinciri
    void runTaskChain();
    void runNextChainTask();
    QStringList m_chainQueue;
    bool m_chainActive = false;
    SignatureHelpData m_lastSig;
    QPointer<CodeEditor> m_lastSigEditor;
    // Stage 27: LSP derinliği
    void reopenLspDocs(LspClient* c, const QString& lang, const QStringList& suffixes);
    void onLspCrashed(LspClient* c, const QString& msg);
    void onLspProgress(const QString& kind, const QString& title, int percent);
    void startPull(const QString& model); // Stage 25: kuyruklu indirme
    void applyEditorSettingsToAll();
    void applyEditorSettingsTo(CodeEditor* e);
    void applyShortcuts();
    void saveSession();
    void restoreSession();
    void refreshProjectViews(); // gitignore + tüm proje panelleri yeni köke
    QStringList collectProjectFiles() const;
    void openFolderDialog();

    // LSP
    LspClient* lspClientFor(const QString& suffix, QString& langId);
    void setupLspFor(const QString& path, const QString& text);
    void pushDocToLsp(CodeEditor* editor, bool immediate = true);
    void flushLspDocs(); // Stage 27: ertelenmiş didChange
    void teardownLsp(const QString& path);

    QAction* m_actMinimap = nullptr;
    QAction* m_actTerminal = nullptr;
    QAction* m_actCrumb = nullptr;      // Stage 10: yerleşim ön ayarları
    QWidget* m_activity = nullptr;      // Stage 10: aktivite çubuğu (zen gizler)
    QWidget* m_sideWrap = nullptr;      // Stage 10: yan panel sarmalayıcısı
    QString m_lastLayout = "standart";  // zen öncesi yerleşim
    QMap<QTabWidget*, class WelcomeView*> m_welcomeViews; // Stage 10
    QStackedWidget* m_side;
    QTabWidget* m_tabs;
    QTabWidget* m_tabs2;
    int m_activeGroup = 0;
    ExplorerPanel* m_explorer;
    SearchPanel* m_search;
    GitPanel* m_git;
    AiPanel* m_ai;
    ClipboardRing m_clipRing; // Stage 21: pano halkası
    class PluginEngine* m_plugins = nullptr; // Stage 22: eklenti motoru
    QMap<QString, QString> m_pluginCmds;     // Stage 22: eklenti komutları
    QLabel* m_chipGrid = nullptr; // Stage 23: ızgara durumu
    bool m_gridOk = true;         // Stage 23
    // Stage 25
    MacroRecorder m_macro;
    bool m_macroPlaying = false;
    TaskRunner* m_schedRunner = nullptr;    QList<QTimer*> m_schedTimers;
    QMap<QString, ToolDef> m_toolCmds;
    QProcess* m_toolProc = nullptr;
    // Stage 27: LSP erteleme
    QList<QPointer<CodeEditor>> m_lspDirty;
    QTimer* m_lspFlushTimer = nullptr;
    QLabel* m_chipAi = nullptr;
    QStringList m_pullQueue;
    bool m_pullActive = false;
    ProblemsPanel* m_problems;
    BreadcrumbBar* m_crumb;
    Minimap* m_minimap;
    class EditorFindBar* m_findBar = nullptr; // Stage 11
    QString m_lastFind;                       // Stage 11: son arama sorgusu
    QList<FindHit> m_findHits;
    int m_findCur = -1;
    TerminalPanel* m_terminal;
    QDockWidget* m_termDock;
    QTabWidget* m_bottomTabs;
    TodoPanel* m_todo;
    HistoryTab* m_history;
    StashTab* m_stashTab;
    RemoteTab* m_remoteTab;
    TaskPanel* m_taskPanel = nullptr;
    BackupManager* m_backups = nullptr;
    // Stage 9: durum çubuğu çipleri
    QLabel* m_chipGit = nullptr;
    QLabel* m_chipProblems = nullptr;
    QLabel* m_chipLang = nullptr;
    QLabel* m_chipEol = nullptr;
    QLabel* m_chipEnc = nullptr;
    QLabel* m_chipProfile = nullptr; // Stage 12: hızlı profil değiştirici
    class TitleBar* m_titleBar = nullptr; // Stage 12
    QTimer* m_autoTimer = nullptr;        // Stage 12: otomatik tema
    GitIgnore* m_gitIgnore = nullptr;
    TrashManager* m_trash = nullptr;
    ProjectSessions* m_sessions = nullptr;
    QSettings* m_sessionStore = nullptr;
    LspClient* m_lspCpp = nullptr;
    LspClient* m_lspPy = nullptr;
    LspClient* m_lspRs = nullptr; // Stage 27
    LspClient* m_lspGo = nullptr; // Stage 27
    LspClient* m_lspJs = nullptr; // Stage 27
    LspClient* m_lspRemote = nullptr; // Stage 16: uzak LSP (ssh stdio köprüsü)
    QMap<QString, QList<LspDiag>> m_diags;
    QMap<QString, QList<LspDiag>> m_spellDiags; // yazım denetimi (source=yazım)
    QMap<QString, QAction*> m_actions;
    bool m_lspWarned = false;
    // Stage 13: dil zekâsı yardımcıları
    class CompletionPopup* m_completion = nullptr;
    QTimer* m_completeTimer = nullptr;
    QTimer* m_sigTimer = nullptr;
    QTimer* m_inlayTimer = nullptr;
    int m_pendingCompleteId = -1;
    QStringList m_semLegend;
    QString m_lastSymbol;
    // Stage 14: hata ayıklama & test
    class GdbDriver* m_gdb = nullptr;
    class DebugPanel* m_debug = nullptr;
    class TestExplorer* m_tests = nullptr;
    class QProcess* m_testProc = nullptr;
    struct TestRunner m_testRunner;
    bool m_hasRunner = false;
    QList<struct TestCase> m_testCases;
    QMap<QString, QString> m_gdbBpNums; // "dosya:satır" → gdb numarası
    // Stage 26: hata ayıklama durumu
    QElapsedTimer m_debugT0;
    bool m_preBuildDone = false;
    bool m_debugSkipLib = true;
    int m_smartDepth = 0;
    QMap<QString, QString> m_watchNames; // ifade → varobj adı
    QString m_hoverWord;
    QPoint m_hoverPos;
    QString m_curThread; // Stage 26: seçili thread id
    struct DebugFrame m_curFrame;
    bool m_hasFrame = false;
    QMap<QString, QMap<int, int>> m_coverage; // dosya → satır → hits
    QMap<QString, QList<LspDiag>> m_buildDiags; // derleme/test sorunları
    // Stage 15: AI hattı
    class OllamaClient* m_ghostAi = nullptr; // hayalet tamamlama (ayrı istemci)
    class OllamaClient* m_flowAi = nullptr;  // tek seferlik AI akışları
    class AiRunner* m_aiRunner = nullptr;       // Stage 37: tek AI cephesi
    class QLabel* m_chipProvider = nullptr;    // Stage 37: sağlayıcı + sağlık noktası
    class QLabel* m_chipRag = nullptr;         // Stage 40: RAG rozeti
    class AiRunner* m_ghostRunner = nullptr;    // Stage 37: hayalet tamamlama koşucusu
    class SecretStore* m_secrets = nullptr;     // Stage 37: anahtar kasası
    // Stage 49: birlikte çalışma
    class CollabSession* m_collab = nullptr;
    bool m_collabApplying = false;
    QTimer* m_collabCursorTimer = nullptr;
    CollabSession* collab();
    QStringList collabPeers() const;
    void collabHost();
    void collabJoin();
    void collabLeave();
    void renderPeerCursors();
    void applyCollabText(const QString& text, const QString& from);
    void publishCollabEdit();
    void publishCollabCursor();
    QTimer* m_ghostTimer = nullptr;
    int m_ghostReq = 0;
    QString m_ghostPrefix;
    QTimer* m_outlineTimer = nullptr; // Stage 17: outline debounce
    QPointer<class CodeEditor> m_ghostEditor;
    bool m_flowBusy = false;
    // Stage 16: uzaktan geliştirme
    SshSession* m_ssh = nullptr;
    PortForwarder* m_fwd = nullptr;
    RemoteExplorer* m_remoteExplorer = nullptr;
    RemoteTerminal* m_remoteTerm = nullptr;
    PortForwardPanel* m_fwdPanel = nullptr;
    QPushButton* m_btnRemote = nullptr;
    QPushButton* m_btnOutline = nullptr; // Stage 17
    class OutlinePanel* m_outline = nullptr; // Stage 17
    class TimelinePanel* m_timeline = nullptr; // Stage 17
    class LocalHistory* m_localHist = nullptr; // Stage 17: kaydetmede anlık görüntü
    ConnectionProfile m_remoteProfile;
    QMap<QString, QDateTime> m_remoteMtimes; // uzak yol → açılış mtime (çakışma denetimi)
    QLabel* m_status;
    QLabel* m_cursorLabel;
    QTimer* m_autosave;
    QString m_root;
    QPushButton* m_btnExplorer;
    QPushButton* m_btnSearch;
    QPushButton* m_btnGit;
    QPushButton* m_btnAi;
    QPushButton* m_btnProblems;
    QPushButton* m_btnSettings;
    QLabel* m_sideTitle;
};
