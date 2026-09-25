#pragma once
#include "core/ConnectionProfile.h"
#include "core/GdbDriver.h"
#include "core/SnippetManager.h"
#include "core/LspClient.h"
#include "core/SearchMarks.h"
#include "core/TestDiscovery.h"
#include "core/TestParser.h"
#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QPointer>
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
    CodeEditor* addEditorTab(CodeEditor* e, const QString& title, const QString& tip,
                             int group = -1);
    void newUntitledFile(); // isimsiz yeni sekme (Kaydet'te Farklı-Kaydet sorar)
    void applyEditorSettingsToAll();
    void applyEditorSettingsTo(CodeEditor* e);
    void applyShortcuts();
    void saveSession();
    void restoreSession();
    DocSession captureSession() const;
    void applyDocSession(const DocSession& s);
    void refreshProjectViews(); // gitignore + tüm proje panelleri yeni köke
    QStringList collectProjectFiles() const;
    void openFolderDialog();

    // LSP
    LspClient* lspClientFor(const QString& suffix, QString& langId);
    void setupLspFor(const QString& path, const QString& text);
    void pushDocToLsp(CodeEditor* editor);
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
    struct DebugFrame m_curFrame;
    bool m_hasFrame = false;
    QMap<QString, QMap<int, int>> m_coverage; // dosya → satır → hits
    QMap<QString, QList<LspDiag>> m_buildDiags; // derleme/test sorunları
    // Stage 15: AI hattı
    class OllamaClient* m_ghostAi = nullptr; // hayalet tamamlama (ayrı istemci)
    class OllamaClient* m_flowAi = nullptr;  // tek seferlik AI akışları
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
