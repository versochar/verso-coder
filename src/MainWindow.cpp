#include "MainWindow.h"
#include "core/AboutInfo.h"
#include "core/BackupManager.h"
#include "core/Commands.h"
#include "core/GitIgnore.h"
#include "core/KeymapPresets.h"
#include "core/LanguageManager.h"
#include "core/LspClient.h"
#include "core/OllamaClient.h"
#include "core/ai/AiProfiles.h"
#include "core/ai/AiRunner.h"
#include "core/ai/ProviderHealth.h"
#include "core/ai/ProviderPrefs.h"
#include "core/ai/UsageLedger.h"
#include "core/ai/SecretStore.h"
#include "core/SshSession.h"
#include "core/PortForwarder.h"
#include "core/RemoteFileSystem.h"
#include "core/RemoteTaskRunner.h"
#include "core/TaskRunner.h"
#include "core/LocalHistory.h"
#include "core/FoldingRanges.h"
#include "core/PasteTransform.h"
#include "core/SnippetEngine.h"
#include "core/WordComplete.h"
#include "core/AutoPairs.h"
#include "core/LspTransport.h"
#include "core/GitRunner.h"
#include "core/LanguageSupport.h"
#include "core/ClipboardRing.h"
#include "core/PerfMonitor.h"
#include "core/ProjectSessions.h"
#include "core/SettingsManager.h"
#include "core/SettingsIO.h"
#include "core/ShortcutCheck.h"
#include "core/SpellChecker.h"
#include "core/Spelling.h"
#include "core/StartupArgs.h"
#include "core/ThemeManager.h"
#include "core/UpdateChecker.h"
#include "core/ThemeStore.h"
#include "core/IconTheme.h"
#include "core/Animator.h"
#include "core/AutoTheme.h"
#include "core/Density.h"
#include "core/DiffGutter.h"
#include "core/HoverCard.h"
#include "core/LayoutPresets.h"
#include "core/ProfileStore.h"
#include "core/SearchMarks.h"
#include "core/BreakpointStore.h"
#include "core/CallHierarchyTree.h"
#include "core/ContextBudget.h"
#include "core/CommitMsg.h"
#include "core/DocGen.h"
#include "core/GhostCompletion.h"
#include "core/ai/AiRunner.h"
#include "core/InlineEdit.h"
#include "core/PromptLibrary.h"
#include "core/TestGen.h"
#include "core/CodeActionList.h"
#include "core/CompletionList.h"
#include "core/GcovParser.h"
#include "core/GdbDriver.h"
#include "core/InlayHintList.h"
#include "core/LaunchConfig.h"
#include "core/LocationSet.h"
#include "core/ProblemMatcher.h"
#include "core/SemanticTokens.h"
#include "core/SignatureHelp.h"
#include "core/SymbolTree.h"
#include "core/TestDiscovery.h"
#include "core/TestParser.h"
#include "core/TextEdits.h"
#include "core/ThemeStore.h"
#include "core/ToastManager.h"
#include "core/UiMetrics.h"
#include "core/UiProfile.h"
#include "core/TrashManager.h"
#include "core/Typography.h"
#include "core/WorkspaceConfig.h"
#include "widgets/CallHierarchyDialog.h"
#include "widgets/CompletionPopup.h"
#include "widgets/DebugPanel.h"
#include "widgets/PortForwardPanel.h"
#include "widgets/PromptLibraryDialog.h"
#include "widgets/OutlinePanel.h"
#include "widgets/RemoteConnectDialog.h"
#include "widgets/RemoteExplorer.h"
#include "widgets/RemoteTerminal.h"
#include "widgets/SnippetDialog.h"
#include "widgets/TimelinePanel.h"
#include "widgets/ReferencesDialog.h"
#include "widgets/RenamePreviewDialog.h"
#include "widgets/SymbolPickerDialog.h"
#include "widgets/TestExplorer.h"
#include "widgets/AiPanel.h"
#include "widgets/ApplyEditDialog.h"
#include "widgets/BreadcrumbBar.h"
#include "widgets/CodeEditor.h"
#include "widgets/EditorFindBar.h"
#include "widgets/ThemeEditorDialog.h"
#include "widgets/TitleBar.h"
#include "widgets/CommandPalette.h"
#include "widgets/CompareDialog.h"
#include "widgets/DiagnosticsDialog.h"
#include "widgets/DiffDialog.h"
#include "widgets/ExplorerPanel.h"
#include "widgets/GitPanel.h"
#include "widgets/GitTools.h"
#include "widgets/Minimap.h"
#include "widgets/ProblemsPanel.h"
#include "widgets/QuickOpenDialog.h"
#include "widgets/SearchPanel.h"
#include "widgets/SettingsDialog.h"
#include "widgets/TaskPanel.h"
#include "widgets/TerminalPanel.h"
#include "widgets/ThemeGalleryDialog.h"
#include "widgets/NewFileDialog.h"
#include "widgets/FirstRunDialog.h"
#include "widgets/ShortcutDialog.h"
#include "widgets/SymbolSearchDialog.h"
#include "widgets/BookmarkDialog.h"
#include "widgets/MarkdownPreviewDialog.h"
#include "widgets/ImageViewerDialog.h"
#include "widgets/HexViewDialog.h"
#include "widgets/GitHistoryDialog.h"
#include "widgets/PluginManagerDialog.h"
#include "widgets/NewProjectDialog.h"
#include "widgets/ClipboardDialog.h"
#include "widgets/MergeEditorDialog.h"
#include "core/SshConfig.h"
#include "widgets/GitTagsDialog.h"
#include "widgets/MetricsDialog.h"
#include "widgets/BulkRenameDialog.h"
#include "core/ExternalTools.h"
#include "core/MacroRecorder.h"
#include "core/PromptVars.h"
#include "core/WorkspaceSymbols.h"
#include "core/PluginEngine.h"
#include "core/CollabSession.h"
#include <QDesktopServices>
#include "widgets/WelcomeView.h"
#include "widgets/TodoPanel.h"
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QEvent>
#include <QMenu>
#include <QDir>
#include <QDirIterator>
#include <QDockWidget>
#include <QFileDialog>
#include <QFontDialog>
#include <QFontInfo>
#include <QFutureWatcher>
#include <QHelpEvent>
#include <QtConcurrent>
#include <QColorDialog>
#include <QDateTime>
#include <QTemporaryFile>
#include <QTextBrowser>
#include <QFileInfo>
#include <QFile>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QSplitter>
#include <QStyleHints>
#include <QTabBar>
#include <QTime>
#include <QStackedWidget>
#include <QStatusBar>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTimer>
#include <QToolTip>
#include <QVBoxLayout>
#include <utility>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    AppSettings s = SettingsManager::instance().load();
    LanguageManager::instance().setLanguage(s.language);
    // Stage 9: tipografi + accent → token hattı
    TypographySettings typo;
    typo.uiFamily = s.uiFontFamily;
    typo.editorFamily = s.editorFontFamily;
    typo.uiSize = s.uiFontSize;
    typo.lineHeight = s.lineHeight;
    typo.letterSpacing = s.letterSpacing;
    typo.ligatures = s.ligatures;
    ThemeManager::instance().setTypography(typo);
    ThemeManager::instance().setSelectionOpacity(s.selectionOpacity);
    ThemeManager::instance().setAccent(s.accentColor.isEmpty() ? QColor() : QColor(s.accentColor));
    ThemeManager::instance().apply(s.theme);
    // Stage 21: ilk-çalıştırma sihirbazı
    if (s.m_firstRun) {
        FirstRunDialog wiz(this);
        if (wiz.exec() == QDialog::Accepted) s = SettingsManager::instance().load();
        else { s.m_firstRun = false; SettingsManager::instance().save(s); }
        LanguageManager::instance().setLanguage(s.language);
        ThemeManager::instance().apply(s.theme);
    }
    m_root = s.lastRoot.isEmpty() ? QDir::homePath() : s.lastRoot;

    // Stage 6 servisleri: .gitignore, çöp kutusu, proje oturumları
    m_gitIgnore = new GitIgnore();
    m_trash = new TrashManager();
    m_sessionStore = new QSettings("Verso", "VersoCoderSessions", this);
    m_sessions = new ProjectSessions(m_sessionStore);
    // Stage 8: zaman damgalı yedekler
    m_backups = new BackupManager(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backups");
    m_marks.load(); // Stage 28: yer imleri

    resize(1280, 800);
    setWindowTitle("Verso Coder");

    m_lspCpp = new LspClient(this);
    m_lspPy = new LspClient(this);
    m_lspRs = new LspClient(this);  // Stage 27
    m_lspGo = new LspClient(this);  // Stage 27
    m_lspJs = new LspClient(this);  // Stage 27
    m_lspRemote = new LspClient(this); // Stage 16: ssh köprülü uzak LSP
    // Stage 27: çökme kurtarma + ilerleme
    for (LspClient* c : {m_lspCpp, m_lspPy, m_lspRs, m_lspGo, m_lspJs}) {
        connect(c, &LspClient::progressUpdate, this, &MainWindow::onLspProgress);
        connect(c, &LspClient::serverError, this, [this, c](const QString& e) {
            onLspCrashed(c, e);
        });
    }
    connect(m_lspCpp, &LspClient::diagnosticsReady, this, &MainWindow::onDiagnostics);
    connect(m_lspPy, &LspClient::diagnosticsReady, this, &MainWindow::onDiagnostics);
    // Stage 16: uzak tanılar ssh:// URI'ye çevrilip aynı havuza girer
    connect(m_lspRemote, &LspClient::diagnosticsReady, this,
            [this](const QString& path, const QList<LspDiag>& diags) {
                onDiagnostics(m_remoteProfile.toUri(path), diags);
            });

    // --- Komut eylemleri (kısayollar ayarlardan uygulanır) ---
    auto mkAct = [this](const QString& id, const QString& title, auto&& fn) {
        QAction* a = new QAction(title, this);
        a->setObjectName(id); // Stage 38: testler/eklenti erişimi için kimlik
        connect(a, &QAction::triggered, this, std::forward<decltype(fn)>(fn));
        addAction(a); // pencere genelinde kısayol
        m_actions[id] = a;
        return a;
    };
    mkAct("file.openFolder", "Klasör Aç...", [this]() { openFolderDialog(); });
    mkAct("file.new", "Yeni Dosya", [this]() { newUntitledFile(); });
    mkAct("file.language", "Dosya Dilini Değiştir", [this]() { changeFileLanguage(); });
    mkAct("file.encodingInfo", "Dosya Kodlama Bilgisi", [this]() { showEncodingInfo(); });
    mkAct("edit.toggleEol", "Satır Sonu Değiştir", [this]() { toggleEol(); });
    mkAct("edit.pasteCycle", "Önceki Panoyu Yapıştır", [this]() { pasteCyclePrevious(); });
    mkAct("edit.multiCaretLineEnds", "Satır Sonlarına İmleç", [this]() { multiCaretLineEnds(); });
    mkAct("ai.exportChat", "Sohbeti Markdown Dışa Aktar", [this]() { if (m_ai) m_ai->exportChatMarkdown(); });
    mkAct("view.font", "Editör Yazı Tipi...", [this]() { chooseEditorFont(); });
    // Stage 24-25
    mkAct("nav.symbol", "Sembol Ara", [this]() { runCommand("nav.symbol"); });
    mkAct("view.radar", "Proje Radarı", [this]() { runCommand("view.radar"); });
    mkAct("file.projectNotes", "Proje Notları", [this]() { runCommand("file.projectNotes"); });
    mkAct("file.bulkRename", "Toplu Yeniden Adlandır", [this]() { runCommand("file.bulkRename"); });
    mkAct("ai.compare", "AI Karşılaştırma", [this]() { runCommand("ai.compare"); });
    mkAct("ai.undoAnswer", "Son Yanıtı Geri Al", [this]() { runCommand("ai.undoAnswer"); });
    // Stage 28
    mkAct("edit.toggleComment", "Satırı Yoruma Al/Çıkar", [this]() { runCommand("edit.toggleComment"); });
    mkAct("edit.bookmarkToggle", "Yer İmi Aç/Kapa", [this]() { runCommand("edit.bookmarkToggle"); });
    mkAct("edit.bookmarkNext", "Sonraki Yer İmi", [this]() { runCommand("edit.bookmarkNext"); });
    mkAct("edit.bookmarkPrev", "Önceki Yer İmi", [this]() { runCommand("edit.bookmarkPrev"); });
    mkAct("view.bookmarks", "Yer İmleri", [this]() { runCommand("view.bookmarks"); });
    mkAct("view.markdownPreview", "Markdown Önizleme", [this]() { runCommand("view.markdownPreview"); });
    mkAct("file.openHex", "Hex Görünümü", [this]() { runCommand("file.openHex"); });
    mkAct("file.copyPath", "Tam Yolu Kopyala", [this]() { runCommand("file.copyPath"); });
    mkAct("file.copyRelativePath", "Göreli Yolu Kopyala", [this]() { runCommand("file.copyRelativePath"); });
    mkAct("file.copyFileName", "Dosya Adını Kopyala", [this]() { runCommand("file.copyFileName"); });
    mkAct("edit.organizeImports", "Importları Düzenle", [this]() { runCommand("edit.organizeImports"); });
    mkAct("git.history", "Dosya Geçmişi", [this]() { runCommand("git.history"); });
    // Stage 49: birlikte çalışma (yerel ağ, salt-davet; LAN güveni varsayılır)
    mkAct("collab.host", "Birlikte Çalış: Oturum Barındır", [this]() { collabHost(); });
    mkAct("collab.join", "Birlikte Çalış: Oturuma Katıl", [this]() { collabJoin(); });
    mkAct("collab.leave", "Birlikte Çalış: Ayrıl", [this]() { collabLeave(); });
    mkAct("git.tags", "Etiketler", [this]() { runCommand("git.tags"); });
    mkAct("term.find", "Terminalde Bul", [this]() { runCommand("term.find"); });
    mkAct("term.findNext", "Sonraki Eşleşme (terminal)", [this]() { runCommand("term.findNext"); });
    mkAct("term.findPrev", "Önceki Eşleşme (terminal)", [this]() { runCommand("term.findPrev"); });
    mkAct("view.notifications", "Bildirimler", [this]() { runCommand("view.notifications"); });
    mkAct("file.projectNotes", "Proje Notları", [this]() { runCommand("file.projectNotes"); });
    mkAct("file.bulkRename", "Toplu Yeniden Adlandır", [this]() { runCommand("file.bulkRename"); });
    mkAct("debug.runFile", "Dosyayı Hata Ayıklayıcıda Çalıştır", [this]() { runCommand("debug.runFile"); });
    mkAct("ai.undoEdit", "AI Düzenlemesini Geri Al", [this]() { runCommand("ai.undoEdit"); });
    mkAct("workspace.trust", "Çalışma Alanı Güveni...", [this]() { runCommand("workspace.trust"); });
    // Stage 30
    mkAct("edit.merge", "Birleştirme Düzenleyicisi", [this]() { runCommand("edit.merge"); });
    mkAct("search.editor", "Arama Düzenleyicisi", [this]() { runCommand("search.editor"); });
    mkAct("file.newProject", "Yeni Proje...", [this]() { runCommand("file.newProject"); });
    mkAct("file.newFromTemplate", "Şablondan Yeni Dosya", [this]() { runCommand("file.newFromTemplate"); });
    mkAct("term.newTab", "Yeni Terminal Sekmesi", [this]() { runCommand("term.newTab"); });
    mkAct("remote.importSsh", "SSH Config İçe Aktar", [this]() { runCommand("remote.importSsh"); });
    mkAct("view.clipboard", "Pano Geçmişi", [this]() { runCommand("view.clipboard"); });
    mkAct("task.chain", "Görev Zincirini Çalıştır", [this]() { runCommand("task.chain"); });
    mkAct("remote.reconnect", "Yeniden Bağlan", [this]() { runCommand("remote.reconnect"); });
    mkAct("ai.searchChats", "Sohbetlerde Ara", [this]() { runCommand("ai.searchChats"); });
    mkAct("ai.reviewStaged", "AI Kod İncelemesi", [this]() { runCommand("ai.reviewStaged"); });
    mkAct("test.runRelated", "İlgili Testi Çalıştır", [this]() { runCommand("test.runRelated"); });
    mkAct("macro.record", "Makro Kaydet/Durdur", [this]() { runCommand("macro.record"); });
    mkAct("macro.play", "Makroyu Oynat", [this]() { runCommand("macro.play"); });
    mkAct("macro.clear", "Makroyu Temizle", [this]() { runCommand("macro.clear"); });
    mkAct("file.save", "Kaydet", [this]() { saveCurrent(); });
    mkAct("file.saveAll", "Tümünü Kaydet", [this]() { saveAll(); });
    mkAct("nav.quickOpen", "Hızlı Aç", [this]() { showQuickOpen(); });
    mkAct("nav.palette", "Komut Paleti", [this]() { showPalette(); });
    mkAct("nav.search", "Projede Ara", [this]() { runCommand("nav.search.focus"); });
    mkAct("nav.gotoLine", "Satıra Git", [this]() { gotoLineDialog(); });
    mkAct("view.minimap", "Minimap Aç/Kapa", [this]() { m_actMinimap->toggle(); });
    mkAct("view.terminal", "Terminal Aç/Kapa", [this]() { m_termDock->setVisible(!m_termDock->isVisible()); });
    mkAct("run.build", "Derle & Çalıştır", [this]() { runBuildForCurrent(); });
    mkAct("lsp.definition", "Tanıma Git", [this]() { lspDefinition(); });
    mkAct("lsp.hover", "Sembol Bilgisi", [this]() { lspHover(); });
    mkAct("nav.splitRight", "Editörü Böl", [this]() { splitActiveToOther(); });
    mkAct("nav.moveTab", "Sekmeyi Diğer Gruba Taşı", [this]() { moveActiveTab(); });
    mkAct("edit.moveUp", "Satırı Yukarı Taşı", [this]() {
        if (auto* e = currentEditor()) e->moveLineOrSelection(-1); });
    mkAct("edit.moveDown", "Satırı Aşağı Taşı", [this]() {
        if (auto* e = currentEditor()) e->moveLineOrSelection(1); });
    mkAct("edit.duplicate", "Satırı Çoğalt", [this]() {
        if (auto* e = currentEditor()) e->duplicateLineOrSelection(); });
    mkAct("edit.sort", "Satırları Sırala", [this]() {
        if (auto* e = currentEditor()) e->sortSelectedLines(); });
    mkAct("edit.trim", "Sondaki Boşlukları Temizle", [this]() {
        if (auto* e = currentEditor()) e->trimTrailingWhitespace(); });
    mkAct("edit.fold", "Katla", [this]() {
        if (auto* e = currentEditor()) e->foldAtCursor(); });
    mkAct("edit.unfold", "Aç", [this]() {
        if (auto* e = currentEditor()) e->unfoldAtCursor(); });
    mkAct("edit.foldAll", "Tümünü Katla", [this]() {
        if (auto* e = currentEditor()) e->foldAll(); });
    mkAct("edit.unfoldAll", "Tümünü Aç", [this]() {
        if (auto* e = currentEditor()) e->unfoldAll(); });
    mkAct("edit.addNext", "Sonraki Eşleşmeyi Seç", [this]() {
        if (auto* e = currentEditor()) e->selectNextOccurrence(); });
    mkAct("edit.spell", "Yazımı Denetle", [this]() { checkSpelling(); });
    mkAct("large.loadFull", "Tamamını Yükle", [this]() { loadFullCurrent(); });
    // Stage 11: editör-içi bulma + akıllı seçim
    mkAct("edit.find", "Bul (editör içi)", [this]() { showFindBar(); });
    mkAct("edit.findNext", "Sonraki Eşleşme", [this]() { findNavigate(+1); });
    mkAct("edit.findPrev", "Önceki Eşleşme", [this]() { findNavigate(-1); });
    mkAct("edit.expandSel", "Akıllı Seçimi Genişlet", [this]() { expandSelectionSmart(); });
    // Stage 12: uyarlanabilir arayüz
    mkAct("view.themeEditor", "Tema Düzenleyici...", [this]() { openThemeEditor(); });
    mkAct("view.cycleDensity", "Yoğunluk Değiştir", [this]() { cycleDensity(); });
    mkAct("view.focusMode", "Odak Modu (geçiş)", [this]() { toggleFocusMode(); });
    mkAct("view.titleBar", "Özel Başlık Çubuğu (geçiş)", [this]() { toggleTitleBar(); });
    mkAct("view.highContrast", "Yüksek Kontrast Teması", [this]() {
        AppSettings s = SettingsManager::instance().load();
        s.theme = "high-contrast";
        SettingsManager::instance().save(s);
        ThemeManager::instance().apply("high-contrast");
        toast(1, "Yüksek kontrast teması uygulandı");
    });
    mkAct("ui.exportPng", "Pencereyi PNG Olarak Kaydet...", [this]() { exportPng(); });
    mkAct("ui.profiles", "Görünüm Profilleri", [this]() { showProfileMenu(); });
    mkAct("ui.saveProfile", "Geçerli Görünümü Profil Olarak Kaydet...", [this]() { saveProfileAs(); });
    // Palet-komutları: QAction da alır (kısayol atanabilir + menüde görünür)
    mkAct("proj.compare", "Klasör Karşılaştır...", [this]() { runCommand("proj.compare"); });
    mkAct("session.saveSnapshot", "Anlık Görüntü Kaydet", [this]() { runCommand("session.saveSnapshot"); });
    mkAct("session.loadSnapshot", "Anlık Görüntü Yükle", [this]() { runCommand("session.loadSnapshot"); });
    mkAct("view.tasks", "Görevler (TODO) Paneli", [this]() { runCommand("view.tasks"); });
    mkAct("ai.agent", "AI Ajan Modu Aç/Kapa", [this]() { runCommand("ai.agent"); });
    mkAct("help.about", "Sistem Teşhisi / Hakkında", [this]() { runCommand("help.about"); });
    mkAct("help.shortcuts", "Klavye Kısayolları", [this]() { showShortcutDialog(); });
    mkAct("file.pluginsDir", "Eklentiler Klasörünü Aç", [this]() { openPluginsDir(); });
    mkAct("view.plugins", "Eklenti Yöneticisi", [this]() { runCommand("view.plugins"); });
    mkAct("file.factoryReset", "Fabrika Ayarlarına Dön...", [this]() { factoryReset(); });
    mkAct("task.run", "Görevi Çalıştır (tasks.json)", [this]() { runCommand("task.run"); });
    mkAct("task.panel", "Görev Panelini Aç", [this]() { runCommand("task.panel"); });
    mkAct("view.theme", "Tema Galerisi", [this]() { runCommand("view.theme"); });
    mkAct("view.git", "Git Panelini Aç", [this]() { runCommand("view.git"); });
    mkAct("view.problems", "Sorunlar Panelini Aç", [this]() { runCommand("view.problems"); });
    mkAct("file.settings", "Ayarlar", [this]() { runCommand("file.settings"); });
    mkAct("view.zen", "Zen Modu (geçiş)", [this]() { runCommand("view.zen"); });
    mkAct("view.zoomIn", "Yakınlaştır", [this]() { runCommand("view.zoomIn"); });
    mkAct("view.zoomOut", "Uzaklaştır", [this]() { runCommand("view.zoomOut"); });
    mkAct("view.zoomReset", "Yakınlaştırmayı Sıfırla", [this]() { runCommand("view.zoomReset"); });
    mkAct("ui.exportProfile", "Görünüm Profilini Dışa Aktar...", [this]() { runCommand("ui.exportProfile"); });
    mkAct("ui.importProfile", "Görünüm Profili İçe Aktar...", [this]() { runCommand("ui.importProfile"); });
    // Stage 13: dil zekâsı
    mkAct("edit.complete", "Tamamlamayı Öner", [this]() { requestCompletion(false); });
    mkAct("lsp.references", "Tüm Referanslar", [this]() { showReferences(); });
    mkAct("lsp.rename", "Sembolü Yeniden Adlandır", [this]() { renameSymbol(); });
    mkAct("lsp.codeAction", "Hızlı Düzeltme...", [this]() { showCodeActions(); });
    mkAct("lsp.signature", "İmza Yardımı", [this]() { showSignatureHelp(false); });
    mkAct("lsp.sigNext", "Sonraki İmza", [this]() { runCommand("lsp.sigNext"); });
    mkAct("lsp.sigPrev", "Önceki İmza", [this]() { runCommand("lsp.sigPrev"); });
    mkAct("lsp.docSymbols", "Belge Simgeleri", [this]() { showDocSymbols(); });
    mkAct("lsp.wsSymbols", "Çalışma Alanı Simgesi", [this]() { showWorkspaceSymbols(); });
    mkAct("format.document", "Belgeyi Biçimlendir", [this]() { formatDocument(); });
    mkAct("lsp.calls", "Çağrı Hiyerarşisi", [this]() { showCallHierarchy(); });
    // Stage 27
    mkAct("lsp.openLink", "Bağlantıyı Aç (include/import)", [this]() { runCommand("lsp.openLink"); });
    mkAct("lsp.typeHierarchy", "Tip Hiyerarşisi", [this]() { runCommand("lsp.typeHierarchy"); });
    mkAct("lsp.pullDiagnostics", "Tanıları Yenile (çek)", [this]() { runCommand("lsp.pullDiagnostics"); });
    mkAct("format.selection", "Seçimi Biçimlendir", [this]() { runCommand("format.selection"); });
    mkAct("lsp.log", "LSP Trafiğini Göster", [this]() { runCommand("lsp.log"); });
    mkAct("lsp.servers", "Dil Sunucuları...", [this]() { runCommand("lsp.servers"); });
    mkAct("view.inlayHints", "Satır İçi İpuçları (aç/kapa)", [this]() { toggleInlayHints(); });
    mkAct("view.semantic", "Semantik Renklendirme (aç/kapa)", [this]() { toggleSemantic(); });
    // Stage 14: hata ayıklama & test
    mkAct("debug.start", "Hata Ayıklamayı Başlat", [this]() { debugStart(); });
    mkAct("debug.stop", "Hata Ayıklamayı Durdur", [this]() { debugStop(); });
    mkAct("debug.continue", "Devam Et", [this]() { debugContinue(); });
    mkAct("debug.stepOver", "Adım Üstü", [this]() { debugNext(); });
    mkAct("debug.stepInto", "Adım İçi", [this]() { debugStep(); });
    mkAct("debug.stepOut", "Bitir", [this]() { debugFinish(); });
    mkAct("debug.toggleBp", "Kesme Noktası Aç/Kapa", [this]() { toggleBreakpointAtCursor(); });
    // Stage 26
    mkAct("debug.smartStep", "Akıllı Adım", [this]() { runCommand("debug.smartStep"); });
    mkAct("debug.functionBp", "Fonksiyon Kesmesi", [this]() { runCommand("debug.functionBp"); });
    mkAct("debug.watchpoint", "İzleme Noktası", [this]() { runCommand("debug.watchpoint"); });
    mkAct("debug.attach", "Sürece Bağlan", [this]() { runCommand("debug.attach"); });
    mkAct("debug.core", "Core ile Aç", [this]() { runCommand("debug.core"); });
    mkAct("debug.substitutePath", "Kaynak Yolu Eşle", [this]() { runCommand("debug.substitutePath"); });
    mkAct("debug.editBp", "Kesme Düzenle", [this]() { runCommand("debug.editBp"); });
    mkAct("debug.toggleSkipped", "Lib Frame Gizle/Göster", [this]() { runCommand("debug.toggleSkipped"); });
    mkAct("debug.setVariable", "Değeri Değiştir", [this]() { runCommand("debug.setVariable"); });
    mkAct("test.discover", "Testleri Keşfet", [this]() { discoverTests(); });
    mkAct("test.runAll", "Tüm Testleri Çalıştır", [this]() { runAllTests(); });
    mkAct("test.coverage", "Kapsama Çalıştır", [this]() { runCoverage(); });
    mkAct("debug.launch", "launch.json Düzenle", [this]() { editLaunchConfig(); });
    // Stage 15: AI hattı
    mkAct("ai.ghost", "Hayalet Tamamlama (aç/kapa)", [this]() { toggleGhost(); });
    mkAct("ai.inlineEdit", "AI ile Yeniden Yaz", [this]() { inlineEditSelection(); });
    mkAct("ai.fixSel", "AI ile Düzelt", [this]() { fixSelectionAi(); });
    mkAct("ai.document", "Fonksiyona Belge Yorumu", [this]() { documentFunction(); });
    mkAct("ai.genTest", "Test Üret", [this]() { genTestForCurrent(); });
    mkAct("ai.commit", "AI Commit Mesajı", [this]() { commitMessageAi(); });
    mkAct("ai.explain", "Sembolü Açıkla", [this]() { explainSymbol(); });
    mkAct("ai.prompts", "İstem Kitaplığı", [this]() { promptLibrary(); });
    // Stage 34: ajan paneli + otonom denetim
    mkAct("ai.agentPanel", "Ajan Paneli", [this]() { runCommand("ai.agentPanel"); });
    mkAct("ai.autonomousAudit", "Projeyi Otonom Denetle", [this]() { runCommand("ai.autonomousAudit"); });
    mkAct("ai.applyLast", "Son AI Kodunu Uygula", [this]() { applyLastAi(); });
    mkAct("ai.pullModel", "Ollama Modeli İndir", [this]() { pullModel(); });
    mkAct("ai.startServer", "Ollama Sunucusunu Başlat", [this]() { runCommand("ai.startServer"); });
    // Stage 16: uzaktan geliştirme
    mkAct("remote.connect", "Uzağa Bağlan", [this]() { remoteConnectDialog(); });
    mkAct("remote.disconnect", "Uzak Bağlantıyı Kes", [this]() { remoteDisconnect(); });
    mkAct("remote.explorer", "Uzak Gezgin", [this]() { showSidePanel(5); });
    mkAct("remote.open", "Uzak Dosya Aç", [this]() { remoteOpenDialog(); });
    mkAct("remote.terminal", "Uzak Terminal", [this]() { remoteTerminalShow(); });
    mkAct("remote.lsp", "Uzak LSP Başlat", [this]() { remoteLspStart(); });
    mkAct("remote.build", "Uzakta Derle", [this]() { remoteBuild(); });
    mkAct("remote.git", "Uzak Git Durumu", [this]() { remoteGitStatus(); });
    mkAct("remote.debug", "Uzak Hedefte Hata Ayıkla", [this]() { remoteDebugStart(); });
    mkAct("remote.forward", "Port Yönlendirme", [this]() { remoteForwardShow(); });
    // Stage 17: editör deneyimi
    mkAct("snippet.insert", "Snippet Ekle", [this]() { snippetPalette(false); });
    mkAct("snippet.new", "Yeni Snippet", [this]() { snippetPalette(true); });
    mkAct("edit.pasteAs", "Özel Yapıştır", [this]() { pasteAs(); });
    mkAct("edit.wordComplete", "Kelime Tamamla", [this]() { wordComplete(); });
    mkAct("outline.show", "Outline Paneli", [this]() { outlineShow(); });
    mkAct("fold.refreshLsp", "Katlamayı Tazele (LSP)", [this]() { foldingRefreshLsp(); });
    mkAct("history.timeline", "Zaman Çizelgesi", [this]() { timelineShow(); });
    mkAct("history.restorePrev", "Önceki Sürüme Dön", [this]() { timelineRestorePrev(); });
    mkAct("search.exclude", "Hariç Tut", [this]() { searchExcludeFocus(); });
    mkAct("git.hunk", "Hunk Menüsü", [this]() { hunkMenuAtCursor(); });

    // --- Menü ---
    auto* fileMenu = menuBar()->addMenu("Dosya");
    // Stage 37: AI isteğini iptal
    {
        QAction* a = new QAction("AI İsteğini İptal Et", this);
        a->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Period));
        connect(a, &QAction::triggered, this, &MainWindow::cancelAi);
        addAction(a);
        fileMenu->addSeparator();
        fileMenu->addAction(a);
    }
    fileMenu->addAction(m_actions["file.new"]);
    fileMenu->addAction(m_actions["file.openFolder"]);
    fileMenu->addAction(m_actions["file.save"]);
    fileMenu->addAction(m_actions["file.saveAll"]);
    fileMenu->addAction(m_actions["nav.quickOpen"]);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actions["file.pluginsDir"]);
    fileMenu->addAction(m_actions["file.factoryReset"]);

    auto* helpMenu = menuBar()->addMenu("Yardım");
    helpMenu->addAction(m_actions["help.shortcuts"]);
    helpMenu->addAction(m_actions["help.about"]);

    auto* viewMenu = menuBar()->addMenu("Görünüm");
    m_actMinimap = viewMenu->addAction("Minimap");
    m_actMinimap->setCheckable(true);
    m_actMinimap->setChecked(true);
    auto* actCrumb = viewMenu->addAction("Breadcrumb");
    m_actCrumb = actCrumb;
    actCrumb->setCheckable(true);
    actCrumb->setChecked(true);
    m_actTerminal = viewMenu->addAction("Terminal");
    m_actTerminal->setCheckable(true);
    m_actTerminal->setChecked(true);
    viewMenu->addAction(m_actions["nav.palette"]);

    auto* goMenu = menuBar()->addMenu("Kod");
    goMenu->addAction(m_actions["lsp.definition"]);
    goMenu->addAction(m_actions["lsp.hover"]);
    goMenu->addAction(m_actions["run.build"]);
    // Stage 13: dil zekâsı menüsü
    auto* codeMenu = menuBar()->addMenu("Dil");
    codeMenu->addAction(m_actions["edit.complete"]);
    codeMenu->addAction(m_actions["lsp.references"]);
    codeMenu->addAction(m_actions["lsp.rename"]);
    codeMenu->addAction(m_actions["lsp.codeAction"]);
    codeMenu->addAction(m_actions["lsp.signature"]);
    codeMenu->addAction(m_actions["lsp.docSymbols"]);
    codeMenu->addAction(m_actions["lsp.wsSymbols"]);
    codeMenu->addAction(m_actions["format.document"]);
    codeMenu->addAction(m_actions["lsp.calls"]);
    // Stage 17: snippet + outline menüsü
    auto* snipMenu = menuBar()->addMenu("Parça");
    snipMenu->addAction(m_actions["snippet.insert"]);
    snipMenu->addAction(m_actions["snippet.new"]);
    snipMenu->addSeparator();
    snipMenu->addAction(m_actions["edit.pasteAs"]);
    snipMenu->addAction(m_actions["edit.wordComplete"]);
    snipMenu->addSeparator();
    snipMenu->addAction(m_actions["outline.show"]);
    snipMenu->addAction(m_actions["fold.refreshLsp"]);
    snipMenu->addSeparator();
    snipMenu->addAction(m_actions["history.timeline"]);
    snipMenu->addAction(m_actions["history.restorePrev"]);
    snipMenu->addSeparator();
    snipMenu->addAction(m_actions["search.exclude"]);
    snipMenu->addAction(m_actions["git.hunk"]);
    // Stage 14: hata ayıklama & test menüsü
    auto* dbgMenu = menuBar()->addMenu("Hata Ayıkla");
    dbgMenu->addAction(m_actions["debug.start"]);
    dbgMenu->addAction(m_actions["debug.stop"]);
    dbgMenu->addAction(m_actions["debug.continue"]);
    dbgMenu->addAction(m_actions["debug.stepOver"]);
    dbgMenu->addAction(m_actions["debug.stepInto"]);
    dbgMenu->addAction(m_actions["debug.stepOut"]);
    dbgMenu->addAction(m_actions["debug.toggleBp"]);
    dbgMenu->addSeparator();
    dbgMenu->addAction(m_actions["test.discover"]);
    dbgMenu->addAction(m_actions["test.runAll"]);
    dbgMenu->addAction(m_actions["test.coverage"]);
    dbgMenu->addAction(m_actions["debug.launch"]);
    // Stage 15: AI menüsü
    auto* aiMenu = menuBar()->addMenu("YZ");
    aiMenu->addAction(m_actions["ai.ghost"]);
    aiMenu->addAction(m_actions["ai.inlineEdit"]);
    aiMenu->addAction(m_actions["ai.fixSel"]);
    aiMenu->addAction(m_actions["ai.document"]);
    aiMenu->addAction(m_actions["ai.genTest"]);
    aiMenu->addAction(m_actions["ai.commit"]);
    aiMenu->addAction(m_actions["ai.explain"]);
    aiMenu->addAction(m_actions["ai.prompts"]);
    aiMenu->addAction(m_actions["ai.agentPanel"]);
    aiMenu->addAction(m_actions["ai.autonomousAudit"]);
    aiMenu->addAction(m_actions["ai.applyLast"]);
    aiMenu->addAction(m_actions["ai.pullModel"]);
    // Stage 16: uzak menüsü
    auto* remoteMenu = menuBar()->addMenu("Uzak");
    remoteMenu->addAction(m_actions["remote.connect"]);
    remoteMenu->addAction(m_actions["remote.disconnect"]);
    remoteMenu->addSeparator();
    remoteMenu->addAction(m_actions["remote.explorer"]);
    remoteMenu->addAction(m_actions["remote.open"]);
    remoteMenu->addAction(m_actions["remote.terminal"]);
    remoteMenu->addSeparator();
    remoteMenu->addAction(m_actions["remote.lsp"]);
    remoteMenu->addAction(m_actions["remote.build"]);
    remoteMenu->addAction(m_actions["remote.git"]);
    remoteMenu->addAction(m_actions["remote.debug"]);
    remoteMenu->addAction(m_actions["remote.forward"]);

    // --- ActivityBar: Gezgin / Ara / Git / AI / Sorunlar + bağımsız ⚙ ---
    m_activity = new QWidget(this);
    auto* actLay = new QVBoxLayout(m_activity);
    actLay->setContentsMargins(4, 4, 4, 4);
    actLay->setSpacing(4);
    // Stage 9: SVG ikonlu aktivite çubuğu (renkler temadan)
    const ThemeTokens tk0 = ThemeManager::instance().tokens();
    auto mkSideBtn = [this, &tk0](const QString& iconName) {
        auto* b = new QPushButton(m_activity);
        b->setFixedSize(40, 40);
        b->setCheckable(true);
        b->setIconSize(QSize(20, 20));
        b->setIcon(IconTheme::icon(iconName, tk0.textDim, 20));
        return b;
    };
    m_btnExplorer = mkSideBtn("explorer");
    m_btnSearch = mkSideBtn("search");
    m_btnGit = mkSideBtn("git");
    m_btnAi = mkSideBtn("ai");
    m_btnProblems = mkSideBtn("problems");
    m_btnExplorer->setToolTip("Dosya Gezgini");
    m_btnSearch->setToolTip("Projede Ara");
    m_btnGit->setToolTip("Git");
    m_btnAi->setToolTip("AI");
    m_btnProblems->setToolTip("Sorunlar (LSP)");
    // Stage 16: uzak gezgin düğmesi (tema ikonu yoksa metin yedeği)
    m_btnRemote = mkSideBtn("remote");
    m_btnRemote->setToolTip("Uzak Gezgin (SSH)");
    if (m_btnRemote->icon().isNull()) {
        m_btnRemote->setText("⇄");
        m_btnRemote->setIcon(QIcon());
    }
    m_btnExplorer->setChecked(true);
    m_btnSettings = mkSideBtn("settings");
    m_btnSettings->setCheckable(false);
    m_btnSettings->setToolTip("Ayarlar");
    actLay->addWidget(m_btnExplorer);
    actLay->addWidget(m_btnSearch);
    actLay->addWidget(m_btnGit);
    actLay->addWidget(m_btnAi);
    actLay->addWidget(m_btnProblems);
    actLay->addWidget(m_btnRemote);
    // Stage 17: outline düğmesi
    m_btnOutline = mkSideBtn("outline");
    m_btnOutline->setToolTip("Outline (semboller)");
    if (m_btnOutline->icon().isNull()) {
        m_btnOutline->setText("Ω");
        m_btnOutline->setIcon(QIcon());
    }
    actLay->addWidget(m_btnOutline);
    actLay->addStretch(1);
    actLay->addWidget(m_btnSettings);

    // --- Yan panel: 0=Gezgin 1=Ara 2=Git 3=AI 4=Sorunlar 5=Uzak ---
    m_sideWrap = new QWidget(this);
    auto* sideLay = new QVBoxLayout(m_sideWrap);
    sideLay->setContentsMargins(0, 0, 0, 0);
    m_sideTitle = new QLabel("GEZGİN", m_sideWrap);
    m_sideTitle->setStyleSheet("font-weight:bold;padding:8px;font-size:11px;");
    m_side = new QStackedWidget(m_sideWrap);
    m_explorer = new ExplorerPanel(m_side);
    m_search = new SearchPanel(m_side);
    m_git = new GitPanel(m_side);
    m_ai = new AiPanel(m_side);
    // Stage 25: AI çevrimdışı rozeti
    connect(m_ai, &AiPanel::aiModelsChanged, this, [this](bool on) {
        if (m_chipAi) m_chipAi->setText(on ? "AI ✓" : "AI ✗");
    });
    m_problems = new ProblemsPanel(m_side);
    // Stage 16: uzak gezgin (SSH)
    m_ssh = new SshSession(this);
    m_fwd = new PortForwarder(this);
    m_remoteExplorer = new RemoteExplorer(m_side);
    // Alt panel bileşenleri (dock sekmesi oluşturulmadan önce: bağlantılar için)
    m_todo = new TodoPanel(this);
    m_history = new HistoryTab(this);
    m_stashTab = new StashTab(this);
    m_remoteTab = new RemoteTab(this);
    m_side->addWidget(m_explorer);
    m_side->addWidget(m_search);
    m_side->addWidget(m_git);
    m_side->addWidget(m_ai);
    m_side->addWidget(m_problems);
    m_side->addWidget(m_remoteExplorer); // Stage 16: dizin 5 = Uzak
    m_outline = new OutlinePanel(m_side); // Stage 17: dizin 6 = Outline
    m_side->addWidget(m_outline);
    connect(m_outline, &OutlinePanel::symbolClicked, this, [this](int line0) {
        if (auto* ce = currentEditor()) {
            ce->gotoLine(line0 + 1);
            ce->flashLine(line0);
            ce->setFocus();
        }
    });
    connect(m_outline, &OutlinePanel::refreshRequested, this,
            &MainWindow::refreshOutline);
    sideLay->addWidget(m_sideTitle);
    sideLay->addWidget(m_side, 1);
    m_sideWrap->setMinimumWidth(280);
    m_sideWrap->setMaximumWidth(340);

    auto switchTo = [this](int i) { showSidePanel(i); };
    connect(m_btnExplorer, &QPushButton::clicked, this, [switchTo]() { switchTo(0); });
    connect(m_btnSearch, &QPushButton::clicked, this, [switchTo]() { switchTo(1); });
    connect(m_btnGit, &QPushButton::clicked, this, [switchTo]() { switchTo(2); });
    connect(m_btnAi, &QPushButton::clicked, this, [switchTo]() { switchTo(3); });
    connect(m_btnProblems, &QPushButton::clicked, this, [switchTo]() { switchTo(4); });
    connect(m_btnRemote, &QPushButton::clicked, this, [switchTo]() { switchTo(5); });
    connect(m_btnOutline, &QPushButton::clicked, this,
            [switchTo]() { switchTo(6); }); // Stage 17
    connect(m_btnSettings, &QPushButton::clicked, this, &MainWindow::openSettings);
    // --- Editör alanı: breadcrumb + (grup1 + grup2 + minimap) ---
    m_crumb = new BreadcrumbBar(this);
    m_tabs = makeTabWidget(0);
    m_tabs2 = makeTabWidget(1);
    m_tabs2->setVisible(false);
    m_minimap = new Minimap(this);
    connect(m_actMinimap, &QAction::toggled, m_minimap, &QWidget::setVisible);
    connect(actCrumb, &QAction::toggled, m_crumb, &QWidget::setVisible);
    connect(m_crumb, &BreadcrumbBar::crumbClicked, this, [this](const QString& path) {
        if (QFileInfo(path).isFile()) openFile(path);
    });
    connect(m_crumb, &BreadcrumbBar::symbolActivated, this, [this](int line) {
        if (auto* e = currentEditor()) e->gotoLine(line);
    });

    connect(m_tabs, &QTabWidget::tabCloseRequested, this, [this](int i) {
        closeTabIn(m_tabs, i);
    });
    connect(m_tabs2, &QTabWidget::tabCloseRequested, this, [this](int i) {
        closeTabIn(m_tabs2, i);
    });
    connect(m_tabs, &QTabWidget::currentChanged, this, [this]() { onGroupCurrentChanged(0); });
    connect(m_tabs2, &QTabWidget::currentChanged, this, [this]() { onGroupCurrentChanged(1); });

    connect(m_explorer, &ExplorerPanel::fileOpened, this, &MainWindow::openFile);
    connect(m_explorer, &ExplorerPanel::filePreviewRequested, this,
            &MainWindow::openPreview); // Stage 24
    connect(m_explorer, &ExplorerPanel::compareRequested, this, &MainWindow::diffPath);
    connect(m_explorer, &ExplorerPanel::pathRenamed, this, &MainWindow::onPathRenamed);
    connect(m_search, &SearchPanel::fileOpened, this, &MainWindow::openFileAt);
    connect(m_git, &GitPanel::fileOpenRequested, this, &MainWindow::openFile);
    connect(m_problems, &ProblemsPanel::fileOpened, this, &MainWindow::openFileAt);
    connect(m_todo, &TodoPanel::fileOpened, this, &MainWindow::openFileAt);
    connect(m_history, &HistoryTab::fileOpenRequested, this, &MainWindow::openFile);
    connect(m_ai, &AiPanel::currentFileRequested, this, [this](QString& path, QString& content) {
        if (auto* e = currentEditor()) { path = e->filePath(); content = e->toPlainText(); }
    });
    connect(m_ai, &AiPanel::selectionRequested, this, [this](QString& sel, int& st, int& ln) {
        if (auto* e = currentEditor()) {
            QTextCursor c = e->textCursor();
            st = c.selectionStart();
            ln = c.selectionEnd() - st;
            sel = c.selectedText().replace(QChar(0x2029), "\n");
        }
    });
    connect(m_ai, &AiPanel::projectRootRequested, this, [this](QString& root) { root = m_root; });
    connect(m_ai, &AiPanel::stagedDiffRequested, this, [this](QString& diff) {
        QProcess p(this);
        p.setWorkingDirectory(m_root);
        p.start("git", {"diff", "--cached", "--no-color"});
        p.waitForFinished(8000);
        diff = QString::fromUtf8(p.readAllStandardOutput()).left(8000);
    });
    connect(m_ai, &AiPanel::applyToEditorRequested, this,
            [this](const QString& t, bool whole, int st, int ln) { applyAiEdit(t, whole, st, ln); });
    connect(m_ai, &AiPanel::problemsTextRequested, this, [this](QString& text) {
        QStringList out;
        auto add = [&](const QMap<QString, QList<LspDiag>>& m, const QString& tag) {
            for (auto it = m.begin(); it != m.end(); ++it)
                for (const auto& d : it.value())
                    out << QString("%1:%2:%3 [%4] %5").arg(it.key()).arg(d.line + 1)
                               .arg(d.col + 1).arg(tag).arg(d.message);
        };
        add(m_diags, "LSP");
        add(m_spellDiags, "yazım");
        text = out.join('\n').left(6000);
    });
    connect(m_ai, &AiPanel::applyFileContentRequested, this,
            [this](const QString& path, const QString& content) {
        QFile f(path); // Stage 28: önce eskiyi yedekle (geri alma)
        QString old;
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            old = QString::fromUtf8(f.readAll());
            f.close();
        }
        pushAiUndo(path, old, nullptr);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            m_status->setText("Ajan yazımı başarısız: " + path);
            return;
        }
        f.write(content.toUtf8());
        f.close();
        for (CodeEditor* e : allEditors())
            if (QFileInfo(e->filePath()) == QFileInfo(path)) {
                QSignalBlocker b(e);
                e->setPlainText(content);
                e->document()->setModified(false);
            }
        m_status->setText("Ajan düzenlemesi yazıldı: " + QFileInfo(path).fileName());
    });
    connect(m_problems, &ProblemsPanel::fixWithAiRequested, this,
            [this](const QString& path, int line, const QString& message) {
        QString code;
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QStringList lines = QString::fromUtf8(f.readAll()).split('\n');
            const int a = qMax(0, line - 4);
            const int b = qMin(lines.size(), line + 6);
            for (int i = a; i < b; ++i) code += QString("%1: %2\n").arg(i + 1).arg(lines[i]);
        }
        m_side->setCurrentIndex(3);
        m_btnAi->setChecked(true);
        m_sideTitle->setText(sideTitleFor(3));
        m_ai->fixProblem(path, line, message, code);
    });

    auto* left = new QWidget(this);
    auto* leftLay = new QHBoxLayout(left);
    leftLay->setContentsMargins(0, 0, 0, 0);
    leftLay->addWidget(m_activity);
    leftLay->addWidget(m_sideWrap, 1);

    auto* editorRow = new QWidget(this);
    auto* editorRowLay = new QHBoxLayout(editorRow);
    editorRowLay->setContentsMargins(0, 0, 0, 0);
    editorRowLay->addWidget(m_tabs, 1);
    editorRowLay->addWidget(m_tabs2, 1);
    editorRowLay->addWidget(m_minimap);

    auto* editorCol = new QWidget(this);
    auto* editorColLay = new QVBoxLayout(editorCol);
    editorColLay->setContentsMargins(0, 0, 0, 0);
    editorColLay->addWidget(m_crumb);
    // Stage 11: editör-içi bulma çubuğu (varsayılan gizli)
    m_findBar = new EditorFindBar(this);
    m_findBar->hide();
    editorColLay->addWidget(m_findBar);
    editorColLay->addWidget(editorRow, 1);
    connect(m_findBar, &EditorFindBar::queryChanged, this, &MainWindow::applyFindToEditor);
    connect(m_findBar, &EditorFindBar::navigate, this, &MainWindow::findNavigate);
    connect(m_findBar, &EditorFindBar::closed, this, [this]() {
        m_findBar->hide();
        if (auto* e = currentEditor()) {
            e->clearFindHits();
            e->setFocus();
        }
        m_minimap->setSearchMarks({}, -1);
    });

    auto* split = new QSplitter(Qt::Horizontal, this);
    split->addWidget(left);
    split->addWidget(editorCol);
    split->setStretchFactor(1, 1);
    split->setSizes({320, 960});
    // Stage 12: özel başlık çubuğu en üstte (varsayılan gizli)
    auto* central = new QWidget(this);
    auto* centralLay = new QVBoxLayout(central);
    centralLay->setContentsMargins(0, 0, 0, 0);
    centralLay->setSpacing(0);
    m_titleBar = new TitleBar(this, central);
    m_titleBar->hide();
    centralLay->addWidget(m_titleBar);
    centralLay->addWidget(split, 1);
    setCentralWidget(central);
    connect(m_titleBar, &TitleBar::minimizeRequested, this, &QWidget::showMinimized);
    connect(m_titleBar, &TitleBar::maximizeRequested, this, [this]() {
        isMaximized() ? showNormal() : showMaximized();
    });
    connect(m_titleBar, &TitleBar::closeRequested, this, &QWidget::close);

    // --- Alt dock: Terminal / Görevler / Geçmiş / Stash / Uzak ---
    m_terminal = new TerminalPanel(this);
    m_terminal->setWorkdir(m_root);
    m_terminal->setShell(s.terminalShell);
    if (m_minimap) m_minimap->setFixedWidth(qBound(40, s.minimapWidth, 220)); // Stage 23
    // Stage 24: sekme grubu renkleri — 2. grup yeşilimsi seçili çizgisi
    if (m_tabs2)
        m_tabs2->tabBar()->setStyleSheet(
            "QTabBar::tab:selected { border-bottom: 2px solid #4ec9b0; }");
    connect(m_terminal, &TerminalPanel::buildRequested, this, [this]() { runCommand("run.build"); });
    m_bottomTabs = new QTabWidget(this);
    m_bottomTabs->setDocumentMode(true);
    m_taskPanel = new TaskPanel(this);
    m_bottomTabs->addTab(m_terminal, tr("Terminal"));
    m_bottomTabs->addTab(m_taskPanel, tr("Tasks"));
    m_bottomTabs->addTab(m_todo, tr("Görevler"));
    m_bottomTabs->addTab(m_history, tr("Geçmiş"));
    m_bottomTabs->addTab(m_stashTab, tr("Stash"));
    m_bottomTabs->addTab(m_remoteTab, tr("Uzak"));
    m_debug = new DebugPanel(this);
    m_tests = new TestExplorer(this);
    m_bottomTabs->addTab(m_debug, tr("Hata Ayıklama"));
    m_bottomTabs->addTab(m_tests, tr("Testler"));
    // Stage 16: uzak terminal + port yönlendirme
    m_remoteTerm = new RemoteTerminal(this);
    m_fwdPanel = new PortForwardPanel(this);
    m_fwdPanel->setForwarder(m_fwd);
    m_bottomTabs->addTab(m_remoteTerm, tr("Uzak Kabuk"));
    m_bottomTabs->addTab(m_fwdPanel, tr("Tüneller"));
    // Stage 17: zaman çizelgesi sekmesi
    m_timeline = new TimelinePanel(m_backups ? m_backups->dir() : QString(), this);
    m_bottomTabs->addTab(m_timeline, tr("Zaman Çizelgesi"));
    connect(m_timeline, &TimelinePanel::restoreRequested, this,
            [this](const QString& file, const QString& content) {
                if (auto* e = openEditorFor(file)) {
                    const int pos = e->textCursor().position();
                    e->setPlainText(content);
                    QTextCursor c(e->document());
                    c.setPosition(qMin(pos, e->document()->characterCount() - 1));
                    e->setTextCursor(c);
                    e->document()->setModified(true);
                    pushDocToLsp(e);
                    m_status->setText("Sürüm geri yüklendi (kaydetmeyi unutmayın).");
                }
            });
    connect(m_timeline, &TimelinePanel::statusMessage, this,
            [this](const QString& m) { m_status->setText(m); });
    connect(m_remoteExplorer, &RemoteExplorer::fileOpenRequested, this,
            &MainWindow::remoteOpenFile);
    connect(m_remoteExplorer, &RemoteExplorer::statusMessage, this,
            [this](const QString& m) { m_status->setText(m); });
    connect(m_remoteTerm, &RemoteTerminal::statusMessage, this,
            [this](const QString& m) { m_status->setText(m); });
    connect(m_fwdPanel, &PortForwardPanel::statusMessage, this,
            [this](const QString& m) { m_status->setText(m); });
    connect(m_debug, &DebugPanel::startRequested, this, &MainWindow::debugStart);
    connect(m_debug, &DebugPanel::stopRequested, this, &MainWindow::debugStop);
    connect(m_debug, &DebugPanel::continueRequested, this, &MainWindow::debugContinue);
    connect(m_debug, &DebugPanel::nextRequested, this, &MainWindow::debugNext);
    connect(m_debug, &DebugPanel::stepRequested, this, &MainWindow::debugStep);
    connect(m_debug, &DebugPanel::finishRequested, this, &MainWindow::debugFinish);
    connect(m_debug, &DebugPanel::breakpointToggled, this, &MainWindow::toggleBreakpoint);
    connect(m_debug, &DebugPanel::evaluateRequested, this, &MainWindow::debugEvaluate);
    // Stage 26: izleme + bellek + thread + değişken düzenleme
    connect(m_debug, &DebugPanel::watchAddRequested, this, &MainWindow::debugAddWatch);
    connect(m_debug, &DebugPanel::watchRemoveRequested, this, &MainWindow::debugRemoveWatch);
    connect(m_debug, &DebugPanel::memoryReadRequested, this, &MainWindow::debugReadMemory);
    connect(m_debug, &DebugPanel::threadSelected, this, &MainWindow::debugSelectThread);
    connect(m_debug, &DebugPanel::variableEditRequested, this, &MainWindow::debugSetVariable);
    connect(m_debug, &DebugPanel::consoleRequested, this, &MainWindow::debugConsole);
    connect(m_debug, &DebugPanel::frameSelected, this, [this](int f) { refreshDebugVars(f); });
    connect(m_tests, &TestExplorer::discoverRequested, this, &MainWindow::discoverTests);
    connect(m_tests, &TestExplorer::runAllRequested, this, &MainWindow::runAllTests);
    connect(m_tests, &TestExplorer::runOneRequested, this, &MainWindow::runOneTest);
    connect(m_tests, &TestExplorer::stopRequested, this, &MainWindow::stopTests);
    // Stage 14: GDB sürücüsü
    m_gdb = new GdbDriver(this);
    connect(m_gdb, &GdbDriver::stopped, this, &MainWindow::onDebugStopped);
    connect(m_gdb, &GdbDriver::exited, this, &MainWindow::onDebugExited);
    connect(m_gdb, &GdbDriver::output, this, [this](const QString& t) {
        m_debug->appendOutput(t);
    });
    connect(m_gdb, &GdbDriver::consoleMsg, this, [this](const QString& t) {
        m_debug->appendOutput(t);
    });
    connect(m_gdb, &GdbDriver::driverError, this, [this](const QString& e) {
        m_status->setText(e);
        m_debug->appendOutput(e + "\n", true);
    });
    m_bottomTabs->setMovable(true); // Stage 12: sürükle-bırak sekme sırası
    connect(m_bottomTabs->tabBar(), &QTabBar::tabMoved, this, &MainWindow::saveBottomOrder);
    m_termDock = new QDockWidget(tr("Alt Panel"), this);
    m_termDock->setWidget(m_bottomTabs);
    addDockWidget(Qt::BottomDockWidgetArea, m_termDock);
    connect(m_termDock, &QDockWidget::visibilityChanged, m_actTerminal, &QAction::setChecked);
    connect(m_actTerminal, &QAction::toggled, m_termDock, &QWidget::setVisible);

    m_status = new QLabel(this);
    // Stage 9: tıklanabilir durum çipleri
    m_chipGit = makeChip("Git dalı — Git panelini açar", "view.git");
    m_chipProblems = makeChip("Sorunlar — paneli açar", "view.problems");
    m_cursorLabel = makeChip("Satır:sütun — satıra git", "nav.gotoLine");
    m_chipLang = makeChip("Dil — değiştirmek için tıkla", "file.language");
    m_chipGrid = makeChip("Izgara: harf hizalaması durumu", QString()); // Stage 23
    m_chipEol = makeChip("Satır sonu — değiştirmek için tıkla", "edit.toggleEol");
    m_chipEnc = makeChip("Kodlama bilgisi — tıklayın", "file.encodingInfo");
    m_chipProfile = makeChip("Görünüm profili — değiştirmek için tıkla", "ui.profiles"); // Stage 12
    m_chipAi = makeChip("AI durumu", QString()); // Stage 25: çevrimdışı rozeti
    m_chipAi->setText("AI ?");
    // Stage 37: etkin sağlayıcı + sağlık noktası (tıklayınca hızlı değiştir)
    m_chipProvider = makeChip("Etkin AI sağlayıcısı (tıklayınca değiştir)", QString());
    m_chipProvider->setText("AI: —");
    m_chipRag = makeChip("RAG indeksleme durumu", QString()); // Stage 40
    m_chipRag->setVisible(false);
    m_chipProvider->setProperty("cmd", QStringLiteral("ai.provider"));
    statusBar()->addWidget(m_status, 1);
    statusBar()->addPermanentWidget(m_chipGit);
    statusBar()->addPermanentWidget(m_chipProblems);
    statusBar()->addPermanentWidget(m_cursorLabel);
    statusBar()->addPermanentWidget(m_chipLang);
    statusBar()->addPermanentWidget(m_chipGrid); // Stage 23
    statusBar()->addPermanentWidget(m_chipEol);
    statusBar()->addPermanentWidget(m_chipEnc);
    statusBar()->addPermanentWidget(m_chipProfile);
    statusBar()->addPermanentWidget(m_chipAi); // Stage 25
    statusBar()->addPermanentWidget(m_chipProvider); // Stage 37
    statusBar()->addPermanentWidget(m_chipRag); // Stage 40
    m_chipEol->setText("LF");
    m_chipEnc->setText("UTF-8");

    m_explorer->setTrash(m_trash);
    refreshProjectViews();

    applyShortcuts();

    // Stage 8: --new ile oturum geri yükleme atlanır
    const QStringList rawArgs = QCoreApplication::arguments();
    const bool newWindow = rawArgs.contains("--new") || rawArgs.contains("-n");
    if (!newWindow) restoreSession();
    // Stage 10: boş grup → yeniden tasarlanmış karşılama ekranı
    if (m_tabs->count() == 0) {
        ensureWelcome(m_tabs);
    }
    if (auto* e = currentEditor()) {
        m_minimap->setEditor(e);
    }

    // Stage 10: animasyon tercihi + kayıtlı yerleşim ön ayarı
    Animator::setReducedMotion(s.reducedMotion);
    if (LayoutPresets::isValid(s.layoutPreset)) applyLayoutPreset(s.layoutPreset);
    ToastManager::instance().attach(this);
    // Stage 21: pano halkası — kopyalanan metinleri biriktir
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this,
            &MainWindow::onClipboardChanged);
    m_macro.load(); // Stage 25: kayıtlı makro
    checkCrashDumps(); // Stage 31: önceki çökme izleri
    // Stage 21: kurtarma yedekleri varsa bildir
    if (m_backups) {
        const int nb = m_backups->list().size();
        if (nb > 0)
            toast(2, QString("%1 kurtarma yedeği bulundu — Zaman Çizelgesi'nden geri yüklenebilir").arg(nb));
    }

    // Stage 12: özel başlık + panel sırası + otomatik tema
    setCustomTitleBar(s.customTitleBar);
    applyBottomOrder();
    applyChips();
    applySidePages();
    m_autoTimer = new QTimer(this);
    connect(m_autoTimer, &QTimer::timeout, this, &MainWindow::evaluateAutoTheme);
    m_autoTimer->start(60000);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, &MainWindow::evaluateAutoTheme);
#endif
    evaluateAutoTheme();

    m_autosave = new QTimer(this);
    connect(m_autosave, &QTimer::timeout, this, &MainWindow::autoSaveTick);
    m_autosave->start(s.autoSaveIntervalMs);

    // Stage 13: dil zekâsı — tamamlama popup + debounce zamanlayıcıları
    m_completion = new CompletionPopup(this);
    m_completion->hide();
    connect(m_completion, &CompletionPopup::chosen, this, &MainWindow::onCompletionChosen);
    // Stage 27: vurgulanan öğenin dokümantasyonu (completionItem/resolve)
    connect(m_completion, &CompletionPopup::highlighted, this,
            [this](const CompletionItem& item) {
                auto* e = currentEditor();
                if (!e || item.raw.isEmpty()) return;
                QString lang;
                LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
                if (!c || !c->isReady()) return;
                c->request("completionItem/resolve", item.raw, [this](QJsonObject res) {
                    if (!m_completion->isVisible()) return;
                    QJsonObject r = res.contains("result") ? res["result"].toObject() : res;
                    QString doc = r["documentation"].isObject()
                        ? r["documentation"].toObject()["value"].toString()
                        : r["documentation"].toString();
                    doc = doc.trimmed().left(600);
                    if (doc.isEmpty()) return;
                    const QPoint p = m_completion->mapToGlobal(
                        QPoint(m_completion->width(), 0));
                    QToolTip::showText(p, doc, m_completion);
                });
            });
    m_completeTimer = new QTimer(this);
    m_completeTimer->setSingleShot(true);
    m_completeTimer->setInterval(350);
    connect(m_completeTimer, &QTimer::timeout, this, [this]() { requestCompletion(true); });
    m_sigTimer = new QTimer(this);
    m_sigTimer->setSingleShot(true);
    m_sigTimer->setInterval(400);
    connect(m_sigTimer, &QTimer::timeout, this, [this]() { showSignatureHelp(true); });
    m_inlayTimer = new QTimer(this);
    m_inlayTimer->setSingleShot(true);
    m_inlayTimer->setInterval(800);
    // Stage 27: lens + renk kutuları (yavaş devir, 8 sn)
    m_lensTimer = new QTimer(this);
    connect(m_lensTimer, &QTimer::timeout, this, [this]() {
        if (auto* e = currentEditor()) {
            refreshCodeLens(e);
            refreshDocColors(e);
        }
    });
    m_lensTimer->start(8000);
    connect(m_inlayTimer, &QTimer::timeout, this, [this]() {
        if (auto* e = currentEditor()) {
            refreshInlayHints(e);
            refreshSemantic(e);
        }
    });
    // Stage 15: hayalet tamamlama istemcileri + debounce
    m_ghostAi = new OllamaClient(this);
    m_flowAi = new OllamaClient(this);
    // Stage 37: tek AI cephesi + API anahtarı kasası
    m_secrets = new SecretStore();
    m_aiRunner = new AiRunner(this);
    m_aiRunner->setSecretStore(m_secrets);
    connect(m_ghostAi, &OllamaClient::error, this, [this](const QString& e) {
        // Hayalet hataları sessiz (durum çubuğunda kısa bilgi)
        if (m_ghostReq > 0) m_status->setText("Hayalet: " + e.left(80));
    });
    m_ghostTimer = new QTimer(this);
    m_ghostTimer->setSingleShot(true);
    m_ghostTimer->setInterval(800);
    connect(m_ghostTimer, &QTimer::timeout, this, &MainWindow::requestGhost);
    // Stage 17: outline debounce (yazarken bayatlamasın)
    m_outlineTimer = new QTimer(this);
    m_outlineTimer->setSingleShot(true);
    m_outlineTimer->setInterval(1500);
    connect(m_outlineTimer, &QTimer::timeout, this, [this]() {
        if (m_side && m_side->currentIndex() == 6) refreshOutline();
    });

    retranslate();
    updateCursorStatus();
    updateBreadcrumb();
    PerfMonitor::instance().mark("UI hazır");
    // Stage 22: eklentiler ilk kareden sonra (başlangıcı yavaşlatmaz)
    QTimer::singleShot(800, this, &MainWindow::loadPluginsDeferred);
    // Ollama kapalıysa bir kez sessizce başlatmayı dene (sonuç toast).
    // Stage 46: ayarlanabilir (testler ve elle yönetenler kapatabilir).
    QTimer::singleShot(2500, this, [this]() {
        AppSettings s = SettingsManager::instance().load();
        if (!s.ollamaAutoStart) return;
        const QString host = s.ollamaHost;
        QFutureWatcher<bool>* w = new QFutureWatcher<bool>(this);
        connect(w, &QFutureWatcher<bool>::finished, this, [this, w]() {
            const bool ok = w->result();
            w->deleteLater();
            if (ok && m_ai) {
                m_ai->refreshModels();
                toast(1, "Ollama hazır ✓");
            }
        });
        w->setFuture(QtConcurrent::run(
            [host]() { return OllamaClient::ensureServer(host, 12000); }));
    });
    // Stage 48: güncelleme denetimi (haftada 1, ayarlanabilir, sessiz başarısızlık)
    QTimer::singleShot(30000, this, [this]() {
        if (!UpdateChecker::shouldCheck()) return;
        auto *uc = new UpdateChecker(this);
        uc->setCurrentVersion(AboutInfo::version());
        connect(uc, &UpdateChecker::ready, this, [this, uc](const UpdateChecker::Result &r) {
            uc->deleteLater();
            if (!r.checked || !r.newer) return;
            toast(0, QString("Yeni sürüm mevcut: %1 (yardım menüsünden indirin)").arg(r.latest));
            m_status->setText(QString("Güncelleme: %1").arg(r.latest));
        });
        uc->check();
    });
    // Stage 9: tema/accent değişince tüm görselleri tazele
    ThemeManager::instance().onApplied([this]() {
        refreshActivityIcons();
        for (CodeEditor* e : allEditors()) e->refreshTheme();
        updateTabMarks();
    });
    refreshActivityIcons();
    refreshGitBranch();
}

QString MainWindow::sideTitleFor(int i) const {
    auto& L = LanguageManager::instance();
    switch (i) {
    case 0: return L.t("explorer");
    case 1: return L.lang() == "tr" ? "ARAMA" : "SEARCH";
    case 2: return L.t("source_control");
    case 3: return L.t("ai_assistant");
    case 5: return L.lang() == "tr" ? "UZAK (SSH)" : "REMOTE (SSH)";
    case 6: return "OUTLINE";
    default: return L.lang() == "tr" ? "SORUNLAR" : "PROBLEMS";
    }
}

void MainWindow::closeEvent(QCloseEvent* e) {
    saveSession();
    saveAll();
    QMainWindow::closeEvent(e);
}

QTabWidget* MainWindow::makeTabWidget(int group) {
    Q_UNUSED(group);
    auto* t = new QTabWidget(this);
    // Stage 47: ekran okuyucu sekme kümelerini ayırt eder
    t->setAccessibleName(group == 0 ? "Düzenleyici sekmeleri" : "İkinci grup sekmeleri");
    t->setTabsClosable(true);
    t->setMovable(true);
    t->setDocumentMode(true);
    // Stage 9: sekme çubuğu taşma menüsü — sağ tıkla açık sekmeler listesi
    t->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    t->tabBar()->installEventFilter(this); // Stage 21: orta-tık + çift-tık
    connect(t->tabBar(), &QWidget::customContextMenuRequested, this, [this, t](const QPoint& pos) {
        if (t->count() == 0) return;
        QMenu menu;
        for (int i = 0; i < t->count(); ++i) {
            QAction* a = menu.addAction(t->tabText(i));
            connect(a, &QAction::triggered, this, [t, i]() {
                t->setCurrentIndex(i);
                t->widget(i)->setFocus();
            });
        }
        menu.exec(t->tabBar()->mapToGlobal(pos));
    });
    return t;
}

QTabWidget* MainWindow::activeTabs() const {
    return m_activeGroup == 0 ? m_tabs : m_tabs2;
}

// Stage 21: pano değişince halkaya ekle
void MainWindow::onClipboardChanged() {
    const QMimeData* md = QApplication::clipboard()->mimeData();
    if (md && md->hasText()) m_clipRing.push(md->text().left(100000));
}

// Stage 21: seçimdeki metni halkanın sonraki girdisiyle değiştir (döner)
void MainWindow::pasteCyclePrevious() {
    CodeEditor* e = currentEditor();
    if (!e) return;
    if (m_clipRing.size() == 0) { toast(0, "Pano halkası boş — önce bir şey kopyalayın"); return; }
    QTextCursor c = e->textCursor();
    if (!c.hasSelection()) {
        c.select(QTextCursor::WordUnderCursor);
        if (!c.hasSelection()) { toast(0, "Önce değiştirilecek metni seçin"); return; }
        e->setTextCursor(c);
    }
    c = e->textCursor();
    c.beginEditBlock();
    c.removeSelectedText();
    c.insertText(m_clipRing.next());
    c.endEditBlock();
}

// Stage 21: seçili satırların (yoksa tüm belgenin) sonlarına ek imleç
void MainWindow::multiCaretLineEnds() {
    CodeEditor* e = currentEditor();
    if (!e) return;
    QTextCursor cur = e->textCursor();
    QTextDocument* doc = e->document();
    int a, b;
    if (cur.hasSelection()) {
        a = doc->findBlock(cur.selectionStart()).blockNumber();
        b = doc->findBlock(cur.selectionEnd()).blockNumber();
    } else {
        a = 0;
        b = doc->blockCount() - 1;
    }
    int added = 0;
    for (int ln = a; ln <= b && added < 300; ++ln) {
        QTextBlock blk = doc->findBlockByNumber(ln);
        if (!blk.isValid() || blk.text().isEmpty()) continue;
        const int pos = blk.position() + blk.length();
        if (pos == cur.position()) continue;
        e->addCursorAt(pos);
        ++added;
    }
    m_status->setText(added > 0 ? QString("%1 ek imleç").arg(added)
                                : "Ek imleç konulacak satır yok");
}

// Stage 21: satır sonunu çevir (LF ↔ CRLF)
void MainWindow::toggleEol() {
    CodeEditor* e = currentEditor();
    if (!e) return;
    e->setCrlf(e->lineEnding() == "LF");
    updateCursorStatus();
    toast(1, "Satır sonu: " + e->lineEnding());
}

// Stage 21: dosya kodlama bilgisi
void MainWindow::showEncodingInfo() {
    CodeEditor* e = currentEditor();
    if (!e) return;
    QString bom = "yok";
    qint64 size = -1;
    if (!e->filePath().isEmpty()) {
        QFile f(e->filePath());
        size = f.size();
        if (f.open(QIODevice::ReadOnly)) {
            const QByteArray head = f.read(4);
            if (head.startsWith("\xef\xbb\xbf")) bom = "UTF-8 BOM";
            else if (head.startsWith("\xff\xfe") || head.startsWith("\xfe\xff"))
                bom = "UTF-16 BOM";
        }
    }
    QMessageBox::information(this, "Kodlama Bilgisi",
        QString("Dosya: %1\nBoyut: %2\nKodlama: UTF-8 (varsayılan)\nBOM: %3\nSatır sonu: %4")
            .arg(e->filePath().isEmpty() ? "(henüz kaydedilmedi)" : e->filePath())
            .arg(size < 0 ? "?" : QString::number(size))
            .arg(bom, e->lineEnding()));
}

void MainWindow::closeTabIn(QTabWidget* tabs, int i) {
    if (isPinnedTab(tabs, i)) {
        toast(0, "Sabit sekme — kapatmak için çift tıklayıp sabiti kaldırın");
        return;
    }
    if (auto* e = qobject_cast<CodeEditor*>(tabs->widget(i))) {
        // Stage 20: içi boş sekme (Adsız + yazısız) sessiz kapanır
        if (e->document()->isModified() && !e->toPlainText().trimmed().isEmpty()) {
            auto r = QMessageBox::question(this, "Kaydet?",
                "Kaydedilmemiş değişiklik var. Kapatmadan önce kaydedilsin mi?",
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            if (r == QMessageBox::Cancel) return;
            if (r == QMessageBox::Save) {
                tabs->setCurrentIndex(i);
                saveCurrent();
                if (e->document()->isModified()) return;
            }
        }
        teardownLsp(e->filePath());
        m_diags.remove(e->filePath());
        m_spellDiags.remove(e->filePath());
        refreshProblemView();
    }
    QWidget* w = tabs->widget(i);
    // Stage 46: bekleyen sekme kapanırsa kayıtları da düşer (yetim girdi yok)
    if (const QString pp = w->property("pendingPath").toString(); !pp.isEmpty()) {
        m_pendingCursors.remove(pp);
        m_pendingFolds.remove(pp);
    }
    tabs->removeTab(i);
    delete w;
    // Stage 10: son sekme kapandıysa karşılama ekranı göster
    if (tabs->count() == 0) ensureWelcome(tabs);
    saveSession();
}

static QList<LspDiag> mergedDiags(const QMap<QString, QList<LspDiag>>& a,
                                  const QMap<QString, QList<LspDiag>>& b, const QString& path) {
    QList<LspDiag> out = a.value(path) + b.value(path);
    return out;
}
// Stage 14: derleme/test sorunları da birleşir
static QList<LspDiag> mergedDiags3(const QMap<QString, QList<LspDiag>>& a,
                                   const QMap<QString, QList<LspDiag>>& b,
                                   const QMap<QString, QList<LspDiag>>& c,
                                   const QString& path) {
    QList<LspDiag> out = a.value(path) + b.value(path) + c.value(path);
    return out;
}

void MainWindow::refreshProblemView() {
    QMap<QString, QList<LspDiag>> all = m_diags;
    for (auto it = m_spellDiags.begin(); it != m_spellDiags.end(); ++it)
        all[it.key()].append(it.value());
    for (auto it = m_buildDiags.begin(); it != m_buildDiags.end(); ++it)
        all[it.key()].append(it.value());
    m_problems->setDiagnostics(all);
    // Stage 9: sorun sayısı çipi
    int total = 0, errors = 0;
    for (auto it = all.begin(); it != all.end(); ++it) {
        total += it.value().size();
        for (const LspDiag& d : it.value())
            if (d.severity == 1) ++errors;
    }
    if (m_chipProblems) {
        m_chipProblems->setText(total > 0 ? QString("⚠ %1 sorun").arg(total) : "✓ 0 sorun");
        const char* objName = errors > 0 ? "chipErr" : (total > 0 ? "chipWarn" : "chipOk");
        if (m_chipProblems->objectName() != objName) {
            m_chipProblems->setObjectName(objName);
            m_chipProblems->style()->unpolish(m_chipProblems);
            m_chipProblems->style()->polish(m_chipProblems);
        }
    }
}

void MainWindow::onGroupCurrentChanged(int group) {
    m_activeGroup = group;
    // Stage 46: tembel sekme — bekleyen yer tutucu ilk aktivasyonda gerçeklenir
    if (QWidget* w = activeTabs()->currentWidget()) materializePending(w);
    updateCursorStatus();
    updateBreadcrumb();
    if (auto* e = qobject_cast<CodeEditor*>(activeTabs()->currentWidget())) {
        m_minimap->setEditor(e);
        e->setDiagnostics(mergedDiags3(m_diags, m_spellDiags, m_buildDiags, e->filePath()));
        refreshGitMarks(e); // Stage 11
        refreshDiagMinimap(); // Stage 17: tanı çizgileri
        if (m_side && m_side->currentIndex() == 6) refreshOutline(); // Stage 17
        if (m_findBar && m_findBar->isVisible()) applyFindToEditor();
        else m_minimap->setSearchMarks({}, -1);
    }
    saveSession();
}

// Stage 17: geçerli editörün tanılarını minimap sağ şeridine yansıt
void MainWindow::refreshDiagMinimap() {
    auto* e = currentEditor();
    if (!e || !m_minimap) return;
    QMap<int, int> marks; // 1-based satır → en kötü severity
    for (const LspDiag& d :
         mergedDiags3(m_diags, m_spellDiags, m_buildDiags, e->filePath())) {
        const int ln = d.line + 1;
        auto it = marks.find(ln);
        if (it == marks.end() || d.severity < it.value()) marks[ln] = d.severity;
    }
    m_minimap->setDiagMarks(marks);
}

CodeEditor* MainWindow::currentEditor() const {
    return qobject_cast<CodeEditor*>(activeTabs()->currentWidget());
}

CodeEditor* MainWindow::openEditorFor(const QString& path, int group) {
    if (group < 0) group = m_activeGroup;
    // Önce iki grupta da ara
    for (QTabWidget* t : {m_tabs, m_tabs2})
        for (int i = 0; i < t->count(); ++i)
            if (auto* e = qobject_cast<CodeEditor*>(t->widget(i)))
                if (e->filePath() == path) {
                    m_activeGroup = (t == m_tabs) ? 0 : 1;
                    t->setCurrentIndex(i);
                    // Stage 24: önizleme sekmesi tam açılır
                    if (e->previewMode() && e->loadFullPreview()) {
                        t->setTabText(i, QFileInfo(path).fileName());
                        t->setTabToolTip(i, path);
                        setupLspFor(path, e->toPlainText());
                        updateCursorStatus();
                    }
                    return e;
                }
    // Stage 46: aynı yolun bekleyen yer tutucusu varsa onu gerçekle
    for (QTabWidget* t : {m_tabs, m_tabs2})
        for (int i = 0; i < t->count(); ++i)
            if (t->widget(i)->property("pendingPath").toString() == path
                && !path.isEmpty()) {
                materializePending(t->widget(i));
                if (auto* done = qobject_cast<CodeEditor*>(t->widget(i))) return done;
            }
    auto* e = new CodeEditor(this);
    applyEditorSettingsTo(e);
    // Stage 16: ssh:// belgelerde içerik çağrıcı tarafından konur (setContent)
    if (path.startsWith("ssh://")) e->setFilePath(path);
    else if (!e->loadFile(path)) { delete e; return nullptr; }
    return addEditorTab(e, QFileInfo(path).fileName(), path, group);
}

// Stage 23: piksel-hizalı ızgara garantisi — font monospace değilse geri dön
void MainWindow::checkEditorGrid(CodeEditor* e) {
    if (!e) return;
    if (GridCheck::isMonospace(e->font())) {
        m_gridOk = true;
        return;
    }
    m_gridOk = false;
    QFont f(GridCheck::systemMonospace());
    f.setPointSize(e->font().pointSize());
    e->setFont(f);
    e->setTabStopDistance(4 * e->fontMetrics().horizontalAdvance(' '));
    static bool warned = false;
    if (!warned) {
        warned = true;
        toast(2, "Seçili font tek aralıklı değil — ızgara için monospace'e dönüldü");
    }
    m_gridOk = GridCheck::isMonospace(e->font());
}

// Stage 23: yazı tipi seçim diyaloğu (editör ailesi + boyut)
void MainWindow::chooseEditorFont() {
    AppSettings s = SettingsManager::instance().load();
    QFont cur = ThemeManager::instance().typography().editorFamily.isEmpty()
        ? QFont(GridCheck::systemMonospace())
        : QFont(s.editorFontFamily);
    cur.setPointSize(s.fontSize);
    bool ok = false;
    const QFont f = QFontDialog::getFont(&ok, cur, this, "Editör Yazı Tipi");
    if (!ok) return;
    // Izgara şartı: monospace değilse uyar, yine de uygula + denetim yakalar
    if (!GridCheck::isMonospace(f))
        toast(2, "Bu font tek aralıklı değil — ızgara bozulabilir");
    s.editorFontFamily = QFontInfo(f).family();
    s.fontSize = f.pointSize() > 0 ? f.pointSize() : s.fontSize;
    SettingsManager::instance().save(s);
    TypographySettings typo = ThemeManager::instance().typography();
    typo.editorFamily = s.editorFontFamily;
    ThemeManager::instance().setTypography(typo);
    applyEditorSettingsToAll();
    updateCursorStatus();
}

// Ortak sekme kablolama: sekmeye ekle + sinyaller + LSP + durum çubuğu
CodeEditor* MainWindow::addEditorTab(CodeEditor* e, const QString& title,
                                     const QString& tip, int group) {
    if (group < 0) group = m_activeGroup;
    QTabWidget* target = (group == 0) ? m_tabs : m_tabs2;
    if (group == 1) m_tabs2->setVisible(true);
    removeWelcome(target); // Stage 10: dosya açılınca karşılama ekranı kalkar
    connect(e, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateCursorStatus);
    // Stage 49: birlikte çalışma yayını (imleç kısık, metin anında)
    connect(e, &QPlainTextEdit::textChanged, this, &MainWindow::publishCollabEdit);
    connect(e, &QPlainTextEdit::cursorPositionChanged, this, [this]() {
        if (!m_collabCursorTimer) {
            m_collabCursorTimer = new QTimer(this);
            m_collabCursorTimer->setSingleShot(true);
            m_collabCursorTimer->setInterval(500);
            connect(m_collabCursorTimer, &QTimer::timeout, this,
                    &MainWindow::publishCollabCursor);
        }
        if (m_collab && m_collab->isActive() && !m_collabCursorTimer->isActive())
            m_collabCursorTimer->start();
    });
    connect(e, &QPlainTextEdit::textChanged, this, &MainWindow::updateCursorStatus);
    connect(e, &QPlainTextEdit::textChanged, this, &MainWindow::updateBreadcrumb);
    connect(e, &QPlainTextEdit::textChanged, this, [this, e]() { pushDocToLsp(e, false); });
    // Stage 15: hayalet — yazınca eskiyi temizle + yeniden kolla
    connect(e, &QPlainTextEdit::textChanged, this, [this, e]() {
        if (e != currentEditor()) return;
        e->clearGhost();
        AppSettings s = SettingsManager::instance().load();
        if (!s.aiGhost || e->largeFileMode()) return;
        QTextCursor c = e->textCursor();
        const QString line = c.block().text();
        const int i = c.positionInBlock() - 1;
        const QChar before = (i >= 0 && i < line.size()) ? line[i] : QChar();
        const QChar before2 = (i - 1 >= 0) ? line[i - 1] : QChar();
        if (GhostCompletion::shouldTrigger(before, before2) && m_ghostTimer)
            m_ghostTimer->start();
    });
    connect(e, &QPlainTextEdit::cursorPositionChanged, this, [this, e]() {
        // İmleç oynayınca geçersiz hayaleti temizle
        if (e->hasGhost() && e == currentEditor()) {
            QTextCursor c = e->textCursor();
            const QString prefix = e->document()->toPlainText().left(c.position());
            if (!GhostCompletion::stillValid(prefix, e->ghostPrefix()))
                e->clearGhost();
        }
    });
    // Stage 13: yazarken otomatik tamamlama + imza tetikleme (debounce)
    connect(e, &QPlainTextEdit::textChanged, this, [this, e]() {
        if (e != currentEditor()) return;
        AppSettings s = SettingsManager::instance().load();
        if (s.autoComplete && m_completeTimer && !m_completion->isVisible())
            m_completeTimer->start();
        if (m_sigTimer) {
            const QString t = e->textCursor().block().text()
                                  .left(e->textCursor().positionInBlock());
            if (t.contains('(') && !t.contains(')')) m_sigTimer->start();
        }
        if (m_inlayTimer) m_inlayTimer->start();
    });
    connect(e, &CodeEditor::externalChangeDetected, this, &MainWindow::onExternalChange);
    connect(e, &CodeEditor::breakpointToggleRequested, this,
            [this, e](int line1) { toggleBreakpoint(e->filePath(), line1); });
    connect(e, &CodeEditor::colorBoxClicked, this,
            [this, e](int line0) { editColorBox(e, line0); }); // Stage 27
    // Stage 17: outline görünürken yazıldıkça tazele (debounce)
    connect(e, &QPlainTextEdit::textChanged, this, [this, e]() {
        if (e != currentEditor() || !m_outlineTimer) return;
        if (m_side && m_side->currentIndex() == 6) m_outlineTimer->start();
    });
    applyBpMarks(e);
    applyBmMarks(e); // Stage 28
    applyCoverageMarks(e);
    target->addTab(e, title);
    target->setTabToolTip(target->count() - 1, tip);
    m_activeGroup = group;
    target->setCurrentWidget(e);
    m_minimap->setEditor(e);
    e->viewport()->installEventFilter(this); // Stage 26: debug hover
    // Stage 21: büyük dosyada LSP ertelenir (tamamı yüklenince başlar)
    if (!e->filePath().isEmpty() && !e->largeFileMode())
        setupLspFor(e->filePath(), e->toPlainText());
    // Stage 27: lens + renk (sunucu hazırsa)
    QTimer::singleShot(2500, this, [this, e]() {
        if (e->isRemote()) return;
        refreshCodeLens(e);
        refreshDocColors(e);
    });
    refreshGitMarks(e); // Stage 11: gutter + minimap diff işaretleri
    updateCursorStatus();
    updateBreadcrumb();
    saveSession();
    if (m_plugins && !e->filePath().isEmpty())
        m_plugins->fireEvent("open", e->filePath()); // Stage 29
    return e;
}

// İsimsiz yeni dosya: önce dil sorulur, iskelet konur; ilk Kaydet'te
// dile uygun ad + filtre önerilir.
// Stage 30: dosya şablon dizini
QString MainWindow::fileTemplateDir() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/templates";
}

QStringList MainWindow::fileTemplateNames() const {
    QDir d(fileTemplateDir());
    if (!d.exists()) return {};
    QStringList out;
    for (const QFileInfo& fi : d.entryInfoList({"*.tpl"}, QDir::Files, QDir::Name))
        out << fi.completeBaseName();
    return out;
}

void MainWindow::newFileFromTemplate() {
    const QStringList names = fileTemplateNames();
    if (names.isEmpty()) {
        toast(0, "Şablon yok — örnek için bir .tpl kaydedin: " + fileTemplateDir());
        QDir().mkpath(fileTemplateDir());
        return;
    }
    bool ok = false;
    const QString sel = QInputDialog::getItem(this, "Şablondan Yeni Dosya", "Şablon:",
                                              names, 0, false, &ok);
    if (!ok) return;
    QFile f(fileTemplateDir() + "/" + sel + ".tpl");
    if (!f.open(QIODevice::ReadOnly)) return;
    QString body = QString::fromUtf8(f.readAll());
    // {{filename}} {{date}} {{author}} + snippet değişkenleri
    const QString today = QDate::currentDate().toString(Qt::ISODate);
    body.replace("{{date}}", today);
    body.replace("{{filename}}", "adsiz");
    body = SnippetEngine::expandVars(body, "adsiz", QString());
    newUntitledFile();
    if (auto* e = currentEditor()) {
        e->setPlainText(body);
        e->document()->setModified(true);
    }
}

void MainWindow::newUntitledFile() {
    NewFileDialog dlg(false, this);
    if (dlg.exec() != QDialog::Accepted || dlg.selectedLang().isEmpty()) return;
    const QString lang = dlg.selectedLang();
    LanguageSupport::pushRecentLang(lang);
    auto* e = new CodeEditor(this);
    applyEditorSettingsTo(e);
    e->setLang(lang);
    static int untitledNo = 1;
    e->setUntitledNo(untitledNo);
    const QString skel = LanguageSupport::skeleton(lang);
    AppSettings s = SettingsManager::instance().load();
    if (s.newFileTemplate && !skel.isEmpty()) {
        e->setPlainText(skel);
        e->moveCursor(QTextCursor::Start);
        e->document()->setModified(false);
    }
    const QString title = LanguageSupport::untitledTitle(untitledNo++, lang);
    addEditorTab(e, title, title);
    e->setFocus();
}

// Açık sekmenin dilini sonradan değiştir (Adsız sekmelerde başlık da güncellenir)
void MainWindow::changeFileLanguage() {
    CodeEditor* e = currentEditor();
    if (!e) return;
    NewFileDialog dlg(true, this);
    if (dlg.exec() != QDialog::Accepted || dlg.selectedLang().isEmpty()) return;
    const QString lang = dlg.selectedLang();
    LanguageSupport::pushRecentLang(lang);
    e->setLang(lang);
    if (e->isUntitled() && e->untitledNo() > 0) {
        const QString title = LanguageSupport::untitledTitle(e->untitledNo(), lang);
        QTabWidget* t = activeTabs();
        const int idx = t->indexOf(e);
        if (idx >= 0) {
            t->setTabText(idx, title);
            t->setTabToolTip(idx, title);
        }
    }
    updateCursorStatus();
    saveSession();
    if (m_plugins) m_plugins->fireEvent("language", lang); // Stage 29
}

void MainWindow::splitActiveToOther() {    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    int other = m_activeGroup == 0 ? 1 : 0;
    openEditorFor(e->filePath(), other); // aynı dosya diğer grupta (bölünmüş görünüm)
}

void MainWindow::moveActiveTab() {
    QTabWidget* src = activeTabs();
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    QString path = e->filePath();
    int off = e->cursorOffset();
    int idx = src->currentIndex();
    teardownLsp(path);
    src->removeTab(idx);
    e->deleteLater();
    if (auto* n = openEditorFor(path, m_activeGroup == 0 ? 1 : 0))
        n->setCursorOffset(off);
}

// --- LSP ---
LspClient* MainWindow::lspClientFor(const QString& suffix, QString& langId) {
    AppSettings s = SettingsManager::instance().load();
    if (!s.lspEnabled) return nullptr;
    const LspServerDef* def = LspServers::forSuffix(suffix);
    if (!def) return nullptr;
    langId = def->lang;
    LspClient* c = nullptr;
    if (def->lang == "cpp") c = m_lspCpp;
    else if (def->lang == "python") c = m_lspPy;
    else if (def->lang == "rust") c = m_lspRs;
    else if (def->lang == "go") c = m_lspGo;
    else c = m_lspJs;
    if (!c) return nullptr;
    if (!c->isRunning() && !m_lspWarned) {
        // Stage 31: 60 sn içinde 3+ çökme → 5 dk bekleme (fırtına koruması)
        {
            QList<qint64>& hist = m_lspRestarts[c];
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            while (!hist.isEmpty() && now - hist.first() > 60000) hist.removeFirst();
            if (hist.size() >= 3) {
                const qint64 wait = 300000 - (now - hist.first());
                if (wait > 0) {
                    m_status->setText(
                        QString("LSP beklemede (çökme fırtınası) — %1 sn").arg(wait / 1000));
                    return nullptr;
                }
                hist.clear();
            }
        }
        QString prog;
        QStringList args = def->args;
        if (def->lang == "python") {
            // Stage 27: proje venv'i öncelikli
            prog = LspServers::pythonExe(m_root);
            if (!LspServers::pythonVenvs(m_root).isEmpty())
                m_status->setText("venv pylsp: " + prog);
        } else {
            prog = LspServers::findProgram(def->program);
        }
        if (prog.isEmpty()) {
            m_status->setText(def->program + " bulunamadı — " + def->installHint);
            return nullptr;
        }
        if (!c->start(prog, args, m_root)) return nullptr;
        m_lspRestarts[c] << QDateTime::currentMSecsSinceEpoch(); // Stage 31
        reopenLspDocs(c, def->lang, def->suffixes); // Stage 27: çökme sonrası belgeleri geri aç
    }
    return c->isRunning() ? c : nullptr;
}

// Stage 27: ertelenmiş didChange'leri boşalt
void MainWindow::flushLspDocs() {
    const auto dirty = m_lspDirty;
    m_lspDirty.clear();
    for (const QPointer<CodeEditor>& e : dirty)
        if (!e.isNull()) pushDocToLsp(e, true);
}

// Stage 27: derleme veritabanı değişince sunucuya bildir
void MainWindow::watchLspConfig() {
    if (!m_lspWatcher) {
        m_lspWatcher = new QFileSystemWatcher(this);
        m_lspBurstCount = 0;
        m_lspBurstMs = 0;
        connect(m_lspWatcher, &QFileSystemWatcher::fileChanged, this,
                [this](const QString& path) {
                    // Stage 31: taşmada sakinleş (10 sn içinde 5+)
                    const qint64 now = QDateTime::currentMSecsSinceEpoch();
                    if (now - m_lspBurstMs > 10000) m_lspBurstCount = 0;
                    m_lspBurstMs = now;
                    if (++m_lspBurstCount > 5) return;
                    if (m_lspCpp && m_lspCpp->isReady())
                        m_lspCpp->didChangeConfiguration(QJsonObject());
                    toast(0, "Derleme yapılandırması değişti: " +
                                 QFileInfo(path).fileName() + " (LSP bilgilendirildi)");
                    // Tek seferlik tetiklenir; yeniden izle
                    if (QFile::exists(path)) m_lspWatcher->addPath(path);
                });
    } else {
        for (const QString& p : m_lspWatcher->files()) m_lspWatcher->removePath(p);
    }
    if (m_root.isEmpty()) return;
    for (const QString& f : {".clangd", "compile_commands.json"}) {
        const QString p = QDir(m_root).absoluteFilePath(f);
        if (QFile::exists(p)) m_lspWatcher->addPath(p);
    }
}

// Stage 27: (yeniden) başlatılan sunucuya açık belgeleri geri bildir
void MainWindow::reopenLspDocs(LspClient* c, const QString& lang, const QStringList& suffixes) {
    if (!c) return;
    for (CodeEditor* e : allEditors()) {
        if (e->filePath().isEmpty() || e->largeFileMode() || e->isRemote()) continue;
        const QString suf = QFileInfo(e->filePath()).suffix().toLower();
        if (!suffixes.contains(suf)) continue;
        c->didOpen(e->filePath(), lang, e->toPlainText());
    }
}

// Stage 27: çökme kurtarma — bildir + sonraki kullanımda otomatik başlar
void MainWindow::onLspCrashed(LspClient* c, const QString& msg) {
    Q_UNUSED(c);
    m_status->setText(msg);
    toast(2, "LSP sunucusu kapandı — sonraki istekte yeniden başlatılacak");
}

// Stage 27: indeksleme ilerlemesi durum çubuğunda
void MainWindow::onLspProgress(const QString& kind, const QString& title, int percent) {
    if (kind == "end" || percent >= 100) {
        if (m_status->text().startsWith("LSP: ")) m_status->setText("");
        return;
    }
    QString t = title.isEmpty() ? "LSP çalışıyor" : title;
    m_status->setText(QString("LSP: %1%2").arg(t).arg(percent >= 0 ? QString(" %%%1").arg(percent) : ""));
}

void MainWindow::setupLspFor(const QString& path, const QString& text) {
    QString lang;
    if (LspClient* c = lspClientFor(QFileInfo(path).suffix(), lang))
        c->didOpen(path, lang, text);
}

void MainWindow::pushDocToLsp(CodeEditor* editor, bool immediate) {
    if (!editor || editor->filePath().isEmpty() || editor->largeFileMode()) return;
    // Stage 27: yazarken ertele (400 ms), istek öncesi anında gönder
    if (!immediate) {
        if (!m_lspFlushTimer) {
            m_lspFlushTimer = new QTimer(this);
            m_lspFlushTimer->setSingleShot(true);
            connect(m_lspFlushTimer, &QTimer::timeout, this, &MainWindow::flushLspDocs);
        }
        m_lspDirty.removeAll(QPointer<CodeEditor>());
        QPointer<CodeEditor> guard(editor);
        if (!m_lspDirty.contains(guard)) m_lspDirty << guard;
        m_lspFlushTimer->start(400);
        if (editor == currentEditor() && m_inlayTimer) m_inlayTimer->start();
        return;
    }
    // Stage 16: uzak belge → uzak LSP (şeritli uzak yol ile)
    if (editor->isRemote()) {
        if (m_lspRemote && m_lspRemote->isReady()) {
            const auto u = RemoteFileSystem::parseUri(editor->filePath());
            const QString suf = QFileInfo(u.path).suffix().toLower();
            const QString lang = (suf == "py") ? "python" : "cpp";
            if (u.ok && (lang == "python" || QString("cpp,h,hpp,c,cc,cxx").split(',').contains(suf)))
                m_lspRemote->didChange(u.path, editor->toPlainText());
        }
        return;
    }
    QString lang;
    if (LspClient* c = lspClientFor(QFileInfo(editor->filePath()).suffix(), lang))
        c->didChange(editor->filePath(), editor->toPlainText());
    // Stage 13: semantik/inlay'i periyodik tazele (debounce)
    if (editor == currentEditor() && m_inlayTimer) m_inlayTimer->start();
}

void MainWindow::teardownLsp(const QString& path) {
    if (path.isEmpty()) return;
    QString lang;
    // Hangi istemcide açıksa kapat (ucuz: ikisine de gönder)
    m_lspCpp->didClose(path);
    m_lspPy->didClose(path);
    Q_UNUSED(lang);
}

void MainWindow::onDiagnostics(const QString& path, const QList<LspDiag>& diags) {
    m_diags[path] = diags;
    refreshProblemView();
    if (auto* e = currentEditor())
        if (e->filePath() == path) {
            e->setDiagnostics(mergedDiags3(m_diags, m_spellDiags, m_buildDiags, path));
            refreshDiagMinimap(); // Stage 17
        }
    int total = 0;
    for (auto it = m_diags.begin(); it != m_diags.end(); ++it) total += it.value().size();
    for (auto it = m_spellDiags.begin(); it != m_spellDiags.end(); ++it) total += it.value().size();
    for (auto it = m_buildDiags.begin(); it != m_buildDiags.end(); ++it) total += it.value().size();
    if (total > 0) m_status->setText(QString("%1 sorun (⚠ panelinde)").arg(total));
    // Stage 9: aktivite çubuğu ikon butonunda sayı rozeti
    m_btnProblems->setText(total > 0 ? QString::number(total) : QString());
    refreshActivityIcons();
}

void MainWindow::checkSpelling() {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    SpellChecker* sc = e->spellChecker();
    if (!sc) { m_status->setText("Yazım denetimi kapalı ya da sözlük yok (⚙ → Yazım dili)."); return; }
    QList<LspDiag> out;
    for (const SpellHit& h : Spelling::checkText(e->toPlainText(), *sc)) {
        LspDiag d;
        d.path = e->filePath();
        QTextBlock b = e->document()->findBlock(h.pos);
        d.line = b.blockNumber();
        d.col = h.pos - b.position();
        d.endLine = d.line;
        d.endCol = d.col + h.len;
        d.severity = 4; // Hint
        d.source = "yazım";
        d.message = "“" + h.word + "”" +
                    (h.suggestions.isEmpty() ? "" : (" → " + h.suggestions.join(", ")));
        out << d;
    }
    m_spellDiags[e->filePath()] = out;
    refreshProblemView();
    e->setDiagnostics(mergedDiags3(m_diags, m_spellDiags, m_buildDiags, e->filePath()));
    m_status->setText(out.isEmpty() ? "Yazım denetimi temiz ✓"
                                    : QString("%1 yazım önerisi (⚠ panelinde)").arg(out.size()));
}

void MainWindow::loadFullCurrent() {
    if (auto* e = currentEditor())
        if (e->previewMode() && e->loadFullPreview()) {
            setupLspFor(e->filePath(), e->toPlainText());
            updateCursorStatus();
            updateBreadcrumb();
            m_status->setText("Tamamı yüklendi: " + e->filePath());
        }
}

void MainWindow::lspDefinition() {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c) { m_status->setText("Bu dil için LSP yok."); return; }
    if (!c->isReady()) { withLspReady(c, [this]() { lspDefinition(); }); return; }
    QTextCursor cur = e->textCursor();
    c->requestDefinition(e->filePath(), cur.blockNumber(), cur.columnNumber(),
        [this](QJsonObject res) {
            // Yanıt: tek Location | Location[] | LocationLink[] (clangd dizi döner)
            const QList<LspLocation> locs = LocationSet::parse(res);
            if (locs.isEmpty()) { m_status->setText("Tanım bulunamadı."); return; }
            const LspLocation& l = locs.first();
            openFileAt(l.path, l.line + 1);
            if (auto* ne = currentEditor()) ne->flashLine(l.line);
        });
}

void MainWindow::lspHover() {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c) { m_status->setText("Bu dil için LSP yok."); return; }
    if (!c->isReady()) { withLspReady(c, [this]() { lspHover(); }); return; }
    QTextCursor cur = e->textCursor();
    QRect r = e->cursorRect(cur);
    // Sembol adı: imleçteki kelime
    QString sym = cur.hasSelection() ? cur.selectedText().left(80) : QString();
    if (sym.isEmpty()) {
        QTextCursor w = cur;
        w.select(QTextCursor::WordUnderCursor);
        sym = w.selectedText().left(80);
    }
    const QString title = QString("%1 — %2").arg(QFileInfo(e->filePath()).fileName(), sym);
    c->requestHover(e->filePath(), cur.blockNumber(), cur.columnNumber(),
        [this, e, r, title](QJsonObject res) {
            QString text;
            QJsonValue cv = res["contents"];
            if (cv.isString()) text = cv.toString();
            else if (cv.isObject()) text = cv.toObject()["value"].toString();
            else if (cv.isArray() && !cv.toArray().isEmpty()) {
                QJsonValue v0 = cv.toArray().first();
                text = v0.isString() ? v0.toString() : v0.toObject()["value"].toString();
            }
            if (text.isEmpty()) text = "(bilgi yok)";
            // Stage 11: düz tooltip yerine temalı zengin kart
            const ThemeTokens tk = ThemeManager::instance().tokens();
            HoverCard::Colors hc;
            hc.bg = tk.surface.name();
            hc.border = tk.border.name();
            hc.title = tk.accent.name();
            hc.text = tk.text.name();
            hc.code = tk.synString.name();
            hc.codeBg = tk.bg.name();
            hc.dim = tk.textDim.name();
            QToolTip::showText(e->mapToGlobal(r.bottomRight()),
                               HoverCard::render(title, text.left(1500), hc), e);
        });
}

// --- Komutlar ---
void MainWindow::openFolderDialog() {
    QString d = QFileDialog::getExistingDirectory(this, "Klasör Aç", m_root);
    if (d.isEmpty()) return;
    m_root = d;
    refreshProjectViews();
    checkWorkspaceTrust();   // Stage 28
    pushRecentProject(d);    // Stage 28
    AppSettings cur = SettingsManager::instance().load();
    cur.lastRoot = d;
    SettingsManager::instance().save(cur);
}

void MainWindow::refreshProjectViews() {
    m_gitIgnore->load(m_root);
    m_explorer->setRoot(m_root);
    m_explorer->setGitIgnore(m_gitIgnore);
    m_search->setRoot(m_root);
    m_git->setWorkdir(m_root);
    m_terminal->setWorkdir(m_root);
    m_todo->setRoot(m_root);
    m_history->setWorkdir(m_root);
    m_stashTab->setWorkdir(m_root);
    m_remoteTab->setWorkdir(m_root);
    if (m_taskPanel) m_taskPanel->setRoot(m_root);
    startScheduledTasks(); // Stage 25
    loadExternalTools();   // Stage 25
    watchLspConfig();      // Stage 27: .clangd izleme
    if (m_plugins) m_plugins->setWorkspaceRoot(m_root); // Stage 29
    startAiSchedule(); // Stage 30
    if (m_chipGit) refreshGitBranch();
    // Stage 8: .verso/workspace.json geçersiz kılmaları (font/tab)
    if (!m_root.isEmpty()) {
        const WorkspaceSettings w = WorkspaceConfig::load(m_root);
        if (w.fontSize > 0 || w.tabWidth > 0) {
            AppSettings base = SettingsManager::instance().load();
            const int fs = w.fontSize > 0 ? w.fontSize : base.fontSize;
            const int tw = w.tabWidth > 0 ? w.tabWidth : base.tabWidth;
            for (CodeEditor* e : allEditors()) e->applyEditorSettings(fs, tw);
        }
    }
    if (!m_root.isEmpty()) m_sessions->touchRoot(m_root);
}

void MainWindow::runCommand(const QString& id) {
    if (!m_macroPlaying) m_macro.push(id); // Stage 25: makro kaydı
    // Stage 28: güvenilmez alanda tehlikeli komutlar kapalı
    if (!m_trusted && (id.startsWith("ai.") || id.startsWith("plugin.") || id == "task.run")) {
        if (!requireTrusted("Bu işlem")) return;
    }
    if (id == "file.openFolder") openFolderDialog();
    else if (id == "file.new") newUntitledFile();
    else if (id == "file.language") changeFileLanguage();
    else if (id == "file.encodingInfo") showEncodingInfo();
    else if (id == "edit.toggleEol") toggleEol();
    else if (id == "edit.pasteCycle") pasteCyclePrevious();
    else if (id == "edit.multiCaretLineEnds") multiCaretLineEnds();
    else if (id == "ai.exportChat") { if (m_ai) m_ai->exportChatMarkdown(); }
    else if (id == "ai.compare") { if (m_ai) m_ai->sendCompare(); }
    else if (id == "ai.undoAnswer") { if (m_ai) m_ai->undoLastAnswer(); }
    else if (id == "ai.undoEdit") undoAiEdit();
    else if (id == "edit.toggleComment") toggleComment();
    else if (id == "edit.bookmarkToggle") toggleBookmark();
    else if (id == "edit.bookmarkNext") jumpBookmark(1);
    else if (id == "edit.bookmarkPrev") jumpBookmark(-1);
    else if (id == "view.bookmarks") showBookmarks();
    else if (id == "view.markdownPreview") showMarkdownPreview();
    else if (id == "file.openHex") showHexView();
    else if (id == "file.copyPath") copyPath(0);
    else if (id == "file.copyRelativePath") copyPath(1);
    else if (id == "file.copyFileName") copyPath(2);
    else if (id == "edit.organizeImports") organizeImports();
    else if (id == "git.history") showGitHistory();
    else if (id == "git.tags") showGitTags();
    else if (id == "term.find") terminalFind(true);
    else if (id == "term.findNext") { if (m_terminal && !m_terminal->findNext(true)) toast(0, "Eşleşme yok"); }
    else if (id == "term.findPrev") { if (m_terminal && !m_terminal->findNext(false)) toast(0, "Eşleşme yok"); }
    else if (id == "view.notifications") showNotifications();
    else if (id == "file.projectNotes") openProjectNotes();
    else if (id == "file.bulkRename") bulkRenameHere();
    else if (id == "debug.runFile") debugRunFile();
    else if (id == "edit.merge") openMergeEditor();
    else if (id == "search.editor") { if (m_search) m_search->openInEditor(); }
    else if (id == "file.newProject") newProjectWizard();
    else if (id == "file.newFromTemplate") newFileFromTemplate();
    else if (id == "term.newTab") { if (m_terminal) m_terminal->newSession(); }
    else if (id == "remote.importSsh") importSshConfig();
    else if (id == "view.clipboard") showClipboardManager();
    else if (id == "task.chain") runTaskChain();
    else if (id == "remote.reconnect") remoteReconnect();
    else if (id == "ai.searchChats") searchAiChats();
    else if (id == "ai.agentPanel") { if (m_ai) m_ai->openAgentPanel(); }
    else if (id == "ai.autonomousAudit") { if (m_ai) m_ai->runAutonomousAudit(); }
    else if (id == "workspace.trust") {
        QSettings q("Verso", "VersoCoder");
        q.remove("trust/roots");
        q.remove("trust/denied");
        checkWorkspaceTrust();
    }
    else if (id == "ai.reviewStaged") reviewStagedDiff();
    else if (id == "nav.symbol") showSymbolSearch();
    else if (id == "view.radar") showMetricsRadar();
    else if (id == "file.projectNotes") openProjectNotes();
    else if (id == "file.bulkRename") bulkRenameHere();
    else if (id == "test.runRelated") { if (auto* e = currentEditor()) runRelatedTest(e->filePath()); }
    else if (id == "macro.record") toggleMacroRecord();
    else if (id == "macro.play") playMacro();
    else if (id == "macro.clear") { m_macro.clear(); m_macro.save(); toast(0, "Makro temizlendi"); }
    else if (id == "view.font") chooseEditorFont();
    else if (id == "file.save") saveCurrent();
    else if (id == "file.saveAll") saveAll();
    else if (id == "nav.quickOpen") showQuickOpen();
    else if (id == "nav.palette") showPalette();
    else if (id == "nav.search" || id == "nav.search.focus") {
        m_side->setCurrentIndex(1);
        m_btnExplorer->setChecked(false);
        m_btnSearch->setChecked(true);
        m_btnGit->setChecked(false);
        m_btnAi->setChecked(false);
        m_btnProblems->setChecked(false);
        m_sideTitle->setText(sideTitleFor(1));
        m_search->focusSearch();
    } else if (id == "nav.gotoLine") gotoLineDialog();
    else if (id == "nav.splitRight") splitActiveToOther();
    else if (id == "nav.moveTab") moveActiveTab();
    else if (id == "edit.moveUp") m_actions["edit.moveUp"]->trigger();
    else if (id == "edit.moveDown") m_actions["edit.moveDown"]->trigger();
    else if (id == "edit.duplicate") m_actions["edit.duplicate"]->trigger();
    else if (id == "edit.sort") m_actions["edit.sort"]->trigger();
    else if (id == "edit.trim") m_actions["edit.trim"]->trigger();
    else if (id == "edit.fold") m_actions["edit.fold"]->trigger();
    else if (id == "edit.unfold") m_actions["edit.unfold"]->trigger();
    else if (id == "edit.foldAll") m_actions["edit.foldAll"]->trigger();
    else if (id == "edit.unfoldAll") m_actions["edit.unfoldAll"]->trigger();
    else if (id == "edit.addNext") m_actions["edit.addNext"]->trigger();
    else if (id == "edit.spell") checkSpelling();
    else if (id == "large.loadFull") loadFullCurrent();
    else if (id == "view.minimap") m_actMinimap->toggle();
    else if (id == "view.terminal") m_termDock->setVisible(!m_termDock->isVisible());
    else if (id == "view.tasks") {
        m_termDock->setVisible(true);
        m_bottomTabs->setCurrentWidget(m_todo);
    } else if (id == "proj.compare") {
        CompareDialog d(this);
        d.setDirA(m_root);
        connect(&d, &CompareDialog::diffRequested, this, &MainWindow::diffExternal);
        d.exec();
    } else if (id == "session.saveSnapshot") saveSnapshot();
    else if (id == "session.loadSnapshot") loadSnapshot();
    else if (id == "ai.agent") {
        m_side->setCurrentIndex(3);
        m_btnAi->setChecked(true);
        m_sideTitle->setText(sideTitleFor(3));
        const bool on = !m_ai->agentMode();
        m_ai->setAgentMode(on);
        m_status->setText(on ? "AI ajan modu açık (araç kullanımı etkin)."
                             : "AI ajan modu kapalı.");
    }
    else if (id == "help.about" || id == "tool.diagnostics") showDiagnostics();
    else if (id == "help.shortcuts") showShortcutDialog();
    else if (id == "file.pluginsDir") openPluginsDir();
    else if (id == "view.plugins") showPluginManager();
    else if (id == "file.factoryReset") factoryReset();
    else if (id.startsWith("plugin.")) {
        if (m_pluginViews.contains(id)) showPluginView(id);
        else runPluginCommand(id);
    }
    else if (id == "view.plugins") showPluginManager();
    else if (id.startsWith("tool.")) runExternalTool(id);
    else if (id == "view.theme") openThemeGallery();
    else if (id == "view.git") showSidePanel(2);
    else if (id == "view.problems") showSidePanel(4);
    else if (id == "file.settings") openSettings();
    // Stage 10: mikro-etkileşim
    else if (id == "view.zen") toggleZen();
    else if (id == "view.zoomIn") zoom(+1);
    else if (id == "view.zoomOut") zoom(-1);
    else if (id == "view.zoomReset") zoom(0);
    else if (id == "ui.exportProfile") exportUiProfile();
    else if (id == "ui.importProfile") importUiProfile();
    // Stage 11: editör görsel derinliği
    else if (id == "edit.find") showFindBar();
    else if (id == "edit.findNext") findNavigate(+1);
    else if (id == "edit.findPrev") findNavigate(-1);
    else if (id == "edit.expandSel") expandSelectionSmart();
    // Stage 12: uyarlanabilir arayüz
    else if (id == "view.themeEditor") openThemeEditor();
    else if (id == "view.cycleDensity") cycleDensity();
    else if (id == "view.focusMode") toggleFocusMode();
    else if (id == "view.titleBar") toggleTitleBar();
    else if (id == "view.highContrast") m_actions["view.highContrast"]->trigger();
    else if (id == "ui.exportPng") exportPng();
    else if (id == "ui.profiles") showProfileMenu();
    else if (id == "ui.saveProfile") saveProfileAs();
    // Stage 13: dil zekâsı
    else if (id == "edit.complete") requestCompletion(false);
    else if (id == "lsp.references") showReferences();
    else if (id == "lsp.rename") renameSymbol();
    else if (id == "lsp.codeAction") showCodeActions();
    else if (id == "lsp.signature") showSignatureHelp(false);
    else if (id == "lsp.docSymbols") showDocSymbols();
    else if (id == "lsp.wsSymbols") showWorkspaceSymbols();
    else if (id == "format.document") formatDocument();
    else if (id == "lsp.calls") showCallHierarchy();
    else if (id == "lsp.openLink") openDocLink();
    else if (id == "lsp.typeHierarchy") showTypeHierarchy();
    else if (id == "lsp.pullDiagnostics") pullDiagnostics();
    else if (id == "format.selection") formatSelection();
    else if (id == "lsp.log") showLspLog();
    else if (id == "lsp.servers") showLspServers();
    else if (id == "lsp.sigNext") cycleSignature(1);
    else if (id == "lsp.sigPrev") cycleSignature(-1);
    else if (id == "view.inlayHints") toggleInlayHints();
    else if (id == "view.semantic") toggleSemantic();
    // Stage 14: hata ayıklama & test
    else if (id == "debug.start") debugStart();
    else if (id == "debug.stop") debugStop();
    else if (id == "debug.continue") debugContinue();
    else if (id == "debug.stepOver") debugNext();
    else if (id == "debug.stepInto") debugStep();
    else if (id == "debug.stepOut") debugFinish();
    else if (id == "debug.toggleBp") toggleBreakpointAtCursor();
    else if (id == "debug.smartStep") debugSmartStep();
    else if (id == "debug.functionBp") debugFunctionBp();
    else if (id == "debug.watchpoint") debugWatchpoint();
    else if (id == "debug.attach") debugAttach();
    else if (id == "debug.core") debugOpenCore();
    else if (id == "debug.substitutePath") debugSubstitutePath();
    else if (id == "debug.editBp") {
        if (auto* e = currentEditor())
            editBreakpoint(e->filePath(), e->textCursor().blockNumber() + 1);
    } else if (id == "debug.toggleSkipped") {
        m_debugSkipLib = !m_debugSkipLib;
        toast(0, m_debugSkipLib ? "Lib frame'leri gizli" : "Lib frame'leri görünür");
    } else if (id == "debug.setVariable") {
        bool ok = false;
        const QString ex = QInputDialog::getText(this, "Değeri Değiştir", "İfade:",
                                                 QLineEdit::Normal, QString(), &ok);
        if (ok && !ex.trimmed().isEmpty()) debugSetVariable(ex.trimmed());
    }
    else if (id == "test.discover") discoverTests();
    else if (id == "test.runAll") runAllTests();
    else if (id == "test.coverage") runCoverage();
    else if (id == "debug.launch") editLaunchConfig();
    // Stage 15: AI hattı
    else if (id == "ai.ghost") toggleGhost();
    else if (id == "ai.inlineEdit") inlineEditSelection();
    else if (id == "ai.fixSel") fixSelectionAi();
    else if (id == "ai.document") documentFunction();
    else if (id == "ai.genTest") genTestForCurrent();
    else if (id == "ai.commit") commitMessageAi();
    else if (id == "ai.explain") explainSymbol();
    else if (id == "ai.prompts") promptLibrary();
    else if (id == "ai.applyLast") applyLastAi();
    else if (id == "ai.pullModel") pullModel();
    else if (id == "ai.startServer") {
        toast(0, "Ollama başlatılıyor...");
        if (m_ai) m_ai->ensureServerAsync();
    }
    // Stage 16: uzaktan geliştirme
    else if (id == "remote.connect") remoteConnectDialog();
    else if (id == "remote.disconnect") remoteDisconnect();
    else if (id == "remote.explorer") showSidePanel(5);
    else if (id == "remote.open") remoteOpenDialog();
    else if (id == "remote.terminal") remoteTerminalShow();
    else if (id == "remote.lsp") remoteLspStart();
    else if (id == "remote.build") remoteBuild();
    else if (id == "remote.git") remoteGitStatus();
    else if (id == "remote.debug") remoteDebugStart();
    else if (id == "remote.forward") remoteForwardShow();
    // Stage 17: editör deneyimi
    else if (id == "snippet.insert") snippetPalette(false);
    else if (id == "snippet.new") snippetPalette(true);
    else if (id == "edit.pasteAs") pasteAs();
    else if (id == "edit.wordComplete") wordComplete();
    else if (id == "outline.show") outlineShow();
    else if (id == "fold.refreshLsp") foldingRefreshLsp();
    else if (id == "history.timeline") timelineShow();
    else if (id == "history.restorePrev") timelineRestorePrev();
    else if (id == "search.exclude") searchExcludeFocus();
    else if (id == "git.hunk") hunkMenuAtCursor();
    else if (id == "task.panel") {
        m_termDock->setVisible(true);
        m_bottomTabs->setCurrentWidget(m_taskPanel);
    }
    else if (id == "task.run") {
        if (!requireTrusted("Görev çalıştırma")) return; // Stage 28
        m_termDock->setVisible(true);
        m_bottomTabs->setCurrentWidget(m_taskPanel);
        m_taskPanel->runSelected();
    }
    else if (id == "run.build") runBuildForCurrent();
    else if (id == "lsp.definition") lspDefinition();
    else if (id == "lsp.hover") lspHover();
}

void MainWindow::showPalette() {
    CommandPalette pal(this);
    AppSettings s = SettingsManager::instance().load();
    QList<PaletteCommand> cmds;
    for (const auto& c : defaultCommands())
        cmds << PaletteCommand{c.id, c.title, c.id, effectiveKeys(s.shortcuts, c)};
    for (auto it = m_pluginCmds.constBegin(); it != m_pluginCmds.constEnd(); ++it)
        cmds << PaletteCommand{it.key(), it.value(), it.key(), QString()}; // Stage 22
    for (auto it = m_toolCmds.constBegin(); it != m_toolCmds.constEnd(); ++it)
        cmds << PaletteCommand{it.key(), it.value().label, it.key(), QString()}; // Stage 25
    pal.setCommands(cmds);
    if (pal.exec() == QDialog::Accepted && !pal.selectedId().isEmpty()) {
        CommandPalette::pushRecent(pal.selectedId()); // Stage 10: son kullanılanlar
        CommandPalette::recordUse(pal.selectedId());  // Stage 30: sıklık
        runCommand(pal.selectedId());
    }
}

void MainWindow::gotoLineDialog() {
    auto* e = currentEditor();
    if (!e) return;
    bool ok = false;
    int n = QInputDialog::getInt(this, "Satıra Git", "Satır:",
                                 e->textCursor().blockNumber() + 1, 1, e->blockCount(), 1, &ok);
    if (ok) e->gotoLine(n);
}

void MainWindow::applyShortcuts() {
    AppSettings s = SettingsManager::instance().load();
    for (const auto& c : defaultCommands()) {
        auto it = m_actions.find(c.id);
        if (it == m_actions.end()) continue;
        QString keys = effectiveKeys(s.shortcuts, c);
        (*it)->setShortcut(keys.isEmpty() ? QKeySequence() : QKeySequence(keys));
        (*it)->setText(c.title + (keys.isEmpty() ? "" : "  [" + keys + "]"));
    }
}

QList<CodeEditor*> MainWindow::allEditors() const {
    QList<CodeEditor*> out;
    for (QTabWidget* t : {m_tabs, m_tabs2})
        for (int i = 0; i < t->count(); ++i)
            if (auto* e = qobject_cast<CodeEditor*>(t->widget(i))) out << e;
    return out;
}

void MainWindow::applyEditorSettingsTo(CodeEditor* e) {
    AppSettings s = SettingsManager::instance().load();
    e->applyEditorSettings(s.fontSize, s.tabWidth);
    // Stage 11: boşluk görünümü + cetvel + sticky
    e->setShowWhitespace(s.showWhitespace);
    e->setShowLineEnds(s.showLineEnds); // Stage 23
    // Stage 23: imleç + kaydırma + vurgular + katlama oku
    e->setCursorStyle(s.cursorStyle);
    e->setCursorBlinkMs(s.cursorBlink);
    e->setSmoothScroll(s.smoothScroll);
    e->setBracketStyle(s.bracketStyle);
    e->setFoldGutter(s.foldGutter);
    e->setLineHiOpacity(s.lineHighlightOpacity);
    checkEditorGrid(e); // Stage 23: ızgara garantisi
    e->setRulerColumn(s.rulerColumn);
    e->refreshSticky();
    e->setFocusMode(s.focusMode); // Stage 12
}

void MainWindow::applyEditorSettingsToAll() {
    for (CodeEditor* e : allEditors()) applyEditorSettingsTo(e);
}

void MainWindow::openFile(const QString& path) {
    if (openSpecial(path)) { // Stage 28: görüntü/md dışı ikililer
        m_status->setText(path);
        return;
    }
    if (auto* e = openEditorFor(path)) {
        // Stage 32: büyük dosya kipi — önizleme / salt-okunur eşikleri
        AppSettings cfg = SettingsManager::instance().load();
        const int mode = PerfTools::largeFileMode(QFileInfo(path).size(),
                                                  cfg.largeFileMb, 8);
        if (mode == 2 && !e->isReadOnly()) {
            e->setReadOnly(true);
            toast(2, "Büyük dosya salt-okunur açıldı (8 MB+)");
        } else if (mode == 1) {
            toast(2, "Büyük dosya — vurgu sınırlı (önizleme kipi)");
        }
        m_status->setText(path);
    }
}

// Stage 24: tek-tık önizleme — varsa önizleme sekmesini dönüştür, yoksa aç
void MainWindow::openPreview(const QString& path) {
    if (path.isEmpty() || !QFileInfo(path).isFile()) return;
    // Stage 46: bekleyen sekme varsa kopya önizleme açılmaz, gerçeklenir
    for (QTabWidget* t : {m_tabs, m_tabs2}) {
        for (int i = 0; i < t->count(); ++i) {
            if (t->widget(i)->property("pendingPath").toString() == path
                && !path.isEmpty()) {
                materializePending(t->widget(i));
                return;
            }
        }
    }
    for (QTabWidget* t : {m_tabs, m_tabs2}) {
        for (int i = 0; i < t->count(); ++i) {
            auto* e = qobject_cast<CodeEditor*>(t->widget(i));
            if (!e) continue;
            if (e->filePath() == path) { // zaten açık: tam yükle + odakla
                if (e->previewMode()) e->loadFullPreview();
                t->setCurrentIndex(i);
                m_minimap->setEditor(e);
                updateCursorStatus();
                return;
            }
        }
    }
    // Başka dosyanın önizlemesi varsa onu dönüştür (tek önizleme sekmesi)
    for (QTabWidget* t : {m_tabs, m_tabs2}) {
        for (int i = 0; i < t->count(); ++i) {
            auto* e = qobject_cast<CodeEditor*>(t->widget(i));
            if (e && e->previewMode()) {
                if (e->loadPreview(path)) {
                    t->setTabText(i, QFileInfo(path).fileName() + "  ~");
                    t->setTabToolTip(i, path + " (önizleme — çift tık tam açar)");
                    t->setCurrentIndex(i);
                    m_minimap->setEditor(e);
                    updateCursorStatus();
                    return;
                }
            }
        }
    }
    auto* e = new CodeEditor(this);
    applyEditorSettingsTo(e);
    if (!e->loadPreview(path)) { delete e; return; }
    addEditorTab(e, QFileInfo(path).fileName() + "  ~", path + " (önizleme)");
}

// Stage 24: tam açış önizlemeyi kalıcılaştırır
// (openEditorFor içinde ele alınır — önizleme sekmesi aynı yola dönüşür)

// Stage 24: Proje Radarı (metrik/kopya/kullanılmayan/bağımlılık)
void MainWindow::showMetricsRadar() {
    CodeEditor* e = currentEditor();
    auto* d = new MetricsDialog(m_root, e ? e->filePath() : QString(), this);
    d->setAttribute(Qt::WA_DeleteOnClose);
    connect(d, &MetricsDialog::fileJumpRequested, this,
            [this, d](const QString& path, int line) {
                d->close();
                openFileAt(path, line);
            });
    d->show();
}

// Stage 25: toplu yeniden adlandırma (etkin dosya dizini)
void MainWindow::bulkRenameHere() {
    CodeEditor* e = currentEditor();
    QString dir = m_root;
    if (e && !e->filePath().isEmpty() && !e->isRemote())
        dir = QFileInfo(e->filePath()).absolutePath();
    if (dir.isEmpty()) { toast(0, "Önce klasör açın"); return; }
    BulkRenameDialog d(dir, this);
    d.exec();
    // QFileSystemModel dosya değişimlerini kendisi izler; ek tazeleme gerekmez
}

// Stage 25: zamanlanmış görevler (tasks.json scheduleMin)
void MainWindow::startScheduledTasks() {
    qDeleteAll(m_schedTimers);
    m_schedTimers.clear();
    if (!m_schedRunner) {
        m_schedRunner = new TaskRunner(this);
        connect(m_schedRunner, &TaskRunner::finished, this,
                [this](const QString& label, int code) {
                    toast(code == 0 ? 1 : 2,
                          QString("Zamanlanmış '%1': çıkış %2").arg(label).arg(code));
                });
    }
    if (m_root.isEmpty() || !m_schedRunner->loadForRoot(m_root)) return;
    for (const TaskDef& t : m_schedRunner->tasks()) {
        if (t.scheduleMin <= 0) continue;
        auto* timer = new QTimer(this);
        timer->setInterval(t.scheduleMin * 60000);
        const QString label = t.label;
        connect(timer, &QTimer::timeout, this, [this, label]() {
            if (!m_schedRunner->isRunning()) m_schedRunner->runLabel(label, m_root);
        });
        timer->start();
        m_schedTimers << timer;
    }
    if (!m_schedTimers.isEmpty())
        m_status->setText(QString("%1 zamanlanmış görev").arg(m_schedTimers.size()));
}

// Stage 25: harici araçlar (.verso/tools.json → tool.* komutları)
void MainWindow::loadExternalTools() {
    m_toolCmds.clear();
    if (m_root.isEmpty()) return;
    QFile f(ExternalTools::configPathForRoot(m_root));
    if (!f.open(QIODevice::ReadOnly)) return;
    QString err;
    for (const ToolDef& t : ExternalTools::parse(QString::fromUtf8(f.readAll()), &err))
        m_toolCmds[ExternalTools::commandId(t.label)] = t;
    if (!m_toolCmds.isEmpty())
        m_status->setText(QString("%1 harici araç").arg(m_toolCmds.size()));
}

void MainWindow::runExternalTool(const QString& id) {
    if (!m_toolCmds.contains(id)) return;
    if (m_toolProc && m_toolProc->state() != QProcess::NotRunning) {
        toast(0, "Bir araç zaten çalışıyor");
        return;
    }
    const ToolDef t = m_toolCmds[id];
    m_toolProc = new QProcess(this);
    const QString dir = t.cwd.isEmpty() ? m_root : QDir(m_root).absoluteFilePath(t.cwd);
    m_toolProc->setWorkingDirectory(dir);
    connect(m_toolProc, &QProcess::readyReadStandardOutput, this, [this]() {
        if (m_terminal)
            m_terminal->appendOut(QString::fromUtf8(m_toolProc->readAllStandardOutput()));
    });
    connect(m_toolProc, &QProcess::readyReadStandardError, this, [this]() {
        if (m_terminal)
            m_terminal->appendOut(QString::fromUtf8(m_toolProc->readAllStandardError()),
                                  QColor("#f44747"));
    });
    connect(m_toolProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, id](int code, QProcess::ExitStatus) {
                toast(code == 0 ? 1 : 2, QString("%1: çıkış %2").arg(id).arg(code));
                m_toolProc->deleteLater();
                m_toolProc = nullptr;
            });
    if (m_terminal) m_terminal->appendOut("$ " + t.command + "\n", QColor("#569cd6"));
    m_toolProc->start("bash", {"-lc", t.command});
}

// Stage 25: staged diff'e AI kod incelemesi
void MainWindow::reviewStagedDiff() {
    if (m_root.isEmpty()) return;
    QProcess git;
    git.setWorkingDirectory(m_root);
    git.start("git", {"diff", "--cached", "--no-color"});
    if (!git.waitForFinished(8000)) { toast(3, "git diff zaman aşımı"); return; }
    const QString diff = QString::fromUtf8(git.readAllStandardOutput()).trimmed();
    if (diff.isEmpty()) { toast(0, "Staged değişiklik yok (önce Stage yapın)"); return; }
    askAiTask(int(AiTask::Review), "Titiz kod inceleme asistanı. Kısa, maddeli, Türkçe yaz.",
          "Aşağıdaki staged diff'i incele: hata riski, kenar durumları, stil. "
          "Varsa en kritik 5 madde; yoksa 'Temiz ✓'.\n```diff\n" + diff.left(12000) + "\n```",
          QJsonObject(), [this](QString reply, QString err) {
              if (!err.isEmpty()) toast(3, "AI hatası: " + err.left(120));
              else m_status->setText("İnceleme hazır — AI paneline bakın");
              Q_UNUSED(reply);
          });
}

// Stage 25: kaydetmede ilgili testi koştur (ayarlıysa)
void MainWindow::runRelatedTest(const QString& sourcePath) {
    if (sourcePath.isEmpty() || sourcePath.startsWith("ssh://")) return;
    const QString target = TestGen::targetFor(sourcePath);
    if (target.isEmpty() || !QFile::exists(target)) return;
    const QString suf = QFileInfo(target).suffix().toLower();
    QStringList args;
    QString prog;
    if (suf == "py") {
        prog = "python3";
        args = {"-m", "pytest", target, "-q"};
    } else {
        const QString buildDir = QDir(m_root).absoluteFilePath("build");
        if (!QFileInfo::exists(buildDir + "/CTestTestfile.cmake")) return;
        prog = "ctest";
        args = {"--test-dir", buildDir, "-R",
                QFileInfo(target).completeBaseName(), "--output-on-failure"};
    }
    auto* p = new QProcess(this);
    p->setWorkingDirectory(QFileInfo(target).absolutePath());
    connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, target, p](int code, QProcess::ExitStatus) {
                toast(code == 0 ? 1 : 2,
                      QString("%1: %2").arg(QFileInfo(target).fileName(),
                                            code == 0 ? "testler geçti ✓" : "test BAŞARISIZ"));
                p->deleteLater();
            });
    m_status->setText("Test koşuyor: " + QFileInfo(target).fileName());
    p->start(prog, args);
}

// Stage 24: proje notları (.verso/notes.md)
void MainWindow::openProjectNotes() {
    if (m_root.isEmpty()) { toast(0, "Önce klasör açın"); return; }
    QDir().mkpath(m_root + "/.verso");
    const QString p = m_root + "/.verso/notes.md";
    if (!QFile::exists(p)) {
        QFile f(p);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            f.write("# Proje Notları\n\n- \n");
    }
    openFile(p);
}

// Stage 24: çalışma alanı sembol araması (Ctrl+T)
void MainWindow::showSymbolSearch() {
    if (m_root.isEmpty()) { toast(0, "Önce klasör açın"); return; }
    QMap<QString, QString> texts;
    QDirIterator it(m_root, QDir::Files | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    int n = 0;
    static const QStringList exts = {"c",  "h",  "cpp", "hpp", "cc", "py", "js",
                                     "ts", "java", "go", "rs"};
    while (it.hasNext() && n < 1200) {
        const QString f = it.next();
        if (f.contains("/.git/") || f.contains("/build/") || f.contains("/node_modules/"))
            continue;
        if (!exts.contains(QFileInfo(f).suffix().toLower())) continue;
        QFile qf(f);
        if (!qf.open(QIODevice::ReadOnly)) continue;
        const QByteArray raw = qf.read(256 * 1024);
        if (raw.contains('\0')) continue;
        texts[f] = QString::fromUtf8(raw);
        ++n;
    }
    SymbolSearchDialog d(WorkspaceSymbols::scanFiles(texts), this);
    if (d.exec() == QDialog::Accepted && d.hasSelection())
        openFileAt(d.selected().file, d.selected().line);
}

void MainWindow::openFileAt(const QString& path, int line) {
    if (auto* e = openEditorFor(path)) {
        e->gotoLine(line);
        m_status->setText(QString("%1:%2").arg(path).arg(line));
    }
}

void MainWindow::saveCurrent() {
    if (auto* e = currentEditor()) {
        // Stage 16: uzak belge ssh ile yazılır
        if (e->isRemote()) { remoteSaveEditor(e); return; }
        // Stage 13: kaydetmede biçimlendir (async: yanıt gelince kaydet)
        if (formatOnSaveIfEnabled(e)) return;
        if (e->filePath().isEmpty()) {
            // Stage 20: dile uygun ad + filtre öner
            QString suggest = m_root;
            QString filter;
            const QString ext = LanguageSupport::defaultExtension(e->lang());
            if (!ext.isEmpty()) {
                QString base = activeTabs()->tabText(activeTabs()->currentIndex());
                base.remove(QRegularExpression(" • [^ ]+$"));
                if (base.isEmpty()) base = "adsiz";
                suggest = QDir(m_root).absoluteFilePath(base + "." + ext);
                if (const LanguageSupport::LangDef* d = LanguageSupport::findLang(e->lang()))
                    filter = QString("%1 (*.%2)").arg(d->title, ext);
            }
            QString p = QFileDialog::getSaveFileName(this, "Kaydet", suggest, filter);
            if (p.isEmpty()) return;
            e->saveFile(p);
            connect(e, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateCursorStatus, Qt::UniqueConnection);
            connect(e, &CodeEditor::externalChangeDetected, this, &MainWindow::onExternalChange, Qt::UniqueConnection);
            activeTabs()->setTabText(activeTabs()->currentIndex(), QFileInfo(p).fileName());
            activeTabs()->setTabToolTip(activeTabs()->currentIndex(), p);
            setupLspFor(p, e->toPlainText());
            saveSession();
        } else e->saveFile();
        snapshotEditor(e); // Stage 17: yerel geçmiş
        if (m_plugins) m_plugins->fireEvent("save", e->filePath()); // Stage 29
        // Stage 25: kaydetmede ilgili test
        if (SettingsManager::instance().load().testOnSave) runRelatedTest(e->filePath());
        m_status->setText("Kaydedildi: " + e->filePath());
        toast(1, "Kaydedildi: " + QFileInfo(e->filePath()).fileName()); // Stage 10
        refreshGitMarks(e); // Stage 11: diff işaretlerini güncelle
        updateCursorStatus();
    }
}

void MainWindow::saveAll() {
    int n = 0;
    for (CodeEditor* e : allEditors())
        if (e->isRemote()) {
            // Stage 16: çakışmada sorma (toplu kip sessiz atlar)
            if (!e->document()->isModified()) continue;
            if (remoteSaveEditor(e, false)) ++n;
        } else if (!e->filePath().isEmpty() && e->document()->isModified()) {
            e->saveFile();
            snapshotEditor(e); // Stage 17
            ++n;
        }
    if (n > 0) toast(1, QString("%1 dosya kaydedildi").arg(QString::number(n))); // Stage 10
    updateCursorStatus();
}

void MainWindow::autoSaveTick() {
    AppSettings s = SettingsManager::instance().load();
    if (!s.autoSave) return;
    bool any = false;
    for (CodeEditor* e : allEditors())
        if (!e->filePath().isEmpty() && e->document()->isModified()) {
            backupEditor(e);
            // Stage 16: uzak belgeler sessiz (çakışmada atlanır)
            if (e->isRemote()) { any |= remoteSaveEditor(e, false); continue; }
            e->saveFile();
            any = true;
        }
    if (any) { m_status->setText("Otomatik kaydedildi."); updateCursorStatus(); }
    saveSession();
}

void MainWindow::updateCursorStatus() {
    auto* e = currentEditor();
    if (!e) { m_cursorLabel->setText(""); return; }
    QTextCursor c = e->textCursor();
    int line = c.blockNumber() + 1;
    int col = c.columnNumber() + 1;
    const QString sel = c.hasSelection()
        ? QString(" (%1 seçili)").arg(c.selectedText().size()) : "";
    m_cursorLabel->setText(QString("Ln %1, Col %2%3").arg(line).arg(col).arg(sel));
    // Stage 9: dil + satır sonu çipleri
    static const QMap<QString, QString> langMap = {
        {"cpp", "C++"}, {"cc", "C++"}, {"cxx", "C++"}, {"hpp", "C++"}, {"h", "C/C++"},
        {"py", "Python"}, {"js", "JavaScript"}, {"ts", "TypeScript"}, {"json", "JSON"},
        {"md", "Markdown"}, {"sh", "Shell"}, {"bash", "Shell"}, {"qss", "QSS"},
        {"cmake", "CMake"}, {"txt", "Düz metin"}, {"xml", "XML"}, {"yaml", "YAML"}};
    const QString suffix = QFileInfo(e->filePath()).suffix().toLower();
    AppSettings as = SettingsManager::instance().load();
    // Stage 20: Adsız sekmede seçili dilin adı gösterilir
    QString lang = langMap.value(suffix, suffix.isEmpty() ? "Metin" : suffix.toUpper());
    if (e->isUntitled())
        if (const LanguageSupport::LangDef* d = LanguageSupport::findLang(e->lang()))
            lang = d->chip;
    m_chipLang->setText(QString("%1  ·  Boşluk: %2").arg(lang).arg(as.tabWidth));
    m_chipEol->setText(e->lineEnding());
    m_chipEnc->setText("UTF-8");
    if (m_chipGrid) m_chipGrid->setText(m_gridOk ? "Izgara ✓" : "Izgara ✗");
    // Stage 21: terminal dizin takibi
    if (m_terminal && !e->filePath().isEmpty())
        m_terminal->syncToDir(QFileInfo(e->filePath()).absolutePath());
    updateTabMarks();
}

void MainWindow::updateBreadcrumb() {
    auto* e = currentEditor();
    if (!e) { m_crumb->setPath(m_root, "", ""); return; }
    m_crumb->setPath(m_root, e->filePath(), e->largeFileMode() ? "" : e->toPlainText());
}

void MainWindow::onExternalChange(CodeEditor* editor) {
    // Stage 28: otomatik yeniden yükleme
    if (SettingsManager::instance().load().autoReload) {
        editor->reloadFromDisk();
        pushDocToLsp(editor);
        updateCursorStatus();
        updateBreadcrumb();
        toast(0, "Otomatik yüklendi: " + QFileInfo(editor->filePath()).fileName());
        return;
    }
    auto r = QMessageBox::question(this, "Dosya değişti",
        "Dosya diskte başka bir program tarafından değiştirilmiş:\n" + editor->filePath() +
        "\nYeniden yüklensin mi? (kaydedilmemiş değişikliklerin kaybolur)",
        QMessageBox::Yes | QMessageBox::No);
    if (r == QMessageBox::Yes) {
        editor->reloadFromDisk();
        pushDocToLsp(editor);
        updateCursorStatus();
        updateBreadcrumb();
    }
}

void MainWindow::applyAiEdit(const QString& newText, bool wholeFile, int selStart, int selLen) {
    auto* e = currentEditor();
    if (!e) return;
    QTextCursor c = e->textCursor();
    int sPos = 0;
    if (wholeFile) {
        c.select(QTextCursor::Document);
    } else {
        int maxPos = e->document()->characterCount() - 1;
        sPos = qBound(0, selStart, maxPos);
        c.setPosition(sPos);
        c.setPosition(qBound(0, selStart + selLen, maxPos), QTextCursor::KeepAnchor);
    }
    const QString oldFull = wholeFile ? e->toPlainText()
        : e->document()->toPlainText().mid(sPos, selLen);
    c.insertText(newText);
    e->setTextCursor(c);
    e->setFocus();
    pushAiUndo(e->filePath(), oldFull, e, wholeFile, sPos, selLen); // Stage 28
    pushDocToLsp(e);
    m_status->setText("AI düzenlemesi uygulandı (geri almak için Ctrl+Z).");
}

void MainWindow::showQuickOpen() {
    QuickOpenDialog d(this);
    d.setFiles(collectProjectFiles());
    if (d.exec() == QDialog::Accepted && !d.selected().isEmpty())
        openFile(d.selected());
}

void MainWindow::runBuildForCurrent() {
    if (auto* e = currentEditor()) {
        if (e->filePath().isEmpty()) saveCurrent();
        if (!e->filePath().isEmpty()) {
            m_termDock->show();
            m_terminal->runBuildFor(e->filePath());
        }
    }
}

QStringList MainWindow::collectProjectFiles() const {
    QStringList out;
    QDirIterator it(m_root, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    int n = 0;
    while (it.hasNext() && n < 5000) {
        QString p = it.next();
        if (p.contains("/.git/") || p.contains("/build/") || p.contains("/node_modules/")) continue;
        out << p;
        ++n;
    }
    return out;
}

void MainWindow::saveSession() {
    AppSettings s = SettingsManager::instance().load();
    s.lastRoot = m_root;
    DocSession d = captureSession();
    // Stage 50-debug: sekme değişiminde disk yazma atlanır (aynıysa).
    // QSettings eşitleme her seferinde dosya yazar; imleç oynamadan
    // sekme değiştirmek diske dokunmamalı.
    if (d.root == m_lastSavedSession.root && d.files == m_lastSavedSession.files &&
        d.cursors == m_lastSavedSession.cursors && d.files2 == m_lastSavedSession.files2 &&
        d.cursors2 == m_lastSavedSession.cursors2 && d.folds == m_lastSavedSession.folds &&
        d.active == m_lastSavedSession.active && d.active2 == m_lastSavedSession.active2)
        return;
    m_lastSavedSession = d;
    s.sessionFiles = d.files;
    s.sessionCursors = d.cursors;
    s.sessionFiles2 = d.files2;
    s.sessionCursors2 = d.cursors2;
    s.sessionFolds = d.folds;
    s.sessionActive = d.active;
    s.sessionActive2 = d.active2;
    SettingsManager::instance().save(s);
    // Stage 31: oturum bütünlük mührü
    QSettings q("Verso", "VersoCoder");
    q.setValue("session/sha", Stability::shaData(
                   (s.sessionFiles + s.sessionCursors + s.sessionFiles2 +
                    s.sessionCursors2 + s.sessionFolds)
                       .join("\n")
                       .toUtf8()));
    if (!m_root.isEmpty())
        m_sessions->save(ProjectSessions::projectKey(m_root), d);
}

DocSession MainWindow::captureSession() const {
    DocSession d;
    d.root = m_root;
    // Stage 46: bekleyen (henüz gerçeklenmemiş) sekmeler de oturuma yazılır;
    // imleçleri m_pendingCursors'tan alınır. Yoksa oturum her kayıtta erirdi.
    auto collect = [this](QTabWidget* t, QStringList& files, QStringList& curs) {
        for (int i = 0; i < t->count(); ++i) {
            if (auto* e = qobject_cast<CodeEditor*>(t->widget(i))) {
                if (!e->filePath().isEmpty()) {
                    files << e->filePath();
                    curs << (e->filePath() + "\x1F" + QString::number(e->cursorOffset()));
                }
                continue;
            }
            const QString pending = t->widget(i)->property("pendingPath").toString();
            if (!pending.isEmpty()) {
                files << pending;
                const int off = m_pendingCursors.value(pending, 0);
                curs << (pending + "\x1F" + QString::number(off));
            }
        }
    };
    collect(m_tabs, d.files, d.cursors);
    collect(m_tabs2, d.files2, d.cursors2);
    for (CodeEditor* e : allEditors()) {
        QList<int> fl = e->foldedStartLines();
        if (!fl.isEmpty() && !e->filePath().isEmpty()) {
            QStringList ns;
            for (int l : fl) ns << QString::number(l);
            d.folds << (e->filePath() + "\x1F" + ns.join(','));
        }
    }
    // Stage 46: bekleyen sekmelerin katlamaları da korunur
    for (auto it = m_pendingFolds.constBegin(); it != m_pendingFolds.constEnd(); ++it) {
        if (it.value().isEmpty()) continue;
        QStringList ns;
        for (int l : it.value()) ns << QString::number(l);
        d.folds << (it.key() + "\x1F" + ns.join(','));
    }
    d.active = m_tabs->currentIndex();
    d.active2 = m_tabs2->currentIndex();
    return d;
}

void MainWindow::applyDocSession(const DocSession& s) {
    auto parseCursors = [](const QStringList& in) {
        QMap<QString, int> out;
        for (const QString& c : in) {
            int k = c.indexOf("\x1F");
            if (k > 0) out[c.left(k)] = c.mid(k + 1).toInt();
        }
        return out;
    };
    QMap<QString, QList<int>> folds;
    for (const QString& f : s.folds) {
        int k = f.indexOf("\x1F");
        if (k <= 0) continue;
        QList<int> ls;
        for (const QString& n : f.mid(k + 1).split(',', Qt::SkipEmptyParts))
            ls << n.toInt();
        folds[f.left(k)] = ls;
    }
    // Stage 46: tembel geri yükleme — yalnız etkin sekme gerçekten açılır,
    // diğerleri hafif yer tutucudur (ilk tıklamada gerçeklenir). 50 sekmelik
    // oturum açılışı böylece tek dosya maliyetine iner.
    auto openGroup = [&](const QStringList& files, const QStringList& curs, int group, int active) {
        QMap<QString, int> cm = parseCursors(curs);
        QTabWidget* t = group == 0 ? m_tabs : m_tabs2;
        const QString activePath =
            (active >= 0 && active < files.size()) ? files.at(active) : QString();
        for (int i = 0; i < files.size(); ++i) {
            const QString& f = files.at(i);
            if (!QFileInfo::exists(f)) continue;
            if (f == activePath) {
                if (auto* e = openEditorFor(f, group)) {
                    if (cm.contains(f)) e->setCursorOffset(cm[f]);
                    if (folds.contains(f)) e->setFoldedLines(folds[f]);
                }
                continue;
            }
            // Yer tutucu: imleç/katlama gerçeklenince uygulanır
            if (cm.contains(f)) m_pendingCursors[f] = cm[f];
            if (folds.contains(f)) m_pendingFolds[f] = folds[f];
            auto* ph = new QLabel("Yükleniyor…", t);
            ph->setAlignment(Qt::AlignCenter);
            ph->setProperty("pendingPath", f);
            t->addTab(ph, QFileInfo(f).fileName());
            t->setTabToolTip(t->count() - 1, f);
        }
        if (active >= 0 && active < t->count()) t->setCurrentIndex(active);
    };
    openGroup(s.files, s.cursors, 0, s.active);
    if (!s.files2.isEmpty()) {
        openGroup(s.files2, s.cursors2, 1, s.active2);
        m_tabs2->setVisible(true);
    }
}

void MainWindow::restoreSession() {
    AppSettings s = SettingsManager::instance().load();
    if (!s.restoreSession) return;
    // Stage 31: mühür tutmuyorsa güvenli kip (geri yükleme yok)
    {
        QSettings q("Verso", "VersoCoder");
        const QString expect = q.value("session/sha").toString();
        const QString actual = Stability::shaData(
            (s.sessionFiles + s.sessionCursors + s.sessionFiles2 +
             s.sessionCursors2 + s.sessionFolds)
                .join("\n")
                .toUtf8());
        if (!expect.isEmpty() && expect != actual) {
            toast(2, "Oturum dosyası bozuk — güvenli kipte açıldı (sekmeler geri yüklenmedi)");
            return;
        }
    }
    DocSession d;
    d.root = s.lastRoot;
    d.files = s.sessionFiles;
    d.cursors = s.sessionCursors;
    d.active = s.sessionActive;
    d.files2 = s.sessionFiles2;
    d.cursors2 = s.sessionCursors2;
    d.active2 = s.sessionActive2;
    d.folds = s.sessionFolds;
    applyDocSession(d);
}

// --- Stage 6: anlık görüntüler ---
void MainWindow::saveSnapshot() {
    bool ok = false;
    QString name = QInputDialog::getText(this, "Anlık Görüntü", "Ad:",
                                          QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    m_sessions->save(ProjectSessions::snapshotKey(name.trimmed()), captureSession());
    m_status->setText("Anlık görüntü kaydedildi: " + name.trimmed());
}

void MainWindow::loadSnapshot() {
    QStringList names = m_sessions->snapshots();
    if (names.isEmpty()) {
        m_status->setText("Kayıtlı anlık görüntü yok.");
        return;
    }
    bool ok = false;
    QString name = QInputDialog::getItem(this, "Anlık Görüntü Yükle", "Ad:",
                                          names, 0, false, &ok);
    if (!ok) return;
    DocSession d;
    if (m_sessions->load(ProjectSessions::snapshotKey(name), d)) {
        applyDocSession(d);
        m_status->setText("Anlık görüntü yüklendi: " + name);
    }
}

// --- Stage 8: Platform & Dağıtım ---
// Stage 22: kısayol hile sayfası
void MainWindow::showShortcutDialog() {
    ShortcutDialog d(this);
    d.exec();
}

// Stage 22: eklenti dizinini dosya yöneticisinde aç
void MainWindow::openPluginsDir() {
    QDir().mkpath(pluginDir());
    QDesktopServices::openUrl(QUrl::fromLocalFile(pluginDir()));
}

QString MainWindow::pluginDir() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/plugins";
}

// Stage 22: eklentileri gecikmeli yükle (başlangıç performansı) + örneği kopyala
void MainWindow::loadPluginsDeferred() {
    if (!m_trusted) return; // Stage 28: güvenilmez alanda eklenti yok
    QDir().mkpath(pluginDir());
    // Stage 29: örnek paketi ilk açılışta kopyala
    for (const QString& s : {"echo.js", "selam.js", "notal.js"}) {
        const QString dst = pluginDir() + "/" + s;
        if (QFile::exists(dst)) continue;
        const QString src =
            QCoreApplication::applicationDirPath() + "/resources/plugins/" + s;
        if (QFile::exists(src)) QFile::copy(src, dst);
    }
    if (!m_plugins) {
        m_plugins = new PluginEngine(this);
        connect(m_plugins, &PluginEngine::commandRegistered, this,
                [this](const QString& id, const QString& title) {
                    m_pluginCmds[id] = title;
                });
        connect(m_plugins, &PluginEngine::pluginLog, this,
                [this](const QString& id, const QString& msg) {
                    m_status->setText("[" + id + "] " + msg.left(80));
                });
        connect(m_plugins, &PluginEngine::permissionDenied, this,
                [this](const QString& id, const QString& perm) {
                    toast(2, "Eklenti izni reddedildi: " + id + " (" + perm + ")");
                });
        // Stage 29 v2 katkıları
        connect(m_plugins, &PluginEngine::viewRegistered, this,
                [this](const QString& id, const QString& title) {
                    m_pluginCmds[id] = title;
                    m_pluginViews[id] = title;
                });
        connect(m_plugins, &PluginEngine::themeRegistered, this,
                &MainWindow::installPluginTheme);
        connect(m_plugins, &PluginEngine::keybindingRegistered, this,
                &MainWindow::installPluginKeybinding);
        connect(m_plugins, &PluginEngine::execRequested, this,
                [this](const QString& pid, const QString& cmd) {
                    m_status->setText("[" + pid + "] → " + cmd);
                    runCommand(cmd);
                });
        connect(m_plugins, &PluginEngine::statusRequested, this,
                [this](const QString&, const QString& t, int ms) {
                    m_status->setText(t);
                    if (ms > 0) {
                        QTimer::singleShot(ms, this, [this, t]() {
                            if (m_status->text() == t) m_status->setText("");
                        });
                    }
                });
        connect(m_plugins, &PluginEngine::terminalRequested, this,
                [this](const QString&, const QString& t) {
                    m_termDock->setVisible(true);
                    if (m_terminal) m_terminal->sendText(t);
                });
        connect(m_plugins, &PluginEngine::problemsReported, this,
                &MainWindow::showPluginProblems);
        connect(m_plugins, &PluginEngine::pluginQuarantined, this,
                [this](const QString& pid, const QString& reason) {
                    toast(3, "Eklenti karantinaya alındı: " + pid + " (" + reason + ")");
                });
    }
    PerfMonitor::instance().mark("eklenti-tara-önce");
    const auto pls = m_plugins->loadAll(pluginDir());
    PerfMonitor::instance().mark("eklenti-tara-sonra");
    int ok = 0;
    for (const auto& pl : pls)
        if (pl.loaded) ++ok;
    if (ok > 0) toast(1, QString("%1 eklenti yüklendi").arg(ok));
    if (m_plugins) {
        m_plugins->setWorkspaceRoot(m_root);
        // Stage 41: eklentilere açık dosya yolu (okuma yine izin+kapsam ister)
        m_plugins->setCurrentFileProvider([this]() -> QString {
            CodeEditor* e = currentEditor();
            return e ? e->filePath() : QString();
        });
        m_plugins->fireEvent("startup"); // Stage 29
    }
}

// Stage 22: kayıtlı eklenti komutunu çalıştır
void MainWindow::runPluginCommand(const QString& id) {
    if (!m_plugins) return;
    QJSValue r = m_plugins->callCommand(id);
    if (r.isError()) toast(3, "Eklenti hatası: " + r.toString().left(150));
}

// Stage 29: eklenti görünümünü pencerede göster
void MainWindow::showPluginView(const QString& cmdId) {
    if (!m_plugins) return;
    const QString html = m_plugins->renderView(cmdId);
    auto* d = new QDialog(this);
    d->setWindowTitle(m_pluginViews.value(cmdId, "Eklenti görünümü"));
    d->resize(560, 420);
    auto* lay = new QVBoxLayout(d);
    auto* view = new QTextBrowser(d);
    view->setHtml(html.isEmpty() ? "<i>(boş)</i>" : html);
    lay->addWidget(view, 1);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, d);
    connect(box, &QDialogButtonBox::rejected, d, &QDialog::reject);
    lay->addWidget(box);
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->show();
}

// Stage 29: eklenti temasını içe aktar
void MainWindow::installPluginTheme(const QString& pid, const QString& name,
                                    const QString& json) {
    Q_UNUSED(pid);
    QTemporaryFile tf;
    tf.setAutoRemove(false);
    if (!tf.open()) {
        toast(3, "Tema yazılamadı");
        return;
    }
    tf.write(json.toUtf8());
    const QString path = tf.fileName();
    tf.close();
    QString key;
    if (ThemeStore::instance().importTheme(path, &key)) {
        QFile::remove(path);
        toast(1, "Tema yüklendi: " + key + " (galeriden uygulayın)");
    } else {
        QFile::remove(path);
        toast(2, "Geçersiz tema JSON: " + name.left(40));
    }
}

// Stage 29: eklenti tuş katkısı (çakışma denetimli)
void MainWindow::installPluginKeybinding(const QString& pid, const QString& cmdId,
                                         const QString& keys) {
    Q_UNUSED(pid);
    AppSettings s = SettingsManager::instance().load();
    QMap<QString, QString> m = s.shortcuts;
    m[cmdId] = keys;
    const QStringList bad = ShortcutCheck::findConflicts(m);
    bool clash = false;
    for (const QString& b : bad)
        if (b.startsWith(keys + ":")) clash = true;
    if (clash) {
        toast(2, "Tuş çakışması, atanmadı: " + keys);
        return;
    }
    s.shortcuts[cmdId] = keys;
    SettingsManager::instance().save(s);
    applyShortcuts();
    toast(1, "Tuş atandı: " + cmdId + " → " + keys);
}

// Stage 29: eklenti sorunlarını Sorunlar paneline işle
void MainWindow::showPluginProblems(const QString& pid, const QString& json) {
    const QJsonDocument d = QJsonDocument::fromJson(json.toUtf8());
    const QJsonArray arr = d.isArray() ? d.array() : d.object().value("problems").toArray();
    if (arr.isEmpty()) return;
    QMap<QString, QList<LspDiag>> byFile;
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        const QString file = o.value("file").toString();
        if (file.isEmpty()) continue;
        LspDiag dg;
        dg.path = file;
        dg.line = qMax(0, o.value("line").toInt(1) - 1);
        dg.col = 0;
        dg.severity = 2;
        dg.message = o.value("message").toString().left(300);
        dg.source = "eklenti:" + pid;
        byFile[file] << dg;
    }
    for (auto it = byFile.constBegin(); it != byFile.constEnd(); ++it) {
        QList<LspDiag> merged;
        for (const LspDiag& old : m_diags.value(it.key()))
            if (old.source != "eklenti:" + pid) merged << old;
        merged << it.value();
        onDiagnostics(it.key(), merged);
    }
    toast(1, "Eklenti sorunları işlendi: " + pid);
}

// Stage 29: eklenti yöneticisi
void MainWindow::showPluginManager() {
    if (!m_plugins) {
        toast(0, "Eklenti motoru henüz hazır değil");
        return;
    }
    auto* d = new PluginManagerDialog(m_plugins, this);
    d->setAttribute(Qt::WA_DeleteOnClose);
    connect(d, &QDialog::finished, this, [this]() {
        // Yönetici değişiklikleri palete yansısın
        m_pluginCmds.clear();
        m_pluginViews.clear();
        if (m_plugins) {
            for (const auto& p : m_plugins->plugins())
                for (const QString& c : p.commands) m_pluginCmds[c] = c;
        }
    });
    d->show();
}

// Stage 22: fabrika ayarları — önce JSON yedeği, sonra sıfırla
void MainWindow::factoryReset() {
    auto r = QMessageBox::question(this, "Fabrika Ayarlarına Dön",
        "Tüm ayarlar sıfırlanacak. Önce JSON yedeği alınsın mı?",
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if (r == QMessageBox::Cancel) return;
    if (r == QMessageBox::Yes) {
        QSettings q("Verso", "VersoCoder");
        const QString bak = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
            + "/ayar-yedegi.json";
        QString err;
        if (!SettingsIO::writeFile(bak, q, &err)) {
            toast(3, "Yedek alınamadı: " + err);
            return;
        }
        toast(1, "Yedek alındı: " + bak);
    }
    QSettings("Verso", "VersoCoder").clear();
    QSettings("Verso", "VersoCoderSessions").clear();
    toast(1, "Ayarlar sıfırlandı — yeniden başlatın");
}

// Stage 25: makro kaydedici (komut dizisi)
void MainWindow::toggleMacroRecord() {
    if (m_macro.recording()) {
        m_macro.stop();
        m_macro.save();
        toast(1, QString("Makro kaydedildi (%1 adım)").arg(m_macro.commands().size()));
    } else {
        m_macro.start();
        toast(0, "Makro kaydı başladı — komutları çalıştırın, bitince tekrar seçin");
    }
}

void MainWindow::playMacro() {
    const QStringList cmds = m_macro.commands();
    if (cmds.isEmpty()) { toast(0, "Kayıtlı makro yok"); return; }
    m_macroPlaying = true;
    for (const QString& c : cmds) runCommand(c);
    m_macroPlaying = false;
    toast(1, QString("Makro oynatıldı (%1 adım)").arg(cmds.size()));
}

void MainWindow::showDiagnostics() {
    DiagnosticsDialog d(this);
    d.setBackupManager(m_backups);
    d.setPluginEngine(m_plugins); // Stage 29
    connect(&d, &DiagnosticsDialog::backupRestored, this, [this](const QString& path) {
        for (CodeEditor* e : allEditors())
            if (QFileInfo(e->filePath()) == QFileInfo(path)) {
                QFile f(path);
                if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QSignalBlocker b(e);
                    e->setPlainText(QString::fromUtf8(f.readAll()));
                    e->document()->setModified(false);
                }
            }
        m_status->setText("Yedek geri yüklendi: " + QFileInfo(path).fileName());
    });
    d.exec();
}

void MainWindow::backupEditor(CodeEditor* e) {
    if (!m_backups || !e || e->filePath().isEmpty()) return;
    QFile f(e->filePath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    m_backups->save(e->filePath(), QString::fromUtf8(f.readAll()));
    m_backups->prune(30);
    m_backups->pruneByQuota(qint64(50) * 1024 * 1024, 90); // Stage 31: 50MB/90gün
}

void MainWindow::applyStartupOptions(const StartupOptions& opts) {
    if (!opts.paths.isEmpty()) {
        for (const QString& p : opts.paths) {
            QFileInfo fi(p);
            if (!fi.exists()) {
                m_status->setText("Bulunamadı: " + p);
                continue;
            }
            if (fi.isDir()) {
                m_root = fi.absoluteFilePath();
                refreshProjectViews();
                AppSettings cur = SettingsManager::instance().load();
                cur.lastRoot = m_root;
                SettingsManager::instance().save(cur);
            } else {
                openFileAt(fi.absoluteFilePath(), opts.line > 0 ? opts.line : 1);
            }
        }
    }
    if (!opts.command.isEmpty()) runCommand(opts.command);
    PerfMonitor::instance().mark("CLI argümanları uygulandı");
}

// --- Stage 9: görünüm / tasarım sistemi ---
QLabel* MainWindow::makeChip(const QString& toolTip, const QString& cmd) {
    auto* l = new QLabel(this);
    l->setObjectName("chip");
    l->setToolTip(toolTip);
    l->setProperty("cmd", cmd);
    // Stage 47: tıklanabilir çipler ekran okuyucuya adını söyler
    l->setAccessibleName(toolTip);
    l->setAccessibleDescription(cmd.isEmpty() ? QString("durum göstergesi")
                                              : QString("etkinleştir: ") + cmd);
    l->installEventFilter(this);
    return l;
}

bool MainWindow::eventFilter(QObject* o, QEvent* e) {
    if (e->type() == QEvent::MouseButtonPress)
        if (auto* l = qobject_cast<QLabel*>(o)) {
            const QString cmd = l->property("cmd").toString();
            if (cmd == QLatin1String("ai.provider")) { showProviderMenu(); return true; }
            if (!cmd.isEmpty()) { runCommand(cmd); return true; }
        }
    // Stage 26: hata ayıklamada değişken hover balonu
    if (e->type() == QEvent::ToolTip) {
        for (CodeEditor* ed : allEditors()) {
            if (o == ed->viewport() && m_gdb && m_gdb->isDebugging() && m_hasFrame) {
                QTextCursor tc = ed->cursorForPosition(
                    static_cast<QHelpEvent*>(e)->pos());
                tc.select(QTextCursor::WordUnderCursor);
                const QString w = tc.selectedText().trimmed();
                if (!w.isEmpty() && w[0].isLetter()) {
                    m_hoverWord = w;
                    m_hoverPos = static_cast<QHelpEvent*>(e)->globalPos();
                    m_gdb->evaluate(w, [this, w](QString v) {
                        if (w == m_hoverWord && !v.isEmpty())
                            QToolTip::showText(m_hoverPos, w + " = " + v.left(300));
                    });
                    return true;
                }
            }
        }
    }
    // Stage 21: sekme çubuğu — orta-tık kapat, çift-tık sabitle
    if (auto* bar = qobject_cast<QTabBar*>(o)) {
        QTabWidget* tabs = (bar == m_tabs->tabBar()) ? m_tabs
                           : (bar == m_tabs2->tabBar() ? m_tabs2 : nullptr);
        if (tabs) {
            if (e->type() == QEvent::MouseButtonPress) {
                auto* m = static_cast<QMouseEvent*>(e);
                if (m->button() == Qt::MiddleButton) {
                    const int i = bar->tabAt(m->pos());
                    if (i >= 0) closeTabIn(tabs, i);
                    return true;
                }
            } else if (e->type() == QEvent::MouseButtonDblClick) {
                auto* m = static_cast<QMouseEvent*>(e);
                const int i = bar->tabAt(m->pos());
                if (i >= 0) togglePinTab(tabs, i);
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(o, e);
}

// Stage 21: sekme sabitleme (tabData="pinned", başlıkta ◆)
bool MainWindow::isPinnedTab(QTabWidget* tabs, int i) const {
    return tabs->tabBar()->tabData(i).toString() == "pinned";
}

void MainWindow::togglePinTab(QTabWidget* tabs, int i) {
    tabs->tabBar()->setTabData(i, isPinnedTab(tabs, i) ? QVariant() : "pinned");
    updateTabMarks();
    toast(0, isPinnedTab(tabs, i) ? "Sekme sabitlendi (orta-tıkla kapanmaz)"
                                  : "Sekme sabiti kaldırıldı");
}

void MainWindow::showSidePanel(int index) {
    m_side->setCurrentIndex(index);
    m_btnExplorer->setChecked(index == 0);
    m_btnSearch->setChecked(index == 1);
    m_btnGit->setChecked(index == 2);
    m_btnAi->setChecked(index == 3);
    m_btnProblems->setChecked(index == 4);
    if (m_btnRemote) m_btnRemote->setChecked(index == 5);
    if (m_btnOutline) m_btnOutline->setChecked(index == 6);
    m_sideTitle->setText(sideTitleFor(index));
    refreshActivityIcons();
}

void MainWindow::refreshActivityIcons() {
    const ThemeTokens tk = ThemeManager::instance().tokens();
    struct { QPushButton* b; const char* icon; } btns[] = {
        {m_btnExplorer, "explorer"}, {m_btnSearch, "search"}, {m_btnGit, "git"},
        {m_btnAi, "ai"}, {m_btnProblems, "problems"}, {m_btnSettings, "settings"}};
    for (auto& x : btns) {
        if (!x.b) continue;
        const QColor c = x.b->isChecked() ? tk.textStrong : tk.textDim;
        x.b->setIcon(IconTheme::icon(x.icon, c, 20));
    }
}

void MainWindow::refreshGitBranch() {
    if (!m_chipGit) return;
    auto* p = new QProcess(this);
    p->setWorkingDirectory(m_root);
    // Stage 46: QPointer koruması — yıkım sırasında biten süreç asılı
    // etikete yazıyordu (m_chipGit ham gösterge, null değil dangling olur).
    QPointer<QLabel> chip = m_chipGit;
    connect(p, &QProcess::finished, this, [this, p, chip](int code, QProcess::ExitStatus) {
        p->deleteLater();
        if (!chip) return;
        QLabel* m_chipGit = chip;
        if (code != 0) {
            m_chipGit->setText("—");
            m_chipGit->setToolTip("Git deposu değil");
            return;
        }
        const QString b = QString::fromUtf8(p->readAllStandardOutput()).trimmed();
        m_chipGit->setText("⑂ " + (b.isEmpty() ? QString("detached") : b));
    });
    p->start("git", {"rev-parse", "--abbrev-ref", "HEAD"});
}

void MainWindow::updateTabMarks() {
    for (QTabWidget* t : {m_tabs, m_tabs2}) {
        if (!t) continue;
        for (int i = 0; i < t->count(); ++i) {
            auto* e = qobject_cast<CodeEditor*>(t->widget(i));
            if (!e) continue;
            QString base = t->tabText(i);
            if (base.startsWith("◆ ")) base = base.mid(2);
            if (base.startsWith("● ")) base = base.mid(2);
            QString pre;
            if (isPinnedTab(t, i)) pre += "◆ ";
            if (e->document()->isModified()) pre += "● ";
            t->setTabText(i, pre + base);
        }
    }
}

void MainWindow::openThemeGallery() {
    AppSettings s = SettingsManager::instance().load();
    ThemeGalleryDialog d(s.theme, s.accentColor.isEmpty() ? QColor() : QColor(s.accentColor), this);
    connect(&d, &ThemeGalleryDialog::themeSelected, this, [this](const QString& name) {
        AppSettings cur = SettingsManager::instance().load();
        cur.theme = name;
        SettingsManager::instance().save(cur);
        ThemeManager::instance().apply(name);
        toast(1, "Tema: " + name); // Stage 10
    });
    connect(&d, &ThemeGalleryDialog::accentSelected, this, [this](const QColor& c) {
        AppSettings cur = SettingsManager::instance().load();
        cur.accentColor = c.isValid() ? c.name() : QString();
        SettingsManager::instance().save(cur);
        ThemeManager::instance().setAccent(c);
        toast(1, "Vurgu rengi: " + (c.isValid() ? c.name() : QString("tema varsayılanı"))); // Stage 10
    });
    d.exec();
}

// --- Stage 6: git diff görüntüleme ---
void MainWindow::diffPath(const QString& path) {
    QProcess p(this);
    p.setWorkingDirectory(m_root);
    p.start("git", {"rev-parse", "--show-toplevel"});
    p.waitForFinished(3000);
    QString top = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    if (p.exitCode() != 0 || top.isEmpty()) {
        m_status->setText("Git deposu bulunamadı.");
        return;
    }
    QString rel = QDir(top).relativeFilePath(path);
    p.start("git", {"diff", "--no-color", "--no-ext-diff", "--", rel});
    p.waitForFinished(8000);
    QString diff = QString::fromUtf8(p.readAllStandardOutput());
    if (diff.trimmed().isEmpty()) {
        // staged'a karşı da dene, yoksa HEAD'e göre
        p.start("git", {"diff", "--no-color", "--cached", "--", rel});
        p.waitForFinished(8000);
        diff = QString::fromUtf8(p.readAllStandardOutput());
    }
    if (diff.trimmed().isEmpty()) {
        m_status->setText("Fark yok: " + rel);
        return;
    }
    DiffDialog d(this);
    d.setRepoDiff(top, diff, diff.contains("+++ b/"));
    d.exec();
    m_git->refresh();
}

void MainWindow::diffExternal(const QString& fileA, const QString& fileB, const QString& title) {
    QProcess p(this);
    p.start("git", {"diff", "--no-color", "--no-index", "--", fileA, fileB});
    p.waitForFinished(15000);
    QString diff = QString::fromUtf8(p.readAllStandardOutput());
    if (diff.trimmed().isEmpty()) {
        m_status->setText("Fark yok.");
        return;
    }
    DiffDialog d(this);
    d.setExternalDiff(diff, title);
    d.exec();
}

void MainWindow::onPathRenamed(const QString& oldPath, const QString& newPath) {
    for (CodeEditor* e : allEditors()) {
        QString p = e->filePath();
        if (p == oldPath) {
            e->setFilePath(newPath);
            // sekme metnini güncelle
            for (QTabWidget* t : {m_tabs, m_tabs2})
                for (int i = 0; i < t->count(); ++i)
                    if (t->widget(i) == e) {
                        t->setTabText(i, QFileInfo(newPath).fileName());
                        t->setTabToolTip(i, newPath);
                    }
            m_diags.remove(oldPath);
            m_spellDiags.remove(oldPath);
            refreshProblemView();
            teardownLsp(oldPath);
            if (!newPath.isEmpty()) setupLspFor(newPath, e->toPlainText());
        } else if (p.startsWith(oldPath + "/")) {
            e->setFilePath(newPath + p.mid(oldPath.length()));
        }
    }
    saveSession();
}

void MainWindow::openSettings() {
    SettingsDialog d(this);
    connect(&d, &SettingsDialog::applied, this, &MainWindow::applySettings);
    connect(&d, &SettingsDialog::themeGalleryRequested, this, &MainWindow::openThemeGallery);
    d.exec();
}

void MainWindow::applySettings() {
    AppSettings s = SettingsManager::instance().load();
    LanguageManager::instance().setLanguage(s.language);
    // Stage 9: tipografi + accent + tema (Stage 12: yoğunluk font farkı eklenir)
    TypographySettings typo;
    typo.uiFamily = s.uiFontFamily;
    typo.editorFamily = s.editorFontFamily; // Stage 23
    typo.uiSize = qBound(8, s.uiFontSize + Density::fontDelta(Density::fromName(s.density)), 22);
    typo.lineHeight = s.lineHeight;
    typo.letterSpacing = s.letterSpacing;
    typo.ligatures = s.ligatures;
    ThemeManager::instance().setTypography(typo);
    ThemeManager::instance().setSelectionOpacity(s.selectionOpacity); // Stage 23
    ThemeManager::instance().setAccent(s.accentColor.isEmpty() ? QColor() : QColor(s.accentColor));
    ThemeManager::instance().setVisionMode(s.colorVision); // Stage 12 (apply öncesi)
    ThemeManager::instance().apply(s.theme);
    m_ai->reloadSettings();
    applyEditorSettingsToAll();
    applyShortcuts();
    m_terminal->setShell(s.terminalShell);
    if (m_autosave) m_autosave->setInterval(s.autoSaveIntervalMs);
    if (!s.lspEnabled) {
        m_lspCpp->stop();
        m_lspPy->stop();
        m_diags.clear();
        m_spellDiags.clear();
        refreshProblemView();
        if (auto* e = currentEditor()) e->clearDiagnostics();
    } else if (auto* e = currentEditor()) {
        if (!e->filePath().isEmpty()) setupLspFor(e->filePath(), e->toPlainText());
    }
    retranslate();
    updateCursorStatus();
    updateBreadcrumb();
    // Stage 10: animasyon tercihi + yerleşim ön ayarı uygulanmış kabul et
    Animator::setReducedMotion(s.reducedMotion);
    // Stage 12: yoğunluk + görme modu + çip/panel + başlık + otomatik tema
    const Density::Level dl = Density::fromName(s.density);
    UiMetrics::setScale(Density::metricsScale(dl));
    ThemeManager::instance().setVisionMode(s.colorVision);
    applyChips();
    applySidePages();
    setCustomTitleBar(s.customTitleBar);
    evaluateAutoTheme();
    updateProfileChip();
    toast(1, "Ayarlar uygulandı");
}

void MainWindow::retranslate() {
    m_sideTitle->setText(sideTitleFor(m_side->currentIndex()));
    m_btnSettings->setToolTip(LanguageManager::instance().t("settings"));
    m_status->setText(LanguageManager::instance().t("statusbar_ready"));
}

// ---------- Stage 10: mikro-etkileşim & cila ----------

void MainWindow::toast(int type, const QString& msg) {
    ToastType t = (type == 1) ? ToastType::Success
                : (type == 2) ? ToastType::Warning
                : (type == 3) ? ToastType::Error
                : ToastType::Info;
    ToastManager::instance().show(t, msg);
}

void MainWindow::applyLayoutPreset(const QString& name) {
    if (!LayoutPresets::isValid(name)) return;
    const LayoutState st = LayoutPresets::preset(name);
    m_activity->setVisible(st.activityBar);
    m_sideWrap->setVisible(st.sidePanel);
    statusBar()->setVisible(st.statusBar);
    m_termDock->setVisible(st.bottomPanel);
    if (m_actMinimap->isChecked() != st.minimap) m_actMinimap->toggle();
    if (m_actCrumb && m_actCrumb->isChecked() != st.breadcrumb) m_actCrumb->toggle();
    menuBar()->setVisible(st.menuBar);

    AppSettings s = SettingsManager::instance().load();
    if (s.layoutPreset != name) {
        s.layoutPreset = name;
        SettingsManager::instance().save(s);
    }
}

void MainWindow::toggleZen() {
    AppSettings s = SettingsManager::instance().load();
    if (s.layoutPreset == "zen") {
        applyLayoutPreset(m_lastLayout);
        toast(0, "Zen modu kapalı — " + LayoutPresets::presetTitle(m_lastLayout));
    } else {
        m_lastLayout = s.layoutPreset;
        applyLayoutPreset("zen");
        toast(0, "Zen modu açık (F11 ile kapat)");
    }
}

void MainWindow::zoom(int delta) {
    AppSettings s = SettingsManager::instance().load();
    if (delta == 0) {
        s.uiFontSize = 13;
        s.fontSize = 11;
    } else {
        s.uiFontSize = qBound(9, s.uiFontSize + delta, 20);
        s.fontSize = qBound(8, s.fontSize + delta, 24);
    }
    SettingsManager::instance().save(s);
    applySettings();
    toast(0, QString("Yakınlaştırma: arayüz %1 px • editör %2 px")
                  .arg(QString::number(s.uiFontSize), QString::number(s.fontSize)));
}

void MainWindow::exportUiProfile() {
    AppSettings s = SettingsManager::instance().load();
    QString suggested = QDir::homePath() + "/verso-gorunum-profili.json";
    QString path = QFileDialog::getSaveFileName(this, "Görünüm Profilini Dışa Aktar",
                                                suggested, "JSON (*.json)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        toast(3, "Profil yazılamadı: " + path);
        return;
    }
    f.write(UiProfile::exportJson(s).toUtf8());
    f.close();
    toast(1, "Görünüm profili dışa aktarıldı: " + QFileInfo(path).fileName());
}

void MainWindow::importUiProfile() {
    QString path = QFileDialog::getOpenFileName(this, "Görünüm Profili İçe Aktar",
                                                QDir::homePath(), "JSON (*.json)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        toast(3, "Profil okunamadı: " + path);
        return;
    }
    const QString json = QString::fromUtf8(f.readAll());
    f.close();
    AppSettings s = SettingsManager::instance().load();
    if (!UiProfile::applyJson(s, json)) {
        toast(3, "Geçersiz profil dosyası (verso-ui-profile değil)");
        return;
    }
    SettingsManager::instance().save(s);
    if (LayoutPresets::isValid(s.layoutPreset)) applyLayoutPreset(s.layoutPreset);
    applySettings();
    toast(1, "Görünüm profili içe aktarıldı");
}

void MainWindow::ensureWelcome(QTabWidget* tabs) {
    if (!tabs || tabs->count() > 0) return;
    WelcomeView*& w = m_welcomeViews[tabs];
    if (!w) {
        w = new WelcomeView(this);
        connect(w, &WelcomeView::commandRequested, this, &MainWindow::runCommand);
        connect(w, &WelcomeView::fileRequested, this, [this](const QString& p) {
            openEditorFor(p);
        });
    // Stage 28: son projeler
    connect(w, &WelcomeView::projectRequested, this, [this](const QString& d) {
        openRoot(d);
    });
    }
    // Son dosyalar: her iki oturum listesinden, var olanlardan
    AppSettings s = SettingsManager::instance().load();
    QStringList recents;
    if (s.restoreSession) {
        recents << s.sessionFiles << s.sessionFiles2;
    }
    w->setRecentFiles(recents);
    w->setRecentProjects(QSettings("Verso", "VersoCoder")
                             .value("session/recentRoots")
                             .toStringList()); // Stage 28
    tabs->addTab(w, "Hoş geldin");
    tabs->setTabToolTip(0, "Verso Coder");
}

void MainWindow::removeWelcome(QTabWidget* tabs) {
    if (!tabs) return;
    for (int i = 0; i < tabs->count(); ++i)
        if (qobject_cast<WelcomeView*>(tabs->widget(i))) {
            tabs->removeTab(i);
            return;
        }
}

// ---------- Stage 11: editör-içi bulma + git gutter ----------

void MainWindow::showFindBar() {
    auto* e = currentEditor();
    if (!e || !m_findBar) return;
    // Seçili metni sorgu yap
    QTextCursor c = e->textCursor();
    if (c.hasSelection()) {
        const QString sel = c.selectedText().replace(QChar(0x2029), "\n").left(120);
        if (!sel.contains('\n')) m_findBar->setNeedle(sel);
    } else if (!m_lastFind.isEmpty()) {
        m_findBar->setNeedle(m_lastFind);
    }
    m_findBar->focusNeedle();
    applyFindToEditor();
}

void MainWindow::applyFindToEditor() {
    auto* e = currentEditor();
    if (!e || !m_findBar || !m_findBar->isVisible()) return;
    const QString needle = m_findBar->needle();
    if (!needle.isEmpty()) m_lastFind = needle;
    if (needle.isEmpty()) {
        e->clearFindHits();
        m_minimap->setSearchMarks({}, -1);
        m_findBar->setCountText("");
        m_findHits.clear();
        m_findCur = -1;
        return;
    }
    m_findHits = SearchMarks::findAll(e->toPlainText(), needle,
                                      m_findBar->caseSensitive(),
                                      m_findBar->wholeWord());
    // İmleçten sonraki ilk isabet aktif olsun
    m_findCur = -1;
    if (!m_findHits.isEmpty()) {
        const int pos = e->textCursor().position();
        m_findCur = 0;
        for (int i = 0; i < m_findHits.size(); ++i)
            if (m_findHits[i].start >= pos) { m_findCur = i; break; }
    }
    e->setFindHits(m_findHits, m_findCur);
    QList<int> lines;
    for (const auto& h : m_findHits) lines << h.line0;
    m_minimap->setSearchMarks(lines, m_findCur >= 0 ? m_findHits[m_findCur].line0 : -1);
    m_findBar->setCountText(m_findHits.isEmpty()
        ? "0 sonuç"
        : QString("%1/%2").arg(m_findCur + 1).arg(m_findHits.size()));
}

void MainWindow::findNavigate(int dir) {
    // Çubuk gizliyse son sorguyla aç
    if (!m_findBar || !m_findBar->isVisible()) {
        showFindBar();
        return;
    }
    auto* e = currentEditor();
    if (!e || m_findHits.isEmpty()) return;
    m_findCur = (m_findCur + dir + m_findHits.size()) % m_findHits.size();
    const FindHit& h = m_findHits[m_findCur];
    QTextCursor c(e->document());
    c.setPosition(h.start);
    c.setPosition(h.end, QTextCursor::KeepAnchor);
    e->setTextCursor(c);
    e->centerCursor();
    e->setFindHits(m_findHits, m_findCur);
    QList<int> lines;
    for (const auto& hh : m_findHits) lines << hh.line0;
    m_minimap->setSearchMarks(lines, h.line0);
    m_findBar->setCountText(QString("%1/%2").arg(m_findCur + 1).arg(m_findHits.size()));
}

void MainWindow::refreshGitMarks(CodeEditor* e) {
    if (!e) return;
    const QString path = e->filePath();
    if (path.isEmpty() || e->previewMode()) {
        e->setGitMarks({});
        return;
    }
    QFileInfo fi(path);
    QProcess p(this);
    p.setWorkingDirectory(fi.absolutePath());
    p.start("git", {"diff", "HEAD", "-U0", "--no-color", "--", fi.fileName()});
    if (!p.waitForFinished(3000) || p.exitCode() != 0) {
        e->setGitMarks({});
        if (e == currentEditor()) m_minimap->setDiffMarks({});
        return;
    }
    const QMap<int, char> marks =
        DiffGutter::changedLines(QString::fromUtf8(p.readAllStandardOutput()));
    e->setGitMarks(marks);
    if (e == currentEditor()) m_minimap->setDiffMarks(marks);
}

// ---------- Stage 12: uyarlanabilir arayüz ----------

void MainWindow::openThemeEditor() {
    ThemeEditorDialog d(ThemeManager::instance().current(), this);
    if (d.exec() == QDialog::Accepted && !d.savedName().isEmpty()) {
        AppSettings s = SettingsManager::instance().load();
        s.theme = d.savedName();
        SettingsManager::instance().save(s);
        ThemeManager::instance().apply(d.savedName());
        toast(1, "Tema kaydedildi ve uygulandı: " + d.savedName());
    } else {
        ThemeManager::instance().clearPreview();
    }
}

void MainWindow::cycleDensity() {
    static const QStringList order = {"compact", "comfortable", "spacious"};
    AppSettings s = SettingsManager::instance().load();
    const int i = order.indexOf(s.density);
    s.density = order[(i + 1) % order.size()]; // bilinmeyense (-1+1)%3=0 → compact
    SettingsManager::instance().save(s);
    applySettings();
    toast(0, "Yoğunluk: " + Density::title(s.density));
}

void MainWindow::toggleFocusMode() {
    AppSettings s = SettingsManager::instance().load();
    s.focusMode = !s.focusMode;
    SettingsManager::instance().save(s);
    applyEditorSettingsToAll();
    toast(0, s.focusMode ? "Odak modu açık (imleç ortalanır)" : "Odak modu kapalı");
}

void MainWindow::toggleTitleBar() {
    AppSettings s = SettingsManager::instance().load();
    s.customTitleBar = !s.customTitleBar;
    SettingsManager::instance().save(s);
    setCustomTitleBar(s.customTitleBar);
    toast(0, s.customTitleBar ? "Özel başlık açık (sorun olursa ayarlardan kapatın)"
                              : "Yerel başlık çubuğuna dönüldü");
}

void MainWindow::setCustomTitleBar(bool on) {
    if (!m_titleBar) return;
    Qt::WindowFlags f = windowFlags();
    if (on) f |= Qt::FramelessWindowHint;
    else f &= ~Qt::FramelessWindowHint;
    if (f != windowFlags()) {
        setWindowFlags(f);
        if (isVisible()) show(); // bayrak değişimi yeniden göstermeyi gerektirir
    }
    m_titleBar->setVisible(on);
}

void MainWindow::exportPng() {
    const QString path = QFileDialog::getSaveFileName(
        this, "Pencereyi PNG Olarak Kaydet",
        QDir::homePath() + "/verso-pencere.png", "PNG (*.png)");
    if (path.isEmpty()) return;
    if (grab().save(path))
        toast(1, "PNG kaydedildi: " + QFileInfo(path).fileName());
    else
        toast(3, "PNG yazılamadı: " + path);
}

void MainWindow::showProfileMenu() {
    QMenu menu(this);
    AppSettings s = SettingsManager::instance().load();
    const QStringList names = ProfileStore::names();
    if (names.isEmpty())
        menu.addAction("(kayıtlı profil yok)")->setEnabled(false);
    for (const QString& n : names) {
        QAction* a = menu.addAction((n == s.activeUiProfile ? "● " : "○ ") + n);
        connect(a, &QAction::triggered, this, [this, n]() { applyProfile(n); });
    }
    menu.addSeparator();
    menu.addAction("Geçerli görünümü kaydet...", [this]() { saveProfileAs(); });
    if (!s.activeUiProfile.isEmpty()) {
        const QString cur = s.activeUiProfile;
        menu.addAction("Etkin profili sil (" + cur + ")", [this, cur]() {
            ProfileStore::remove(cur);
            AppSettings st = SettingsManager::instance().load();
            if (st.activeUiProfile == cur) {
                st.activeUiProfile.clear();
                SettingsManager::instance().save(st);
            }
            updateProfileChip();
            toast(0, "Profil silindi: " + cur);
        });
    }
    menu.exec(QCursor::pos());
}

void MainWindow::saveProfileAs() {
    AppSettings s = SettingsManager::instance().load();
    bool ok = false;
    QString name = QInputDialog::getText(this, "Görünüm Profili", "Profil adı:",
                                         QLineEdit::Normal, s.activeUiProfile, &ok);
    if (!ok) return;
    name = ProfileStore::sanitize(name);
    if (name.isEmpty()) { toast(2, "Geçersiz profil adı."); return; }
    if (!ProfileStore::save(name, UiProfile::exportJson(s))) {
        toast(3, "Profil kaydedilemedi.");
        return;
    }
    s.activeUiProfile = name;
    SettingsManager::instance().save(s);
    updateProfileChip();
    toast(1, "Profil kaydedildi: " + name);
}

void MainWindow::applyProfile(const QString& name) {
    const QString json = ProfileStore::load(name);
    AppSettings s = SettingsManager::instance().load();
    if (json.isEmpty() || !UiProfile::applyJson(s, json)) {
        toast(3, "Profil uygulanamadı: " + name);
        return;
    }
    s.activeUiProfile = name;
    SettingsManager::instance().save(s);
    if (LayoutPresets::isValid(s.layoutPreset)) applyLayoutPreset(s.layoutPreset);
    applySettings();
    toast(1, "Profil uygulandı: " + name);
}

void MainWindow::updateProfileChip() {
    if (!m_chipProfile) return;
    AppSettings s = SettingsManager::instance().load();
    m_chipProfile->setText(s.activeUiProfile.isEmpty() ? "Profil: —"
                                                     : "Profil: " + s.activeUiProfile);
}

void MainWindow::evaluateAutoTheme() {
    AppSettings s = SettingsManager::instance().load();
    if (s.autoThemeMode == "off") return;
    bool systemDark = true;
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    systemDark = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#endif
    const int hour = QTime::currentTime().hour();
    const QString want = AutoTheme::pick(s.autoThemeMode, systemDark, hour,
                                         s.dayTheme, s.nightTheme, s.theme);
    if (want != s.theme && ThemeStore::instance().hasTheme(want)) {
        s.theme = want;
        SettingsManager::instance().save(s);
        ThemeManager::instance().apply(want);
        m_status->setText("Otomatik tema: " + want);
    }
}

void MainWindow::applyChips() {
    AppSettings s = SettingsManager::instance().load();
    if (m_chipGit) m_chipGit->setVisible(s.chipGit);
    if (m_chipProblems) m_chipProblems->setVisible(s.chipProblems);
    if (m_cursorLabel) m_cursorLabel->setVisible(s.chipCursor);
    if (m_chipLang) m_chipLang->setVisible(s.chipLang);
    if (m_chipEol) m_chipEol->setVisible(s.chipEol);
    if (m_chipEnc) m_chipEnc->setVisible(s.chipEnc);
    updateProfileChip();
}

void MainWindow::applySidePages() {
    AppSettings s = SettingsManager::instance().load();
    const QStringList ids = {"explorer", "search", "git", "ai", "problems"};
    QPushButton* btns[5] = {m_btnExplorer, m_btnSearch, m_btnGit, m_btnAi, m_btnProblems};
    for (int i = 0; i < 5; ++i)
        if (btns[i]) btns[i]->setVisible(s.sidePages.contains(ids[i]));
    if (s.sidePages.isEmpty()) {
        if (m_sideWrap) m_sideWrap->setVisible(false);
        return;
    }
    if (m_sideWrap) m_sideWrap->setVisible(true);
    if (!s.sidePages.contains(ids[m_side->currentIndex()])) {
        for (int i = 0; i < 5; ++i)
            if (s.sidePages.contains(ids[i])) { showSidePanel(i); break; }
    }
}

void MainWindow::saveBottomOrder() {
    if (!m_bottomTabs) return;
    QStringList order;
    for (int i = 0; i < m_bottomTabs->count(); ++i)
        order << m_bottomTabs->tabText(i);
    AppSettings s = SettingsManager::instance().load();
    if (s.bottomOrder != order) {
        s.bottomOrder = order;
        SettingsManager::instance().save(s);
    }
}

void MainWindow::applyBottomOrder() {
    AppSettings s = SettingsManager::instance().load();
    if (s.bottomOrder.isEmpty() || !m_bottomTabs) return;
    auto* bar = m_bottomTabs->tabBar();
    bar->blockSignals(true);
    for (int target = 0; target < s.bottomOrder.size(); ++target) {
        for (int i = target; i < m_bottomTabs->count(); ++i) {
            if (m_bottomTabs->tabText(i) == s.bottomOrder[target]) {
                bar->moveTab(i, target);
                break;
            }
        }
    }
    bar->blockSignals(false);
}

// ---------- Stage 13: dil zekâsı ----------

void MainWindow::withLspReady(LspClient* c, std::function<void()> fn) {
    if (!c) return;
    if (c->isReady()) { fn(); return; }
    m_status->setText("LSP hazırlanıyor — hazır olunca çalışacak...");
    connect(c, &LspClient::started, this, [fn]() { fn(); }, Qt::SingleShotConnection);
}

static QString lspWordUnderCursor(CodeEditor* e) {
    QTextCursor c = e->textCursor();
    if (c.hasSelection()) return c.selectedText().left(80);
    QTextCursor w = c;
    w.select(QTextCursor::WordUnderCursor);
    return w.selectedText().left(80);
}

void MainWindow::requestCompletion(bool autoTrigger) {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    AppSettings s = SettingsManager::instance().load();
    if (autoTrigger && !s.autoComplete) return;
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c) {
        if (!autoTrigger) m_status->setText("Bu dil için LSP yok.");
        return;
    }
    if (!c->hasCap("completionProvider")) {
        if (!autoTrigger) m_status->setText("Sunucu tamamlama desteklemiyor.");
        return;
    }
    if (!c->isReady()) {
        if (!autoTrigger) withLspReady(c, [this]() { requestCompletion(false); });
        else withLspReady(c, [this]() { requestCompletion(true); });
        return;
    }
    QTextCursor cur = e->textCursor();
    const QString prefix = e->completionPrefix();
    if (autoTrigger && prefix.size() < 2) return; // gürültüyü azalt
    pushDocToLsp(e);
    if (m_pendingCompleteId >= 0) c->cancelRequest(m_pendingCompleteId);
    m_pendingCompleteId = c->request("textDocument/completion",
        QJsonObject{{"textDocument", QJsonObject{{"uri", LspClient::pathToUri(e->filePath())}}},
                    {"position", QJsonObject{{"line", cur.blockNumber()},
                                             {"character", cur.columnNumber()}}},
                    {"context", QJsonObject{{"triggerKind", autoTrigger ? 2 : 1}}}},
        [this, e, prefix, autoTrigger](QJsonObject res) {
            m_pendingCompleteId = -1;
            if (e != currentEditor()) return;
            QList<CompletionItem> items = CompletionList::parse(res);
            items = CompletionList::filter(items, autoTrigger ? prefix : e->completionPrefix());
            if (items.isEmpty()) {
                if (!autoTrigger) m_status->setText("Öneri yok.");
                return;
            }
            QRect r = e->cursorRect(e->textCursor());
            m_completion->showItems(items, e->mapToGlobal(r.bottomRight()));
        });
}

void MainWindow::onCompletionChosen(const CompletionItem& item) {
    auto* e = currentEditor();
    if (!e || item.label.isEmpty()) return;
    // Öneki sil + insertText yaz
    QTextCursor c = e->textCursor();
    const QString prefix = e->completionPrefix();
    if (!prefix.isEmpty()) {
        c.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, prefix.size());
        c.removeSelectedText();
    }
    c.insertText(item.insertText.isEmpty() ? item.label : item.insertText);
    e->setTextCursor(c);
    pushDocToLsp(e);
}

void MainWindow::showReferences() {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c) { m_status->setText("Bu dil için LSP yok."); return; }
    const QString sym = lspWordUnderCursor(e);
    if (sym.isEmpty()) { m_status->setText("İmleçte sembol yok."); return; }
    if (!c->isReady()) { withLspReady(c, [this]() { showReferences(); }); return; }
    QTextCursor cur = e->textCursor();
    m_status->setText("Referanslar aranıyor: " + sym);
    c->requestReferences(e->filePath(), cur.blockNumber(), cur.columnNumber(),
        [this, sym](QJsonObject res) {
            QList<LspLocation> locs = LocationSet::parse(res);
            if (locs.isEmpty()) { m_status->setText("Referans bulunamadı."); return; }
            ReferencesDialog d(sym, locs, this);
            if (d.exec() == QDialog::Accepted && !d.selected().path.isEmpty()) {
                const LspLocation l = d.selected();
                openFileAt(l.path, l.line + 1);
                if (auto* ne = currentEditor()) ne->flashLine(l.line);
            }
        });
}

void MainWindow::renameSymbol() {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c) { m_status->setText("Bu dil için LSP yok."); return; }
    const QString sym = lspWordUnderCursor(e);
    if (sym.isEmpty()) { m_status->setText("İmleçte sembol yok."); return; }
    if (!c->isReady()) { withLspReady(c, [this]() { renameSymbol(); }); return; }
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Yeniden Adlandır",
        QString("Yeni ad (%1):").arg(sym), QLineEdit::Normal, sym, &ok);
    if (!ok || name.trimmed().isEmpty() || name == sym) return;
    QTextCursor cur = e->textCursor();
    const QString path = e->filePath();
    const QString oldText = e->toPlainText();
    c->requestRename(path, cur.blockNumber(), cur.columnNumber(), name.trimmed(),
        [this, path, oldText, sym, name](QJsonObject res) {
            QList<LspTextEdit> edits = TextEdits::parseWorkspace(res);
            if (edits.isEmpty()) edits = TextEdits::parse(res);
            if (edits.isEmpty()) { m_status->setText("Yeniden adlandırma desteklenmiyor."); return; }
            // Dosyalar arası mı?
            bool multi = false;
            for (const LspTextEdit& ed : edits)
                if (!ed.file.isEmpty() && ed.file != path) { multi = true; break; }
            if (multi) {
                // Stage 24: proje geneli önizleme + dosya dışlama
                QMap<QString, QList<LspTextEdit>> byFile;
                for (const LspTextEdit& ed : edits)
                    byFile[ed.file.isEmpty() ? path : ed.file] << ed;
                RenamePreviewDialog md(sym, edits, oldText, this);
                md.setFileList(byFile.keys());
                if (md.exec() != QDialog::Accepted) return;
                const QSet<QString> excl(md.excludedFiles().begin(),
                                         md.excludedFiles().end());
                QList<LspTextEdit> sim = edits;
                for (LspTextEdit& ed : sim) ed.newText = md.newName();
                QMap<QString, QList<LspTextEdit>> keep;
                for (const LspTextEdit& ed : sim) {
                    const QString f = ed.file.isEmpty() ? path : ed.file;
                    if (!excl.contains(f)) keep[f] << ed;
                }
                int n = 0;
                for (auto it = keep.begin(); it != keep.end(); ++it) {
                    if (it.key() == path) {
                        if (auto* ce = currentEditor()) {
                            ce->setPlainText(TextEdits::apply(oldText, it.value()));
                            pushDocToLsp(ce);
                        }
                    } else {
                        QFile f(it.key());
                        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                            QString t = QString::fromUtf8(f.readAll());
                            f.close();
                            if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                                f.write(TextEdits::apply(t, it.value()).toUtf8());
                                f.close();
                            }
                        }
                    }
                    n += it.value().size();
                }
                toast(1, QString("%1 yerde yeniden adlandırıldı").arg(n));
                return;
            }
            RenamePreviewDialog d(sym, edits, oldText, this);
            if (d.exec() == QDialog::Accepted) {
                QList<LspTextEdit> sim = edits;
                for (LspTextEdit& ed : sim) ed.newText = d.newName();
                if (auto* ce = currentEditor())
                    if (ce->filePath() == path) {
                        ce->setPlainText(TextEdits::apply(oldText, sim));
                        pushDocToLsp(ce);
                        toast(1, QString("%1 yerde uygulandı").arg(sim.size()));
                    }
            }
        });
}

void MainWindow::showCodeActions() {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c) { m_status->setText("Bu dil için LSP yok."); return; }
    if (!c->isReady()) { withLspReady(c, [this]() { showCodeActions(); }); return; }
    QTextCursor cur = e->textCursor();
    QTextCursor c1(e->document());
    c1.setPosition(cur.selectionStart());
    QTextCursor c2(e->document());
    c2.setPosition(cur.selectionEnd());
    const int sln = c1.blockNumber(), slc = c1.columnNumber();
    const int eln = c2.blockNumber(), elc = c2.columnNumber();
    c->requestCodeAction(e->filePath(), sln, slc, eln, elc, {},
        [this, e](QJsonObject res) {
            QList<CodeActionItem> items = CodeActionList::parse(res);
            if (items.isEmpty()) { m_status->setText("Önerilen düzeltme yok."); return; }
            QMenu menu(this);
            for (const CodeActionItem& it : items) {
                QString label = it.title;
                if (!it.kind.isEmpty()) label += QString("  [%1]").arg(it.kind);
                QAction* act = menu.addAction(label);
                connect(act, &QAction::triggered, this, [this, e, it]() {
                    applyCodeAction(e, it);
                });
            }
            menu.exec(QCursor::pos());
        });
}

// Yardımcı: CodeAction uygula (edit → metne, command → sunucuya ilet)
void MainWindow::applyCodeAction(CodeEditor* e, const CodeActionItem& it) {
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c) return;
    const QJsonObject o = it.raw;
    if (o.contains("edit") && o["edit"].isObject()) {
        QList<LspTextEdit> edits = TextEdits::parseWorkspace(
            QJsonObject{{"result", o["edit"].toObject()}});
        if (edits.isEmpty()) edits = TextEdits::parse(
            QJsonObject{{"result", o["edit"].toObject()["changes"]}});
        // changes doğrudan uri-> [] ise:
        if (edits.isEmpty() && o["edit"].toObject().contains("changes")) {
            QJsonObject ch = o["edit"].toObject()["changes"].toObject();
            for (auto k = ch.begin(); k != ch.end(); ++k)
                edits << TextEdits::parseArray(k.value().toArray(), LspClient::uriToPath(k.key()));
        }
        if (!edits.isEmpty()) {
            // Yalnızca geçerli dosya desteklenir (çok dosyalı rename yolunu kullanır)
            QList<LspTextEdit> mine;
            for (const LspTextEdit& ed : edits)
                if (ed.file.isEmpty() || ed.file == e->filePath()) mine << ed;
            if (!mine.isEmpty()) {
                e->setPlainText(TextEdits::apply(e->toPlainText(), mine));
                pushDocToLsp(e);
                toast(1, "Düzeltme uygulandı: " + it.title);
                return;
            }
        }
    }
    if (o.contains("command")) {
        QJsonObject cmd = o["command"].isObject() ? o["command"].toObject() : o;
        QJsonArray args = cmd["arguments"].toArray();
        c->request("workspace/executeCommand",
                   QJsonObject{{"command", cmd["command"].toString()}, {"arguments", args}},
                   [this, e](QJsonObject) {
                       pushDocToLsp(e);
                       toast(1, "Komut çalıştırıldı.");
                   });
        return;
    }
    m_status->setText("Bu eylem uygulanamadı.");
}

void MainWindow::showSignatureHelp(bool autoTrigger) {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c) {
        if (!autoTrigger) m_status->setText("Bu dil için LSP yok.");
        return;
    }
    if (!c->hasCap("signatureHelpProvider")) {
        if (!autoTrigger) m_status->setText("Sunucu imza yardımı vermiyor.");
        return;
    }
    if (!c->isReady()) {
        if (!autoTrigger) withLspReady(c, [this]() { showSignatureHelp(false); });
        return;
    }
    QTextCursor cur = e->textCursor();
    c->requestSignature(e->filePath(), cur.blockNumber(), cur.columnNumber(),
        [this, e](QJsonObject res) {
            SignatureHelpData d = SignatureHelp::parse(res);
            if (d.empty()) return;
            if (e != currentEditor()) return;
            // Stage 27: overload döngüsü için sakla
            m_lastSig = d;
            m_lastSigEditor = e;
            showSigTooltip();
        });
}

// ============ Stage 27: LSP derinliği ============

// Akıllı seçim: önce sunucu aralığı, yoksa yerel büyütme
void MainWindow::expandSelectionSmart() {
    CodeEditor* e = currentEditor();
    if (!e) return;
    QString lang;
    LspClient* c = !e->filePath().isEmpty()
        ? lspClientFor(QFileInfo(e->filePath()).suffix(), lang)
        : nullptr;
    QTextCursor cur = e->textCursor();
    if (!c || !c->isReady() || !c->hasCap("selectionRangeProvider")) {
        e->expandSelection();
        return;
    }
    const QList<QPair<int, int>> pos = {
        qMakePair(cur.blockNumber(), cur.columnNumber())};
    c->requestSelectionRange(e->filePath(), pos, [this, e](QJsonObject res) {
        if (e != currentEditor()) return;
        QJsonArray arr = res["result"].toArray();
        if (arr.isEmpty()) { e->expandSelection(); return; }
        // İlk aralığın ebeveynine genişlet (mevcut seçimi kapsayan üst aralık)
        QTextCursor cur = e->textCursor();
        const int s0 = cur.selectionStart(), e0 = cur.selectionEnd();
        QJsonObject node = arr[0].toObject();
        QJsonObject best = node["range"].toObject();
        while (node.contains("parent")) {
            node = node["parent"].toObject();
            QJsonObject r = node["range"].toObject();
            const int rs = e->document()->findBlockByNumber(r["start"].toObject()["line"].toInt()).position()
                + r["start"].toObject()["character"].toInt();
            const int re = e->document()->findBlockByNumber(r["end"].toObject()["line"].toInt()).position()
                + r["end"].toObject()["character"].toInt();
            if (rs <= s0 && re >= e0 && (rs < s0 || re > e0)) { best = r; break; }
        }
        const int bs = e->document()->findBlockByNumber(best["start"].toObject()["line"].toInt()).position()
            + best["start"].toObject()["character"].toInt();
        const int be = e->document()->findBlockByNumber(best["end"].toObject()["line"].toInt()).position()
            + best["end"].toObject()["character"].toInt();
        if (bs == s0 && be == e0) { e->expandSelection(); return; }
        QTextCursor nc(e->document());
        nc.setPosition(bs);
        nc.setPosition(be, QTextCursor::KeepAnchor);
        e->setTextCursor(nc);
    });
}

// Belge bağlantısı: imleç satırındaki include/import hedefini aç
void MainWindow::openDocLink() {
    CodeEditor* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c || !c->isReady() || !c->hasCap("documentLinkProvider")) {
        m_status->setText("Sunucu belge bağlantısı vermiyor.");
        return;
    }
    pushDocToLsp(e);
    const int line = e->textCursor().blockNumber();
    c->requestDocumentLink(e->filePath(), [this, e, line](QJsonObject res) {
        QJsonArray arr = res["result"].toArray();
        QString best;
        int bestDist = INT_MAX;
        for (const QJsonValue& v : arr) {
            const QJsonObject o = v.toObject();
            const int l0 = o["range"].toObject()["start"].toObject()["line"].toInt();
            const int l1 = o["range"].toObject()["end"].toObject()["line"].toInt();
            if (line < l0 || line > l1) continue;
            const int d = qAbs(line - l0);
            if (d < bestDist) {
                bestDist = d;
                best = o["target"].toString();
            }
        }
        if (best.isEmpty()) { m_status->setText("Bu satırda bağlantı yok."); return; }
        const QString p = best.startsWith("file:") ? LspClient::uriToPath(best) : best;
        if (QFileInfo(p).isFile()) openFile(p);
        else m_status->setText("Hedef dosya değil: " + best.left(80));
    });
}

// Renk kutularını tazele (açık dosya için)
void MainWindow::refreshDocColors(CodeEditor* e) {
    if (!e || e->filePath().isEmpty()) return;
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c || !c->isReady() || !c->hasCap("colorProvider")) {
        e->setColorBoxes({});
        return;
    }
    pushDocToLsp(e);
    c->requestDocumentColor(e->filePath(), [e](QJsonObject res) {
        QList<CodeEditor::ColorBox> boxes;
        for (const QJsonValue& v : res["result"].toArray()) {
            const QJsonObject o = v.toObject();
            const QJsonObject r = o["range"].toObject();
            const int l0 = r["start"].toObject()["line"].toInt();
            if (l0 != r["end"].toObject()["line"].toInt()) continue; // tek satırlık
            QTextBlock b = e->document()->findBlockByNumber(l0);
            if (!b.isValid()) continue;
            const int s = b.position() + r["start"].toObject()["character"].toInt();
            const int en = b.position() + r["end"].toObject()["character"].toInt();
            const QJsonObject col = o["color"].toObject();
            QColor c(col["red"].toDouble() * 255, col["green"].toDouble() * 255,
                     col["blue"].toDouble() * 255,
                     col["alpha"].toDouble() * 255);
            CodeEditor::ColorBox cb;
            cb.line0 = l0;
            cb.start = s;
            cb.end = en;
            cb.color = c;
            boxes << cb;
            if (boxes.size() >= 200) break;
        }
        e->setColorBoxes(boxes);
    });
}

// Renk kutusu tıklaması → renk seç + metni değiştir
void MainWindow::editColorBox(CodeEditor* e, int line0) {
    if (!e) return;
    // Kutuyu bul (satırda ilk)
    // (kutular editörde; aralık için güncel listeyi iste)
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    Q_UNUSED(c);
    // Basit yol: kutunun metnini oku, diyaloğu aç, geri yaz
    // (aralık bilgisi için belgeyi yeniden tara)
    const QTextBlock b = e->document()->findBlockByNumber(line0);
    if (!b.isValid()) return;
    static QRegularExpression hexRe("#([0-9a-fA-F]{8}|[0-9a-fA-F]{6}|[0-9a-fA-F]{3})\b");
    auto m = hexRe.match(b.text());
    if (!m.hasMatch()) {
        m_status->setText("Bu satırda düzenlenebilir renk yok.");
        return;
    }
    const QColor cur(m.captured(0));
    const QColor picked = QColorDialog::getColor(cur, this, "Renk Seç");
    if (!picked.isValid()) return;
    QString rep = picked.name(QColor::HexRgb);
    if (m.captured(0).size() == 4) { // #rgb kısaltması
        const QString n = picked.name(QColor::HexRgb);
        rep = QString("#%1%2%3").arg(n[1]).arg(n[3]).arg(n[5]);
    }
    QTextCursor cur2(e->document());
    const int pos = b.position() + m.capturedStart();
    cur2.setPosition(pos);
    cur2.setPosition(pos + m.capturedLength(), QTextCursor::KeepAnchor);
    cur2.beginEditBlock();
    cur2.removeSelectedText();
    cur2.insertText(rep);
    cur2.endEditBlock();
    e->setTextCursor(cur2);
    pushDocToLsp(e);
    refreshDocColors(e);
}

// Code lens: gönderme sayılarını satır içi etiket yap
void MainWindow::refreshCodeLens(CodeEditor* e) {
    if (!e || e->filePath().isEmpty()) {
        if (e) e->setCodeLens({});
        return;
    }
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c || !c->isReady() || !c->hasCap("codeLensProvider")) {
        e->setCodeLens({});
        return;
    }
    pushDocToLsp(e);
    c->requestCodeLens(e->filePath(), [e](QJsonObject res) {
        QList<InlayHint> lens;
        // command.title "N references" kalıbı
        static QRegularExpression refRe(R"((\d+)\s+references?)");
        for (const QJsonValue& v : res["result"].toArray()) {
            const QJsonObject o = v.toObject();
            const QString title = o["command"].toObject()["title"].toString();
            auto m = refRe.match(title);
            if (!m.hasMatch()) continue;
            InlayHint h;
            h.line = o["range"].toObject()["start"].toObject()["line"].toInt();
            h.label = m.captured(1) + " gönderme";
            lens << h;
            if (lens.size() >= 100) break;
        }
        e->setCodeLens(lens);
    });
}

// Tip hiyerarşisi (çağrı diyaloğu yeniden kullanılır: üst/alt türler)
void MainWindow::showTypeHierarchy() {
    CodeEditor* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c || !c->isReady() || !c->hasCap("typeHierarchyProvider")) {
        m_status->setText("Sunucu tip hiyerarşisi vermiyor.");
        return;
    }
    QTextCursor cur = e->textCursor();
    const QString sym = cur.selectedText().isEmpty() ? lspWordUnderCursor(e)
                                                     : cur.selectedText();
    c->requestTypePrepare(e->filePath(), cur.blockNumber(), cur.columnNumber(),
                          [this, sym, c](QJsonObject res) {
                              QJsonArray arr = res["result"].toArray();
                              if (arr.isEmpty()) {
                                  m_status->setText("Tip bulunamadı.");
                                  return;
                              }
                              const QJsonObject item = arr[0].toObject();
                              auto* d = new CallHierarchyDialog(sym, this);
                              d->setAttribute(Qt::WA_DeleteOnClose);
                              c->requestTypeSupertypes(item, [d](QJsonObject r) {
                                  QList<CallNode> ns;
                                  for (const QJsonValue& v : r["result"].toArray()) {
                                      const QJsonObject o = v.toObject();
                                      CallNode n;
                                      n.name = o["name"].toString();
                                      n.uri = o["uri"].toString();
                                      const QJsonObject rng =
                                          o["range"].toObject()["start"].toObject();
                                      n.line = rng["line"].toInt();
                                      ns << n;
                                  }
                                  d->setIncoming(ns);
                              });
                              c->requestTypeSubtypes(item, [d](QJsonObject r) {
                                  QList<CallNode> ns;
                                  for (const QJsonValue& v : r["result"].toArray()) {
                                      const QJsonObject o = v.toObject();
                                      CallNode n;
                                      n.name = o["name"].toString();
                                      n.uri = o["uri"].toString();
                                      const QJsonObject rng =
                                          o["range"].toObject()["start"].toObject();
                                      n.line = rng["line"].toInt();
                                      ns << n;
                                  }
                                  d->setOutgoing(ns);
                              });
                              if (d) d->show();
                          });
}

// Çekme tanılama (isteğe bağlı yenileme)
void MainWindow::pullDiagnostics() {
    CodeEditor* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c || !c->isReady() || !c->hasCap("diagnosticProvider")) {
        m_status->setText("Sunucu çekme tanılama vermiyor.");
        return;
    }
    pushDocToLsp(e);
    c->requestPullDiagnostics(e->filePath(), [this, e](QJsonObject res) {
        QJsonObject r = res["result"].toObject();
        const QString path = e->filePath();
        QList<LspDiag> diags;
        for (const QJsonValue& v : r["items"].toArray()) {
            const QJsonObject d = v.toObject();
            const QJsonObject rng = d["range"].toObject();
            LspDiag dg;
            dg.path = path;
            dg.line = rng["start"].toObject()["line"].toInt();
            dg.col = rng["start"].toObject()["character"].toInt();
            dg.endLine = rng["end"].toObject()["line"].toInt();
            dg.endCol = rng["end"].toObject()["character"].toInt();
            dg.severity = d["severity"].toInt(1);
            dg.message = d["message"].toString();
            // Stage 27: ilişkili bilgi satıra eklenir
            QStringList rel;
            for (const QJsonValue& rv : d["relatedInformation"].toArray()) {
                const QJsonObject ro = rv.toObject();
                const QJsonObject rl = ro["location"].toObject();
                rel << QString("%1:%2").arg(
                    QFileInfo(LspClient::uriToPath(rl["uri"].toString())).fileName(),
                    QString::number(
                        rl["range"].toObject()["start"].toObject()["line"].toInt() + 1));
            }
            if (!rel.isEmpty()) dg.message += "  →  " + rel.join(", ");
            diags << dg;
        }
        onDiagnostics(path, diags);
        m_status->setText(QString("%1 tanı çekildi").arg(diags.size()));
    });
}

// Seçim biçimlendirme
void MainWindow::formatSelection() {
    CodeEditor* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    QTextCursor cur = e->textCursor();
    if (!cur.hasSelection()) { formatDocument(); return; }
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c || !c->isReady() || !c->hasCap("documentRangeFormattingProvider")) {
        m_status->setText("Sunucu aralık biçimlendirme vermiyor.");
        return;
    }
    if (!c->isReady()) return;
    pushDocToLsp(e);
    QTextCursor s(e->document()), en(e->document());
    s.setPosition(cur.selectionStart());
    en.setPosition(cur.selectionEnd());
    c->requestRangeFormat(e->filePath(), s.blockNumber(), s.columnNumber(),
                          en.blockNumber(), en.columnNumber(),
                          [this, e](QJsonObject res) {
                              QList<LspTextEdit> edits = TextEdits::parse(res);
                              if (edits.isEmpty()) {
                                  m_status->setText("Değişiklik yok.");
                                  return;
                              }
                              e->setPlainText(TextEdits::apply(e->toPlainText(), edits));
                              pushDocToLsp(e);
                              toast(1, "Seçim biçimlendirildi.");
                          });
}

// Sunucu günlüğü görüntüleyici
void MainWindow::showLspLog() {
    QString out = "== C++ (clangd) ==\n";
    if (m_lspCpp) out += m_lspCpp->messageLog().join("\n");
    out += "\n\n== Python (pylsp) ==\n";
    if (m_lspPy) out += m_lspPy->messageLog().join("\n");
    out += "\n\n== Rust ==\n";
    if (m_lspRs) out += m_lspRs->messageLog().join("\n");
    out += "\n\n== Go ==\n";
    if (m_lspGo) out += m_lspGo->messageLog().join("\n");
    out += "\n\n== JS/TS ==\n";
    if (m_lspJs) out += m_lspJs->messageLog().join("\n");
    if (out.trimmed().endsWith("==")) out += "(ileti yok — sunucu çalışmadı)";
    auto* d = new QDialog(this);
    d->setWindowTitle("LSP Trafiği");
    d->resize(640, 440);
    auto* lay = new QVBoxLayout(d);
    auto* t = new QTextEdit(d);
    t->setReadOnly(true);
    t->setFontFamily("monospace");
    t->setPlainText(out);
    lay->addWidget(t, 1);
    auto* row = new QHBoxLayout();
    auto* bClear = new QPushButton("Temizle", d);
    connect(bClear, &QPushButton::clicked, this, [this]() {
        for (LspClient* c : {m_lspCpp, m_lspPy, m_lspRs, m_lspGo, m_lspJs})
            if (c) c->clearLog();
    });
    row->addStretch(1);
    row->addWidget(bClear);
    lay->addLayout(row);
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->show();
}

// Sunucu yöneticisi: kurulu sürümler + kurulum yönergeleri
void MainWindow::showLspServers() {
    QStringList rows;
    for (const LspServerDef& d : LspServers::table()) {
        QString prog = (d.lang == "python") ? LspServers::pythonExe(m_root)
                                            : LspServers::findProgram(d.program);
        QString ver;
        if (!prog.isEmpty()) {
            QProcess p;
            p.start(prog, {"--version"});
            if (p.waitForFinished(5000))
                ver = QString::fromUtf8(p.readAllStandardOutput() +
                                        p.readAllStandardError())
                          .split('\n')
                          .first()
                          .trimmed()
                          .left(80);
        }
        rows << QString("%1: %2%3").arg(
            d.lang, prog.isEmpty() ? ("YOK — " + d.installHint) : prog,
            ver.isEmpty() ? "" : ("  (" + ver + ")"));
    }
    QMessageBox::information(this, "Dil Sunucuları", rows.join("\n\n"));
}

// ============ Stage 28 ============

// Çalışma alanı güveni: bilinmeyen kökte sor, kısıtlı modda tehlikelileri kapat
void MainWindow::checkWorkspaceTrust() {
    m_trusted = true;
    if (m_root.isEmpty()) return;
    QSettings q("Verso", "VersoCoder");
    QStringList trusted = q.value("trust/roots").toStringList();
    QStringList denied = q.value("trust/denied").toStringList();
    if (trusted.contains(m_root)) return;
    if (denied.contains(m_root)) {
        m_trusted = false;
        toast(2, "Güvenilmez çalışma alanı — görev/eklenti/ajan kapalı");
        return;
    }
    auto r = QMessageBox::question(
        this, "Çalışma Alanı Güveni",
        "Bu klasöre güveniyor musunuz?\n" + m_root +
            "\n\nGüvenilmeyen alanda görev, eklenti ve AI ajanı çalışmaz.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (r == QMessageBox::Yes) {
        trusted << m_root;
        q.setValue("trust/roots", trusted);
        return;
    }
    auto r2 = QMessageBox::question(
        this, "Çalışma Alanı Güveni", "Tek seferlik mi açılsın, hep mi kısıtlansın?",
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    // Yes = tek seferlik güven, No = hep kısıtla, Cancel = şimdi kısıtla
    if (r2 == QMessageBox::Yes) return;
    if (r2 == QMessageBox::No) {
        denied << m_root;
        q.setValue("trust/denied", denied);
    }
    m_trusted = false;
    toast(2, "Güvenilmez çalışma alanı — görev/eklenti/ajan kapalı");
}

bool MainWindow::requireTrusted(const QString& what) {
    if (m_trusted) return true;
    toast(2, what + " güvenilmez alanda kapalı (paletten güven verin)");
    return false;
}

// Yer imleri
void MainWindow::applyBmMarks(CodeEditor* e) {
    if (e) e->setBookmarks(m_marks.lines(e->filePath()));
}

void MainWindow::toggleBookmark() {
    CodeEditor* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    m_marks.toggle(e->filePath(), e->textCursor().blockNumber() + 1);
    m_marks.save();
    applyBmMarks(e);
}

void MainWindow::jumpBookmark(int dir) {
    CodeEditor* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    const int cur = e->textCursor().blockNumber() + 1;
    const int ln = dir > 0 ? m_marks.next(e->filePath(), cur)
                           : m_marks.prev(e->filePath(), cur);
    if (ln < 0) {
        toast(0, "Başka yer imi yok");
        return;
    }
    e->gotoLine(ln);
    e->setFocus();
}

void MainWindow::showBookmarks() {
    auto* d = new BookmarkDialog(&m_marks, this);
    d->setAttribute(Qt::WA_DeleteOnClose);
    connect(d, &BookmarkDialog::jumpRequested, this,
            [this, d](const QString& path, int line) {
                d->close();
                openFileAt(path, line);
            });
    d->show();
}

// Satır yorumu (dile göre önek, seçim destekli)
void MainWindow::toggleComment() {
    CodeEditor* e = currentEditor();
    if (!e) return;
    QString prefix = LanguageSupport::commentPrefix(
        e->filePath().isEmpty() ? e->lang() : e->filePath());
    if (prefix.isEmpty()) {
        toast(0, "Bu dilde satır yorumu yok");
        return;
    }
    QTextCursor c = e->textCursor();
    QTextDocument* doc = e->document();
    // Stage 28: blok yorum dillerinde kapatma eki (örn. <!-- ... -->)
    const QString suffix = (prefix == "<!--") ? " -->" : QString();
    auto leadWs = [](const QString& t) {
        int n = 0;
        while (n < t.size() && t[n].isSpace()) ++n;
        return n;
    };
    auto trailWs = [](const QString& t) {
        int n = 0;
        while (n < t.size() && t[t.size() - 1 - n].isSpace()) ++n;
        return n;
    };
    int a = doc->findBlock(c.selectionStart()).blockNumber();
    int b = doc->findBlock(c.selectionEnd()).blockNumber();
    // İmleç satır sonundaysa son satırı hariç tut
    if (c.hasSelection() && c.columnNumber() == 0 && b > a) --b;
    bool allOn = true;
    for (int ln = a; ln <= b; ++ln) {
        const QString t = doc->findBlockByNumber(ln).text();
        const QString s = t.mid(leadWs(t));
        if (s.isEmpty()) continue;
        if (!s.startsWith(prefix)) { allOn = false; break; }
    }
    c.beginEditBlock();
    for (int ln = a; ln <= b; ++ln) {
        QTextBlock blk = doc->findBlockByNumber(ln);
        if (!blk.isValid() || blk.text().trimmed().isEmpty()) continue;
        QTextCursor lc(doc);
        lc.setPosition(blk.position() + leadWs(blk.text()));
        if (allOn) {
            lc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, prefix.size());
            if (lc.selectedText() == prefix) {
                lc.removeSelectedText();
                // Öneki izleyen tek boşluğu da al
                QTextCursor sp(doc);
                sp.setPosition(lc.position());
                sp.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
                if (sp.selectedText() == " ") sp.removeSelectedText();
            }
            if (!suffix.isEmpty()) {
                // Satır sonundaki kapatma ekini temizle
                const QString line = blk.text();
                const QString tr = line.left(line.size() - trailWs(line));
                const QString st = suffix.trimmed();
                if (tr.endsWith(st)) {
                    QTextCursor ec(doc);
                    ec.setPosition(blk.position() + tr.size() - st.size());
                    ec.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, st.size());
                    ec.removeSelectedText();
                }
            }
        } else {
            lc.insertText(prefix + " ");
            if (!suffix.isEmpty()) {
                QTextCursor ec(doc);
                ec.setPosition(blk.position() + blk.length() - 1);
                ec.movePosition(QTextCursor::EndOfBlock);
                ec.insertText(suffix);
            }
        }
    }
    c.endEditBlock();
}

// Görüntü/md yönlendirme (dosya açılışında)
bool MainWindow::openSpecial(const QString& path) {
    const QString suf = QFileInfo(path).suffix().toLower();
    static const QStringList imgs = {"png", "jpg", "jpeg", "gif", "bmp", "svg", "webp"};
    if (imgs.contains(suf)) {
        auto* d = new ImageViewerDialog(path, this);
        d->setAttribute(Qt::WA_DeleteOnClose);
        d->show();
        return true;
    }
    return false;
}

void MainWindow::showMarkdownPreview() {
    CodeEditor* e = currentEditor();
    if (!e) return;
    auto* d = new MarkdownPreviewDialog(e, this);
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->show();
}

void MainWindow::showHexView() {
    CodeEditor* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    auto* d = new HexViewDialog(e->filePath(), this);
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->show();
}

// Yol kopyalama çeşitleri
void MainWindow::copyPath(int mode) {
    CodeEditor* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    const QFileInfo fi(e->filePath());
    QString out = e->filePath();
    if (mode == 1) out = QDir(m_root).relativeFilePath(e->filePath());
    else if (mode == 2) out = fi.fileName();
    QApplication::clipboard()->setText(out);
    toast(1, "Kopyalandı: " + out.left(80));
}

// Stage 28: importları düzenle (source.organizeImports, doğrudan uygula)
void MainWindow::organizeImports() {
    CodeEditor* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c || !c->isReady()) {
        m_status->setText("Sunucu hazır değil.");
        return;
    }
    QTextCursor cur = e->textCursor();
    c->requestCodeAction(e->filePath(), 0, 0,
                         e->document()->blockCount(), 0,
                         {"source.organizeImports"},
                         [this, e, c](QJsonObject res) {
                             const QList<CodeActionItem> items = CodeActionList::parse(res);
                             const CodeActionItem* pick = nullptr;
                             for (const CodeActionItem& it : items)
                                 if (it.hasEdit || it.kind == "source.organizeImports") {
                                     pick = &it;
                                     break;
                                 }
                             if (!pick) {
                                 m_status->setText("Düzenlenecek import yok.");
                                 return;
                             }
                             // Ham yanıttan WorkspaceEdit uygula
                             QJsonObject raw = pick->raw;
                             QJsonObject edit = raw["edit"].toObject();
                             bool applied = false;
                             // changes: {uri: [TextEdit]}
                             const QJsonObject changes = edit["changes"].toObject();
                             for (auto it = changes.constBegin();
                                  it != changes.constEnd() && !applied; ++it) {
                                 const QString fp = LspClient::uriToPath(it.key());
                                 QList<LspTextEdit> edits =
                                     TextEdits::parseArray(it.value().toArray(), fp);
                                 if (edits.isEmpty()) continue;
                                 if (fp == e->filePath()) {
                                     e->setPlainText(TextEdits::apply(e->toPlainText(), edits));
                                     pushDocToLsp(e);
                                 } else {
                                     QFile f(fp);
                                     if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                                         const QString t = QString::fromUtf8(f.readAll());
                                         f.close();
                                         if (f.open(QIODevice::WriteOnly | QIODevice::Text |
                                                    QIODevice::Truncate))
                                             f.write(TextEdits::apply(t, edits).toUtf8());
                                     }
                                 }
                                 applied = true;
                             }
                             // command-only eylemse sunucuya ilet
                             if (!applied) {
                                 const QJsonObject cmd = raw["command"].toObject();
                                 if (!cmd.isEmpty() && c) {
                                     c->request("workspace/executeCommand", cmd, nullptr);
                                     applied = true;
                                 }
                             }
                             toast(applied ? 1 : 0, applied ? "Importlar düzenlendi."
                                                             : "Uygulanabilir düzenleme yok.");
                         });
}

// Stage 28: git dosya geçmişi + etiketler
void MainWindow::showGitHistory() {
    CodeEditor* e = currentEditor();
    const QString path = e ? e->filePath() : QString();
    if (path.isEmpty()) { toast(0, "Önce dosya açın"); return; }
    auto* d = new GitHistoryDialog(m_root, path, this);
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->show();
}

// Stage 30: kopan uzak bağlantıya tek tıkla dönüş
void MainWindow::remoteReconnect() {
    if (remoteConnected()) {
        toast(0, "Zaten bağlı");
        return;
    }
    if (!m_remoteProfile.isValid()) {
        toast(0, "Kayıtlı profil yok — önce bağlanın");
        return;
    }
    remoteConnect(m_remoteProfile);
}

// Stage 30: clang-format yedeği (LSP biçimlendirme yoksa)
void MainWindow::formatWithClangFormat(CodeEditor* e) {
    if (!e || e->filePath().isEmpty()) return;
    const QString bin = QStandardPaths::findExecutable("clang-format");
    if (bin.isEmpty()) {
        m_status->setText("Biçimlendirme yok: ne LSP ne clang-format bulundu.");
        return;
    }
    QProcess p;
    p.setWorkingDirectory(QFileInfo(e->filePath()).absolutePath());
    p.start(bin, {"--style=file", "--assume-filename=" + e->filePath()});
    if (!p.waitForStarted(3000)) return;
    p.write(e->toPlainText().toUtf8());
    p.closeWriteChannel();
    if (!p.waitForFinished(10000) || p.exitCode() != 0) {
        toast(2, "clang-format başarısız");
        return;
    }
    const QString out = QString::fromUtf8(p.readAllStandardOutput());
    if (!out.isEmpty() && out != e->toPlainText()) {
        e->setPlainText(out);
        pushDocToLsp(e);
        toast(1, "clang-format ile biçimlendirildi.");
    } else {
        m_status->setText("Biçimlendirme değişikliği yok.");
    }
}

// Stage 30: çakışmalı dosyayı birleştirme düzenleyicisinde aç
void MainWindow::openMergeEditor() {
    CodeEditor* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    if (MergeParse::find(e->toPlainText()).isEmpty()) {
        toast(0, "Çakışma işareti yok");
        return;
    }
    auto* d = new MergeEditorDialog(e->filePath(), e->toPlainText(), this);
    d->setAttribute(Qt::WA_DeleteOnClose);
    connect(d, &QDialog::accepted, this, [this, e, d]() {
        e->setPlainText(d->resolvedText());
        e->document()->setModified(true);
        pushDocToLsp(e);
        toast(1, "Birleştirme uygulandı");
    });
    d->show();
}

void MainWindow::showGitTags() {
    if (m_root.isEmpty()) return;
    auto* d = new GitTagsDialog(m_root, this);
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->show();
}

// Stage 28: terminalde bul
void MainWindow::terminalFind(bool forward) {
    if (!m_terminal) return;
    m_bottomTabs->setCurrentWidget(m_terminal);
    if (forward && m_terminal->findNext(true)) return;
    if (!forward && m_terminal->findNext(false)) return;
    bool ok = false;
    const QString q = QInputDialog::getText(this, "Terminalde Bul", "Metin:",
                                            QLineEdit::Normal, QString(), &ok);
    if (ok && !q.isEmpty() && !m_terminal->findFirst(q))
        toast(0, "Bulunamadı: " + q.left(40));
}

// Stage 28: bildirim geçmişi
void MainWindow::showNotifications() {
    auto* d = new QDialog(this);
    d->setWindowTitle("Bildirimler");
    d->resize(520, 380);
    auto* lay = new QVBoxLayout(d);
    auto* list = new QListWidget(d);
    list->addItems(ToastManager::instance().history());
    if (list->count() == 0) list->addItem("(bildirim yok)");
    lay->addWidget(list, 1);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, d);
    connect(box, &QDialogButtonBox::rejected, d, &QDialog::reject);
    lay->addWidget(box);
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->show();
}

// Stage 28: son projeler (kalıcı, en çok 8)
void MainWindow::pushRecentProject(const QString& root) {
    if (root.isEmpty()) return;
    QSettings q("Verso", "VersoCoder");
    QStringList r = q.value("session/recentRoots").toStringList();
    r.removeAll(root);
    r.prepend(root);
    while (r.size() > 8) r.removeLast();
    q.setValue("session/recentRoots", r);
}

void MainWindow::openRoot(const QString& root) {
    if (root.isEmpty() || !QDir(root).exists()) return;
    m_root = root;
    refreshProjectViews();
    pushRecentProject(root);
    AppSettings cur = SettingsManager::instance().load();
    cur.lastRoot = root;
    SettingsManager::instance().save(cur);
}

// Stage 28: çalıştırılabilir dosyayı hata ayıklayıcıda çalıştır
void MainWindow::debugRunFile() {
    CodeEditor* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    const QString prog = e->filePath();
    QFileInfo fi(prog);
    if (!fi.isExecutable() && !fi.suffix().isEmpty()) {
        toast(0, "Çalıştırılabilir değil — önce derleyin (F5), sonra tekrar deneyin");
        return;
    }
    m_debugOverride = true;
    m_overrideLc = LaunchConfig();
    m_overrideLc.program = prog;
    m_overrideLc.cwd = fi.absolutePath();
    debugStart();
}

// Stage 28: son AI/ajan düzenlemesini geri al
void MainWindow::undoAiEdit() {
    if (m_aiUndo.isEmpty()) {
        toast(0, "Geri alınacak AI düzenlemesi yok");
        return;
    }
    const AiUndo u = m_aiUndo.takeLast();
    if (!u.editor.isNull()) {
        QSignalBlocker b(u.editor);
        if (u.wholeFile || u.path.isEmpty()) {
            u.editor->setPlainText(u.oldText);
        } else {
            QTextCursor c(u.editor->document());
            const int maxPos = u.editor->document()->characterCount() - 1;
            c.setPosition(qBound(0, u.selStart, maxPos));
            c.setPosition(qBound(0, u.selStart + u.selLen, maxPos), QTextCursor::KeepAnchor);
            c.insertText(u.oldText);
        }
        u.editor->document()->setModified(true);
        pushDocToLsp(u.editor);
        toast(1, "AI düzenlemesi geri alındı (sekme)");
        return;
    }
    if (!u.path.isEmpty()) {
        QFile f(u.path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            f.write(u.oldText.toUtf8());
            f.close();
            for (CodeEditor* ed : allEditors())
                if (QFileInfo(ed->filePath()) == QFileInfo(u.path)) {
                    QSignalBlocker b2(ed);
                    ed->setPlainText(u.oldText);
                    ed->document()->setModified(false);
                }
            toast(1, "AI düzenlemesi geri alındı: " + QFileInfo(u.path).fileName());
            return;
        }
    }
    toast(2, "Geri alınamadı");
}

void MainWindow::pushAiUndo(const QString& path, const QString& oldText,
                            CodeEditor* editor, bool whole, int selStart, int selLen) {
    AiUndo u;
    u.path = path;
    u.oldText = oldText;
    u.editor = editor;
    u.wholeFile = whole;
    u.selStart = selStart;
    u.selLen = selLen;
    m_aiUndo << u;
    while (m_aiUndo.size() > 20) m_aiUndo.removeFirst();
}

// Stage 30: dependsOn zincirini sırayla koştur
void MainWindow::runTaskChain() {
    if (m_root.isEmpty()) return;
    TaskRunner runner;
    if (!runner.loadForRoot(m_root)) {
        toast(0, "tasks.json yok");
        return;
    }
    QString err;
    const QList<QString> order =
        TaskChain::order(TaskChain::depsFrom(runner.tasksJsonArray()), err);
    if (!err.isEmpty()) {
        toast(2, err);
        return;
    }
    if (order.isEmpty()) {
        toast(0, "Çalıştırılacak görev yok");
        return;
    }
    m_termDock->setVisible(true);
    m_bottomTabs->setCurrentWidget(m_taskPanel);
    m_chainQueue = order;
    m_chainActive = true;
    connect(m_taskPanel, &TaskPanel::taskDone, this, &MainWindow::runNextChainTask,
            Qt::UniqueConnection);
    runNextChainTask();
}

void MainWindow::runNextChainTask() {
    if (!m_chainActive) return; // elle koşturmalarda zincir ilerlemez
    if (m_chainQueue.isEmpty() || !m_taskPanel) {
        if (m_chainQueue.isEmpty()) {
            m_chainActive = false;
            toast(1, "Zincir tamamlandı ✓");
        }
        return;
    }
    const QString label = m_chainQueue.takeFirst();
    m_taskPanel->runTask(label);
    // Devamı taskDone sinyalinden gelir (her iki çalıştırma yolunda da yayılır)
}

// Stage 30: ~/.ssh/config içe aktarma
void MainWindow::importSshConfig() {
    const QString path = QDir::homePath() + "/.ssh/config";
    const QList<SshConfig::Entry> es = SshConfig::parseFile(path);
    if (es.isEmpty()) {
        toast(0, "~/.ssh/config bulunamadı ya da kayıt yok");
        return;
    }
    int n = 0;
    for (const SshConfig::Entry& e : es) {
        ConnectionProfile p = SshConfig::toProfile(e);
        if (!ConnectionProfiles::exists(p.name) && ConnectionProfiles::save(p)) ++n;
    }
    toast(1, QString("%1 profil içe aktarıldı").arg(n));
}

// Stage 30: sohbetlerde arama
void MainWindow::searchAiChats() {
    bool ok = false;
    const QString q = QInputDialog::getText(this, "Sohbetlerde Ara", "Metin:",
                                            QLineEdit::Normal, QString(), &ok);
    if (!ok || q.trimmed().isEmpty() || !m_ai) return;
    m_ai->searchChats(q.trimmed());
}

// Stage 30: zamanlanmış AI denetimi
void MainWindow::startAiSchedule() {
    delete m_aiTimer;
    m_aiTimer = nullptr;
    const int mins = SettingsManager::instance().load().aiScheduleMin;
    if (mins <= 0 || !m_ai) return;
    m_aiTimer = new QTimer(this);
    m_aiTimer->setInterval(mins * 60000);
    connect(m_aiTimer, &QTimer::timeout, this, [this]() {
        if (!m_ai || m_root.isEmpty()) return;
        m_ai->send("Zamanlanmış denetim: projede bitmemiş TODO "
                   "veya bariz hata var mı? Kısa maddelerle yaz.");
    });
    m_aiTimer->start();
}

// Stage 30: pano yöneticisi
void MainWindow::showClipboardManager() {
    auto* d = new ClipboardDialog(&m_clipRing, this);
    d->setAttribute(Qt::WA_DeleteOnClose);
    connect(d, &ClipboardDialog::pasteRequested, this, [this, d](const QString& t) {
        d->close();
        if (auto* e = currentEditor()) {
            e->insertPlainText(t);
            e->setFocus();
        } else {
            QApplication::clipboard()->setText(t);
        }
    });
    d->show();
}

// Stage 30: yeni proje sihirbazı
void MainWindow::newProjectWizard() {
    NewProjectDialog d(this);
    if (d.exec() != QDialog::Accepted || d.createdRoot().isEmpty()) return;
    openRoot(d.createdRoot());
}

// Stage 31: bekleyen çökme izleri
void MainWindow::checkCrashDumps() {
    const QStringList dumps = CrashHandler::pendingDumps();
    if (dumps.isEmpty()) return;
    auto r = QMessageBox::question(
        this, "Çökme İzi",
        QString("Önceki çalışmada %1 çökme izi bulundu.\nTeşhis → Tanı Raporu'na ekleyebilirsiniz.\nŞimdi silinsin mi?")
            .arg(dumps.size()),
        QMessageBox::Yes | QMessageBox::No);
    if (r == QMessageBox::Yes) CrashHandler::clearDumps();
}

// Stage 32: başlangıç süresini bütçeyle karşılaştır
void MainWindow::checkStartupBudget(int budgetMs) {
    const qint64 e = PerfMonitor::instance().sinceStartMs();
    if (e > budgetMs)
        toast(3, QString("Başlangıç %1 ms — bütçe %2 ms aşıldı (Tanı → Perf)")
                      .arg(e)
                      .arg(budgetMs));
    else
        m_status->setText(QString("Hazır — başlangıç %1 ms").arg(e));
}

// Stage 27: kayıtlı imza yardımını (aktif overload ile) yeniden göster
void MainWindow::showSigTooltip() {
    CodeEditor* e = m_lastSigEditor.data();
    if (!e || m_lastSig.empty()) return;
    QRect r = e->cursorRect(e->textCursor());
    QToolTip::showText(e->mapToGlobal(r.bottomRight()),
                       SignatureHelp::render(m_lastSig).left(800), e);
}

void MainWindow::cycleSignature(int dir) {
    if (m_lastSig.sigs.size() < 2) return;
    CodeEditor* e = m_lastSigEditor.data();
    if (!e || e != currentEditor()) return;
    const int n = m_lastSig.sigs.size();
    m_lastSig.activeSig = (m_lastSig.activeSig + dir + n) % n;
    m_lastSig.activeParam = 0;
    showSigTooltip();
}

void MainWindow::showDocSymbols() {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c) { m_status->setText("Bu dil için LSP yok."); return; }
    if (!c->isReady()) { withLspReady(c, [this]() { showDocSymbols(); }); return; }
    c->requestDocSymbols(e->filePath(), [this, e](QJsonObject res) {
        // parseDocument result dizisi bekler
        QJsonArray arr = res["result"].toArray();
        QList<SymbolNode> roots = SymbolTree::parseDocumentArray(arr);
        if (roots.isEmpty()) { m_status->setText("Sembol yok."); return; }
        SymbolPickerDialog d(QString("Outline — %1").arg(QFileInfo(e->filePath()).fileName()),
                             roots, this);
        if (d.exec() == QDialog::Accepted && !d.selected().name.isEmpty()) {
            e->gotoLine(d.selected().line + 1);
            e->flashLine(d.selected().line);
        }
    });
}

void MainWindow::showWorkspaceSymbols() {
    auto* e = currentEditor();
    QString lang;
    LspClient* c = nullptr;
    if (e && !e->filePath().isEmpty()) c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c) c = m_lspCpp && m_lspCpp->isRunning() ? m_lspCpp : m_lspPy;
    if (!c || !c->isRunning()) { m_status->setText("LSP çalışmıyor."); return; }
    bool ok = false;
    const QString q = QInputDialog::getText(this, "Çalışma Alanı Simgesi",
                                            "Sembol:", QLineEdit::Normal, m_lastSymbol, &ok);
    if (!ok) return;
    m_lastSymbol = q;
    c->requestWorkspaceSymbols(q, [this](QJsonObject res) {
        QList<SymbolNode> roots = SymbolTree::parseWorkspace(res);
        if (roots.isEmpty()) { m_status->setText("Sembol bulunamadı."); return; }
        SymbolPickerDialog d("Çalışma Alanı Simgeleri", roots, this);
        if (d.exec() == QDialog::Accepted && !d.selected().name.isEmpty()) {
            const SymbolNode n = d.selected();
            if (!n.path.isEmpty()) {
                openFileAt(n.path, n.line + 1);
                if (auto* ne = currentEditor()) ne->flashLine(n.line);
            } else if (auto* ce = currentEditor()) {
                ce->gotoLine(n.line + 1);
                ce->flashLine(n.line);
            }
        }
    });
}

void MainWindow::formatDocument() {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c || !c->hasCap("documentFormattingProvider")) {
        formatWithClangFormat(e); // Stage 30: harici yedek
        return;
    }
    if (!c->isReady()) { withLspReady(c, [this]() { formatDocument(); }); return; }
    pushDocToLsp(e);
    const QString path = e->filePath();
    const QString old = e->toPlainText();
    c->requestFormat(path, [this, path, old](QJsonObject res) {
        QList<LspTextEdit> edits = TextEdits::parse(res);
        if (edits.isEmpty()) { m_status->setText("Biçimlendirme değişikliği yok."); return; }
        for (QTabWidget* t : {m_tabs, m_tabs2})
            for (int i = 0; i < t->count(); ++i)
                if (auto* ce = qobject_cast<CodeEditor*>(t->widget(i)))
                    if (ce->filePath() == path) {
                        ce->setPlainText(TextEdits::apply(ce->toPlainText(), edits));
                        pushDocToLsp(ce);
                        toast(1, "Biçimlendirildi.");
                        return;
                    }
        Q_UNUSED(old);
    });
}

bool MainWindow::formatOnSaveIfEnabled(CodeEditor* e) {
    AppSettings s = SettingsManager::instance().load();
    if (!s.formatOnSave || !e || e->filePath().isEmpty() || e->largeFileMode()) return false;
    if (e->property("fmtGuard").toBool()) { e->setProperty("fmtGuard", false); return false; }
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c || !c->hasCap("documentFormattingProvider")) return false;
    if (!c->isReady()) return false; // sunucu ısınıyor: bu seferlik formatsız kaydet
    e->setProperty("fmtGuard", true);
    pushDocToLsp(e);
    const QString path = e->filePath();
    c->requestFormat(path, [this, path](QJsonObject res) {
        QList<LspTextEdit> edits = TextEdits::parse(res);
        for (QTabWidget* t : {m_tabs, m_tabs2})
            for (int i = 0; i < t->count(); ++i)
                if (auto* ce = qobject_cast<CodeEditor*>(t->widget(i)))
                    if (ce->filePath() == path) {
                        if (!edits.isEmpty()) {
                            ce->setPlainText(TextEdits::apply(ce->toPlainText(), edits));
                        }
                        ce->setProperty("fmtGuard", false);
                        saveCurrent(); // biçimlendirilmiş hali kaydet
                        return;
                    }
    });
    return true; // kaydetme yanıt sonrası yapılacak
}

void MainWindow::toggleInlayHints() {
    AppSettings s = SettingsManager::instance().load();
    s.inlayHints = !s.inlayHints;
    SettingsManager::instance().save(s);
    if (auto* e = currentEditor()) refreshInlayHints(e);
    toast(0, s.inlayHints ? "Satır içi ipuçları açık" : "Satır içi ipuçları kapalı");
}

void MainWindow::toggleSemantic() {
    AppSettings s = SettingsManager::instance().load();
    s.semanticHighlight = !s.semanticHighlight;
    SettingsManager::instance().save(s);
    if (auto* e = currentEditor()) refreshSemantic(e);
    toast(0, s.semanticHighlight ? "Semantik renklendirme açık" : "Semantik renklendirme kapalı");
}

void MainWindow::refreshInlayHints(CodeEditor* e) {
    AppSettings s = SettingsManager::instance().load();
    if (!s.inlayHints || !e || e->filePath().isEmpty() || e->largeFileMode()) {
        if (e) e->clearInlayHints();
        return;
    }
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c || !c->hasCap("inlayHintProvider")) { e->clearInlayHints(); return; }
    if (!c->isReady()) return; // bir sonraki didChange/debounce döngüsünde tazelenir
    const QString path = e->filePath();
    const int endLine = e->blockCount() + 5;
    c->requestInlay(path, 0, endLine, [this, path](QJsonObject res) {
        QList<InlayHint> hints = InlayHintList::parse(res);
        for (QTabWidget* t : {m_tabs, m_tabs2})
            for (int i = 0; i < t->count(); ++i)
                if (auto* ce = qobject_cast<CodeEditor*>(t->widget(i)))
                    if (ce->filePath() == path) { ce->setInlayHints(hints); return; }
    });
}

void MainWindow::refreshSemantic(CodeEditor* e) {
    AppSettings s = SettingsManager::instance().load();
    if (!s.semanticHighlight || !e || e->filePath().isEmpty() || e->largeFileMode()) {
        if (e) e->clearSemanticTokens();
        return;
    }
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c || !c->hasCap("semanticTokensProvider")) { e->clearSemanticTokens(); return; }
    if (!c->isReady()) return; // bir sonraki didChange/debounce döngüsünde tazelenir
    const QString path = e->filePath();
    c->requestSemantic(path, [this, path, c](QJsonObject res) {
        QList<SemanticToken> toks = SemanticTokens::decode(res);
        // Legend: sunucu bilgisinden (yoksa clangd varsayılan sırası)
        QStringList legend;
        const QJsonObject caps = c->serverCaps();
        QJsonArray arr = caps["semanticTokensProvider"].toObject()["legend"]
                             .toObject()["tokenTypes"].toArray();
        for (const QJsonValue& v : arr) legend << v.toString();
        if (legend.isEmpty())
            legend = QStringList{"namespace","type","class","enum","interface","struct",
                "typeParameter","parameter","variable","property","enumMember","function",
                "method","macro","keyword","comment","string","number","operator"};
        for (QTabWidget* t : {m_tabs, m_tabs2})
            for (int i = 0; i < t->count(); ++i)
                if (auto* ce = qobject_cast<CodeEditor*>(t->widget(i)))
                    if (ce->filePath() == path) { ce->setSemanticTokens(toks, legend); return; }
    });
}

void MainWindow::showCallHierarchy() {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c) { m_status->setText("Bu dil için LSP yok."); return; }
    if (!c->hasCap("callHierarchyProvider")) {
        m_status->setText("Sunucu çağrı hiyerarşisi vermiyor.");
        return;
    }
    if (!c->isReady()) { withLspReady(c, [this]() { showCallHierarchy(); }); return; }
    const QString sym = lspWordUnderCursor(e);
    QTextCursor cur = e->textCursor();
    const QString path = e->filePath();
    const int line = cur.blockNumber(), col = cur.columnNumber();
    c->requestCallPrepare(path, line, col, [this, sym, c](QJsonObject res) {
        QList<CallNode> roots = CallHierarchyTree::parsePrepare(res);
        if (roots.isEmpty()) { m_status->setText("Çağrı öğesi bulunamadı."); return; }
        auto* d = new CallHierarchyDialog(sym, this);
        d->setAttribute(Qt::WA_DeleteOnClose);
        const QJsonObject item = CallHierarchyTree::itemParams(roots.first());
        c->requestCallIncoming(item, [d](QJsonObject r) {
            d->setIncoming(CallHierarchyTree::parseCalls(r, true));
        });
        c->requestCallOutgoing(item, [d](QJsonObject r) {
            d->setOutgoing(CallHierarchyTree::parseCalls(r, false));
        });
        connect(d, &CallHierarchyDialog::jumpRequested, this, [this](const CallNode& n) {
            QString p = n.uri.startsWith("file:") ? LspClient::uriToPath(n.uri) : n.uri;
            if (!p.isEmpty()) {
                openFileAt(p, n.line + 1);
                if (auto* ne = currentEditor()) ne->flashLine(n.line);
            } else if (auto* ce = currentEditor()) {
                ce->gotoLine(n.line + 1);
                ce->flashLine(n.line);
            }
        });
        connect(d, &CallHierarchyDialog::expandIncoming, this,
                [c](const CallNode& n, QTreeWidgetItem* item) {
                    c->requestCallIncoming(CallHierarchyTree::itemParams(n),
                        [item](QJsonObject r) {
                            const int depth = item->data(0, Qt::UserRole + 4).toInt() + 1;
                            for (const CallNode& ch : CallHierarchyTree::parseCalls(r, true)) {
                                auto* it = new QTreeWidgetItem(item);
                                it->setText(0, ch.name);
                                it->setData(0, Qt::UserRole, ch.name);
                                it->setData(0, Qt::UserRole + 1, ch.uri);
                                it->setData(0, Qt::UserRole + 2, ch.line);
                                it->setData(0, Qt::UserRole + 4, depth); // Stage 24
                                it->addChild(new QTreeWidgetItem(it, QStringList("…")));
                            }
                            if (item->childCount() > 0
                                && item->child(0)->text(0) == "…")
                                delete item->takeChild(0);
                            if (item->childCount() == 0) // Stage 24: boş sonuç işareti
                                item->addChild(new QTreeWidgetItem(item, QStringList("(yok)")));
                        });
                });
        connect(d, &CallHierarchyDialog::expandOutgoing, this,
                [c](const CallNode& n, QTreeWidgetItem* item) {
                    c->requestCallOutgoing(CallHierarchyTree::itemParams(n),
                        [item](QJsonObject r) {
                            const int depth = item->data(0, Qt::UserRole + 4).toInt() + 1;
                            for (const CallNode& ch : CallHierarchyTree::parseCalls(r, false)) {
                                auto* it = new QTreeWidgetItem(item);
                                it->setText(0, ch.name);
                                it->setData(0, Qt::UserRole, ch.name);
                                it->setData(0, Qt::UserRole + 1, ch.uri);
                                it->setData(0, Qt::UserRole + 2, ch.line);
                                it->setData(0, Qt::UserRole + 4, depth); // Stage 24
                                it->addChild(new QTreeWidgetItem(it, QStringList("…")));
                            }
                            if (item->childCount() > 0
                                && item->child(0)->text(0) == "…")
                                delete item->takeChild(0);
                            if (item->childCount() == 0) // Stage 24: boş sonuç işareti
                                item->addChild(new QTreeWidgetItem(item, QStringList("(yok)")));
                        });
                });
        d->show();
    });
}

// ---------- Stage 14: hata ayıklama & test ----------

void MainWindow::debugStart() {
    AppSettings s = SettingsManager::instance().load();
    LaunchConfig lc = LaunchConfig::load(m_root);
    // Stage 28: tek seferlik geçersiz kılma (çalıştırılabilir dosyayı çalıştır)
    const bool ovr = m_debugOverride;
    if (ovr) {
        lc = m_overrideLc;
        m_debugOverride = false;
    }
    if (!ovr && (lc.program.trimmed().isEmpty()
                 || !QFile::exists(lc.resolvedProgram(m_root)))) {
        // launch.json yoksa/bozuksa oluşturup aç
        LaunchConfig d = LaunchConfig::defaults(m_root);
        if (lc.program.trimmed().isEmpty()) lc = d;
        lc.save(m_root);
        openFileAt(LaunchConfig::configPath(m_root), 1);
        toast(2, "launch.json düzenleyin (program yolu), sonra tekrar başlatın.");
        return;
    }
    if (!lc.preBuild.trimmed().isEmpty() && !m_preBuildDone) {
        // Stage 26: ön derleme bitmeden başlama; hatalıysa dur
        m_termDock->setVisible(true);
        m_bottomTabs->setCurrentWidget(m_taskPanel);
        auto* runner = new TaskRunner(this);
        if (!runner->loadForRoot(m_root)) {
            toast(2, "Ön derleme görevi bulunamadı: " + lc.preBuild);
            runner->deleteLater();
            return;
        }
        connect(runner, &TaskRunner::finished, this,
                [this, runner, pre = lc.preBuild](const QString&, int code) {
                    runner->deleteLater();
                    if (code != 0) {
                        toast(2, QString("Ön derleme başarısız (%1) — başlatılmadı").arg(code));
                        return;
                    }
                    m_preBuildDone = true;
                    debugStart();
                    m_preBuildDone = false;
                });
        toast(0, "Ön derleme çalışıyor: " + lc.preBuild);
        runner->runLabel(lc.preBuild, m_root);
        return;
    }
    m_termDock->setVisible(true);
    m_bottomTabs->setCurrentWidget(m_debug);
    m_debug->clearOutput();
    m_debug->setRunning(true, false);
    m_status->setText("Hata ayıklama başlıyor: " + lc.program);
    // GDB'yi başlat (gerekirse) → programı yükle
    if (!m_gdb->isRunning() && !m_gdb->start(s.gdbPath)) {
        m_debug->setRunning(false, false);
        return;
    }
    m_gdbBpNums.clear();
    // Stage 26: şema dışı anahtar uyarısı
    const QStringList unk = LaunchConfig::unknownKeys(m_root);
    if (!unk.isEmpty())
        toast(2, "launch.json: bilinmeyen anahtar (" + unk.join(", ") + ") yoksayıldı");
    const QString curFile = currentEditor() ? currentEditor()->filePath() : QString();
    m_gdb->launch(lc.resolvedProgram(m_root), lc.resolvedArgs(m_root, curFile),
                  lc.resolvedCwd(m_root), lc.stopAtEntry, [this](bool ok) {
                      if (!ok) {
                          m_debug->setRunning(false, false);
                          return;
                      }
                      syncBreakpointsToGdb();
                      // Stage 26: kayıtlı fonksiyon kesmeleri
                      for (const QString& fn :
                           QSettings("Verso", "VersoCoder").value("debug/funcBps").toStringList())
                          m_gdb->breakFunction(fn, nullptr);
                      m_debugT0.start(); // Stage 26: çıkış özeti
                      m_status->setText("Program çalışıyor (gdb).");
                  });
}

void MainWindow::syncBreakpointsToGdb() {
    if (!m_gdb || !m_gdb->isReady()) return;
    BreakpointStore store;
    for (const Breakpoint& b : store.all()) {
        if (!b.enabled || b.isLogPoint()) continue;
        const QString key = QString("%1:%2").arg(b.file).arg(b.line);
        if (m_gdbBpNums.contains(key)) continue;
        const int hits = b.hitCount;
        m_gdb->breakInsert(b.file, b.line, b.condition, [this, key, hits](QString num) {
            if (num.isEmpty()) return;
            m_gdbBpNums[key] = num;
            if (hits > 0 && m_gdb) m_gdb->breakAfter(num, hits); // Stage 26
        });
    }
    if (auto* e = currentEditor())
        m_debug->setBreakpoints(BreakpointStore().forFile(e->filePath()), e->filePath());
}

void MainWindow::debugStop() {
    clearFrameMarks();
    if (m_gdb) m_gdb->quit();
    m_debug->setRunning(false, false);
    m_status->setText("Hata ayıklama durduruldu.");
}

void MainWindow::debugContinue() {
    if (m_gdb && m_gdb->isDebugging()) {
        clearFrameMarks();
        m_gdb->execContinue();
        m_debug->setRunning(true, false);
    }
}

void MainWindow::debugNext() {
    if (m_gdb && m_gdb->isDebugging()) {
        clearFrameMarks();
        m_gdb->execNext();
    }
}

void MainWindow::debugStep() {
    if (m_gdb && m_gdb->isDebugging()) {
        clearFrameMarks();
        m_gdb->execStep();
    }
}

void MainWindow::debugFinish() {
    if (m_gdb && m_gdb->isDebugging()) {
        clearFrameMarks();
        m_gdb->execFinish();
    }
}

void MainWindow::toggleBreakpoint(const QString& file, int line) {
    const QString path = file.isEmpty() ? (currentEditor() ? currentEditor()->filePath() : QString())
                                        : file;
    if (path.isEmpty() || line <= 0) return;
    BreakpointStore store;
    const bool removed = store.toggle(path, line);
    const QString key = QString("%1:%2").arg(path).arg(line);
    // Çalışan oturumla eşitle
    if (m_gdb && m_gdb->isDebugging() && m_gdb->isReady()) {
        if (removed) {
            if (m_gdbBpNums.contains(key)) {
                m_gdb->breakDelete(m_gdbBpNums[key]);
                m_gdbBpNums.remove(key);
            }
        } else {
            m_gdb->breakInsert(path, line, QString(), [this, key](QString num) {
                if (!num.isEmpty()) m_gdbBpNums[key] = num;
            });
        }
    }
    applyBpMarksAll();
    if (auto* e = currentEditor())
        m_debug->setBreakpoints(store.forFile(e->filePath()), e->filePath());
    m_status->setText(removed ? QString("Kesme kaldırıldı: %1:%2").arg(path).arg(line)
                              : QString("Kesme eklendi: %1:%2").arg(path).arg(line));
}

void MainWindow::toggleBreakpointAtCursor() {
    if (auto* e = currentEditor())
        toggleBreakpoint(e->filePath(), e->textCursor().blockNumber() + 1);
}

void MainWindow::applyBpMarks(CodeEditor* e) {
    if (!e || e->filePath().isEmpty()) return;
    QSet<int> lines;
    for (const Breakpoint& b : BreakpointStore().forFile(e->filePath()))
        if (b.enabled) lines << b.line;
    e->setBreakpoints(lines);
}

void MainWindow::applyBpMarksAll() {
    for (CodeEditor* e : allEditors()) applyBpMarks(e);
}

void MainWindow::applyCoverageMarks(CodeEditor* e) {
    if (!e || e->filePath().isEmpty()) return;
    if (m_coverage.contains(e->filePath())) e->setCoverage(m_coverage[e->filePath()]);
}

void MainWindow::clearFrameMarks() {
    m_hasFrame = false;
    for (CodeEditor* e : allEditors()) e->setFrameLine(-1);
}

void MainWindow::onDebugStopped(const QString& reason, const DebugFrame& frame) {
    m_curFrame = frame;
    m_hasFrame = !frame.file.isEmpty() && frame.line >= 0;
    m_debug->setRunning(true, true);
    m_termDock->setVisible(true);
    m_bottomTabs->setCurrentWidget(m_debug);
    if (m_hasFrame) {
        openFileAt(frame.file, frame.line + 1);
        if (auto* e = currentEditor()) {
            // Doğru dosya mı? (inline/başka dosya olabilir)
            bool same = QFileInfo(e->filePath()) == QFileInfo(frame.file);
            for (CodeEditor* ed : allEditors()) {
                if (QFileInfo(ed->filePath()) == QFileInfo(frame.file))
                    ed->setFrameLine(frame.line);
                else ed->setFrameLine(-1);
            }
            Q_UNUSED(same);
            e->flashLine(frame.line);
        }
        m_status->setText(QString("Durdu (%1): %2:%3").arg(reason, frame.func)
                              .arg(frame.line + 1));
    } else {
        m_status->setText("Durdu (" + reason + ")");
    }
    onDebugSmartCheck(); // Stage 26: std:: içindeyse otomatik üstten
    // Yığın + değişkenleri tazele (Stage 26: lib filtreli + tüm sekmeler)
    if (m_gdb) {
        m_gdb->stackFrames([this](QList<DebugFrame> frames) {
            if (m_debugSkipLib) {
                frames.erase(std::remove_if(frames.begin(), frames.end(),
                                            [](const DebugFrame& f) {
                                                return f.func.startsWith("std::") ||
                                                       f.file.contains("/usr/include/");
                                            }),
                             frames.end());
            }
            m_debug->setFrames(frames);
        });
        refreshDebugVars(0);
        refreshWatches();
        m_gdb->registers([this](QList<QPair<QString, QString>> regs) {
            m_debug->setRegisters(regs);
        });
        m_gdb->threadList([this](QList<GdbDriver::ThreadInfo> ts) {
            QList<QPair<QString, QString>> items;
            QString cur;
            for (const auto& t : ts) {
                items << qMakePair(t.id, QString("%1: %2 %3").arg(t.id, t.target, t.name));
                Q_UNUSED(cur);
            }
            m_debug->setThreads(items, m_curThread);
        });
        // Disas: geçerli kare dosya+satırındaysa
        if (m_hasFrame && !m_curFrame.file.isEmpty())
            m_gdb->disassemble(m_curFrame.file, m_curFrame.line + 1, 30,
                               [this](QList<QMap<QString, QString>> rows) {
                                   m_debug->setDisas(rows);
                               });
    }
}

void MainWindow::refreshDebugVars(int frame) {
    if (!m_gdb || !m_gdb->isDebugging()) return;
    m_gdb->stackVariables(frame, [this](QList<DebugVar> vars) {
        m_debug->setVariables(vars);
    });
}

void MainWindow::refreshDebugVars() {
    refreshDebugVars(0);
}

void MainWindow::onDebugExited(int code) {
    clearFrameMarks();
    m_debug->setRunning(false, false);
    m_gdbBpNums.clear();
    m_watchNames.clear();
    // Stage 26: çıkış özeti (kod + süre)
    const qint64 ms = m_debugT0.isValid() ? m_debugT0.elapsed() : -1;
    const QString dur = ms < 0 ? "" : QString(" • %1 sn").arg(ms / 1000.0, 0, 'f', 1);
    m_status->setText(code == 0 ? "Program normal çıktı (0)." + dur
                                : QString("Program çıktı (%1).").arg(code) + dur);
    toast(code == 0 ? 1 : 2, m_status->text());
}

// Stage 26: akıllı adım — std:: içine girerse otomatik üstten devam (max 30)
void MainWindow::debugSmartStep() {
    if (!m_gdb || !m_gdb->isDebugging()) return;
    m_smartDepth = 0;
    m_gdb->execStep();
}

void MainWindow::onDebugSmartCheck() {
    if (m_smartDepth < 0 || !m_gdb) return;
    const bool inLib = m_curFrame.func.startsWith("std::") ||
                       m_curFrame.file.contains("/usr/include/") ||
                       m_curFrame.func.startsWith("__");
    if (inLib && m_smartDepth < 30) {
        ++m_smartDepth;
        m_gdb->execNext();
    } else {
        m_smartDepth = -1;
    }
}

// Stage 26: izleme panelini tazele (varobj güncelle + değerleri oku)
void MainWindow::refreshWatches() {
    if (!m_gdb || !m_gdb->isDebugging()) return;
    if (m_watchNames.isEmpty()) {
        m_debug->setWatches({});
        return;
    }
    m_gdb->watchUpdateAll([this](QVariantList) {
        const QStringList exprs = m_watchNames.keys();
        auto* pending = new int(exprs.size());
        auto* vals = new QMap<QString, QString>();
        if (exprs.isEmpty()) {
            delete pending;
            delete vals;
            m_debug->setWatches({});
            return;
        }
        for (const QString& ex : exprs) {
            m_gdb->watchEvaluate(m_watchNames[ex], [this, ex, pending, vals](QString v) {
                (*vals)[ex] = v;
                if (--(*pending) == 0) {
                    QList<QPair<QString, QString>> items;
                    for (auto it = vals->constBegin(); it != vals->constEnd(); ++it)
                        items << qMakePair(it.key(), it.value());
                    m_debug->setWatches(items);
                    delete pending;
                    delete vals;
                }
            });
        }
    });
}

void MainWindow::debugAddWatch(const QString& expr) {
    if (!m_gdb || !m_gdb->isDebugging()) {
        toast(0, "Önce hata ayıklamayı başlatın");
        return;
    }
    m_gdb->watchCreate(expr, [this, expr](QString name) {
        if (name.isEmpty()) {
            toast(2, "İzleme eklenemedi: " + expr.left(60));
            return;
        }
        m_watchNames[expr] = name;
        refreshWatches();
    });
}

void MainWindow::debugRemoveWatch(const QString& expr) {
    if (m_watchNames.contains(expr) && m_gdb) m_gdb->watchDelete(m_watchNames[expr]);
    m_watchNames.remove(expr);
    refreshWatches();
}

void MainWindow::debugReadMemory(const QString& addr) {
    if (!m_gdb || !m_gdb->isDebugging()) return;
    m_gdb->memoryRead(addr, 64, [this, addr](QString dump) {
        m_debug->setMemory(addr, dump.isEmpty() ? "(okunamadı)" : dump);
    });
}

void MainWindow::debugSelectThread(const QString& id) {
    if (!m_gdb || !m_gdb->isDebugging() || id.isEmpty()) return;
    m_curThread = id;
    m_gdb->threadSelect(id);
    refreshDebugVars(0);
}

void MainWindow::debugSetVariable(const QString& expr) {
    if (!m_gdb || !m_gdb->isDebugging()) return;
    bool ok = false;
    const QString v = QInputDialog::getText(this, "Değeri Değiştir",
                                            expr + " =", QLineEdit::Normal, QString(), &ok);
    if (!ok) return;
    m_gdb->setVariable(expr, v, [this, expr](bool good) {
        toast(good ? 1 : 2, good ? "Atandı: " + expr : "Atama başarısız");
        refreshDebugVars(0);
        refreshWatches();
    });
}

// Stage 26: koşullu kesme düzenleyici (koşul + hit-count)
void MainWindow::editBreakpoint(const QString& file, int line) {
    BreakpointStore store;
    const QString path = file.isEmpty() && currentEditor() ? currentEditor()->filePath() : file;
    if (path.isEmpty() || line <= 0) return;
    if (!store.has(path, line)) {
        toggleBreakpoint(path, line);
        if (!store.has(path, line)) return;
    }
    bool ok = false;
    QString cond;
    for (const Breakpoint& b : store.forFile(path))
        if (b.line == line) cond = b.condition;
    cond = QInputDialog::getText(this, "Koşullu Kesme", "Koşul (boşsa her zaman):",
                                 QLineEdit::Normal, cond, &ok);
    if (!ok) return;
    const QString key = QString("%1:%2").arg(path).arg(line);
    store.setCondition(path, line, cond.trimmed());
    // gdb tarafını güncelle (numara biliniyorsa koşulu işlet)
    if (m_gdb && m_gdb->isDebugging() && m_gdbBpNums.contains(key))
        m_gdb->breakCondition(m_gdbBpNums[key], cond.trimmed());
    int hits = 0;
    for (const Breakpoint& b : store.forFile(path))
        if (b.line == line) hits = b.hitCount;
    hits = QInputDialog::getInt(this, "Koşullu Kesme", "Kaç vuruşta bir dur (0=her sefer):",
                                hits, 0, 1000000, 1, &ok);
    if (!ok) return;
    store.setHitCount(path, line, hits);
    if (m_gdb && m_gdb->isDebugging() && m_gdbBpNums.contains(key) && hits > 0)
        m_gdb->breakAfter(m_gdbBpNums[key], hits);
    applyBpMarksAll();
    if (auto* e = currentEditor()) m_debug->setBreakpoints(store.forFile(e->filePath()), e->filePath());
    toast(1, "Kesme noktası güncellendi");
}

// Stage 26: fonksiyon kesme noktası (kalıcı liste)
void MainWindow::debugFunctionBp() {
    bool ok = false;
    const QString fn = QInputDialog::getText(this, "Fonksiyon Kesmesi", "Fonksiyon adı:",
                                             QLineEdit::Normal, QString(), &ok);
    if (!ok || fn.trimmed().isEmpty()) return;
    QSettings q("Verso", "VersoCoder");
    QStringList fns = q.value("debug/funcBps").toStringList();
    if (!fns.contains(fn.trimmed())) {
        fns << fn.trimmed();
        q.setValue("debug/funcBps", fns);
    }
    if (m_gdb && m_gdb->isDebugging())
        m_gdb->breakFunction(fn.trimmed(), [this, fn](QString num) {
            toast(num.isEmpty() ? 2 : 1, num.isEmpty() ? "Kesme konulamadı"
                                                       : "Fonksiyon kesmesi #" + num + ": " + fn);
        });
    else toast(0, "Kaydedildi — sonraki oturumda kurulacak: " + fn.trimmed());
}

// Stage 26: izleme noktası (watchpoint)
void MainWindow::debugWatchpoint() {
    bool ok = false;
    const QString ex = QInputDialog::getText(this, "İzleme Noktası", "İfade:",
                                             QLineEdit::Normal, QString(), &ok);
    if (!ok || ex.trimmed().isEmpty()) return;
    const QString acc = QInputDialog::getItem(this, "İzleme Noktası", "Erişim:",
                                              {"yazma", "okuma", "okuma+yazma"}, 0,
                                              false, &ok);
    if (!ok) return;
    const QString mode = acc.startsWith("okuma+y") ? "rw" : (acc.startsWith("okuma") ? "r" : "");
    if (!m_gdb || !m_gdb->isDebugging()) {
        toast(0, "Önce hata ayıklamayı başlatın");
        return;
    }
    m_gdb->breakWatch(ex.trimmed(), mode, [this, ex](QString num) {
        toast(num.isEmpty() ? 2 : 1, num.isEmpty() ? "İzleme konulamadı"
                                                   : "İzleme #" + num + ": " + ex);
    });
}

// Stage 26: çalışan sürece bağlan (pid listesi)
void MainWindow::debugAttach() {
    QProcess ps;
    ps.start("ps", {"-eo", "pid,comm", "--sort=pid"});
    if (!ps.waitForFinished(5000)) { toast(3, "ps çalışmadı"); return; }
    QStringList items;
    for (const QString& ln : QString::fromUtf8(ps.readAllStandardOutput()).split('\n')) {
        const QString t = ln.trimmed();
        if (!t.isEmpty() && t[0].isDigit()) items << t;
    }
    if (items.isEmpty()) { toast(0, "Süreç listelenemedi"); return; }
    bool ok = false;
    const QString sel = QInputDialog::getItem(this, "Sürece Bağlan", "PID:", items, 0,
                                              true, &ok);
    if (!ok) return;
    const int pid = sel.section(' ', 0, 0).toInt();
    if (pid <= 0) return;
    AppSettings s = SettingsManager::instance().load();
    if (!m_gdb->isRunning() && !m_gdb->start(s.gdbPath)) return;
    m_termDock->setVisible(true);
    m_bottomTabs->setCurrentWidget(m_debug);
    m_debug->setRunning(true, false);
    m_gdb->attach(pid, [this, pid](bool good) {
        if (!good) {
            toast(3, "Bağlanılamadı (yetki? ptrace_scope?)");
            m_debug->setRunning(false, false);
            return;
        }
        m_debugT0.start();
        m_status->setText(QString("Bağlanıldı: pid %1").arg(pid));
    });
}

// Stage 26: core dump ile aç
void MainWindow::debugOpenCore() {
    const QString prog = QFileDialog::getOpenFileName(this, "Çalıştırılabilir dosya", m_root);
    if (prog.isEmpty()) return;
    const QString core = QFileDialog::getOpenFileName(this, "Core dosyası",
                                                      QFileInfo(prog).absolutePath(),
                                                      "Core (*)");
    if (core.isEmpty()) return;
    AppSettings s = SettingsManager::instance().load();
    if (!m_gdb->isRunning() && !m_gdb->start(s.gdbPath)) return;
    m_termDock->setVisible(true);
    m_bottomTabs->setCurrentWidget(m_debug);
    m_debug->setRunning(true, false);
    m_gdb->openCore(prog, core, [this](bool good) {
        if (!good) {
            toast(3, "Core açılamadı");
            m_debug->setRunning(false, false);
            return;
        }
        m_debugT0.start();
        m_gdb->stackFrames([this](QList<DebugFrame> frames) {
            m_debug->setFrames(frames);
            if (!frames.isEmpty() && !frames.first().file.isEmpty()) {
                openFileAt(frames.first().file, frames.first().line + 1);
                m_status->setText("Core: " + frames.first().func);
            }
        });
        refreshDebugVars(0);
    });
}

// Stage 26: kaynak eşleme (substitute-path)
void MainWindow::debugSubstitutePath() {
    bool ok = false;
    const QString from = QInputDialog::getText(this, "Kaynak Eşleme", "Derlemedeki yol:",
                                               QLineEdit::Normal, QString(), &ok);
    if (!ok || from.trimmed().isEmpty()) return;
    const QString to = QInputDialog::getText(this, "Kaynak Eşleme", "Gerçek yol:",
                                             QLineEdit::Normal, m_root, &ok);
    if (!ok) return;
    if (m_gdb && m_gdb->isRunning()) {
        m_gdb->substitutePath(from, to);
        toast(1, "Eşlendi: " + from + " → " + to);
    } else toast(0, "Önce gdb başlatın (oturumda uygulanır)");
}

void MainWindow::debugEvaluate(const QString& expr) {
    if (!m_gdb || !m_gdb->isDebugging()) {
        m_debug->appendOutput("(hata ayıklama yok)\n", true);
        return;
    }
    m_gdb->evaluate(expr, [this, expr](QString v) {
        m_debug->appendOutput(QString("%1 = %2\n").arg(expr, v.isEmpty() ? "?" : v));
    });
}

void MainWindow::debugConsole(const QString& cmd) {
    if (!m_gdb || !m_gdb->isRunning()) {
        m_debug->appendOutput("(gdb çalışmıyor)\n", true);
        return;
    }
    m_debug->appendOutput("(gdb) " + cmd + "\n");
    if (cmd.startsWith('-')) {
        m_gdb->command(cmd, [this](QVariantMap r) {
            m_debug->appendOutput(QJsonDocument::fromVariant(r).toJson(QJsonDocument::Compact)
                                  + "\n");
        });
    } else {
        QString c = cmd;
        m_gdb->command("-interpreter-exec console \"" + c.replace('"', "\\\"") + "\"",
                       [this](QVariantMap) {});
    }
}

// ---------- Stage 14: test gezgini ----------

void MainWindow::discoverTests() {
    const QList<TestRunner> runners = TestDiscovery::findRunners(m_root);
    if (runners.isEmpty()) {
        m_status->setText("Test çalıştırıcı bulunamadı (ctest/gtest/pytest/unittest).");
        toast(2, "Test çalıştırıcı bulunamadı.");
        return;
    }
    m_testRunner = runners.first();
    m_hasRunner = true;
    m_termDock->setVisible(true);
    m_bottomTabs->setCurrentWidget(m_tests);
    m_status->setText(QString("Testler listeleniyor (%1)...").arg(m_testRunner.kind));
    QProcess* p = new QProcess(this);
    const QStringList cmd = TestDiscovery::listCommand(m_testRunner);
    p->setWorkingDirectory(m_testRunner.kind == "ctest" ? m_testRunner.path : m_root);
    connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, p](int, QProcess::ExitStatus) {
                const QString out = QString::fromUtf8(p->readAllStandardOutput())
                    + QString::fromUtf8(p->readAllStandardError());
                p->deleteLater();
                if (m_testRunner.kind == "unittest") {
                    // unittest listesi statik taramadan (işlem çıktısı kullanılmaz)
                    m_testCases = TestDiscovery::parseList(m_testRunner, QString());
                } else {
                    m_testCases = TestDiscovery::parseList(m_testRunner, out);
                }
                m_tests->setTests(m_testCases);
                m_status->setText(QString("%1 test bulundu (%2).")
                                      .arg(m_testCases.size())
                                      .arg(m_testRunner.kind));
            });
    p->start(cmd.first(), cmd.mid(1));
}

static QList<TestCase> parseTestOutput(const TestRunner& r, const QString& out) {
    if (r.kind == "gtest") return TestParser::parseGtest(out);
    if (r.kind == "pytest" || r.kind == "unittest") return TestParser::parsePytest(out);
    return TestParser::parseCtest(out);
}

void MainWindow::runAllTests() {
    if (!m_hasRunner) { discoverTests(); return; }
    runOneTest(QString());
}

void MainWindow::runOneTest(const QString& testId) {
    if (!m_hasRunner) { discoverTests(); return; }
    if (m_testProc && m_testProc->state() != QProcess::NotRunning) {
        m_status->setText("Test çalışıyor, önce durdurun.");
        return;
    }
    m_termDock->setVisible(true);
    m_bottomTabs->setCurrentWidget(m_tests);
    m_tests->setRunning(true);
    m_tests->clearLog();
    m_tests->appendLog(QString("\n$ %1\n")
                           .arg(TestDiscovery::runCommand(m_testRunner, m_root, testId)
                                    .join(' ')));
    m_testProc = new QProcess(this);
    const QStringList cmd = TestDiscovery::runCommand(m_testRunner, m_root, testId);
    m_testProc->setWorkingDirectory(m_testRunner.kind == "ctest" ? m_testRunner.path : m_root);
    connect(m_testProc, &QProcess::readyReadStandardOutput, this, [this]() {
        m_tests->appendLog(QString::fromUtf8(m_testProc->readAllStandardOutput()));
    });
    connect(m_testProc, &QProcess::readyReadStandardError, this, [this]() {
        m_tests->appendLog(QString::fromUtf8(m_testProc->readAllStandardError()));
    });
    connect(m_testProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, testId](int code, QProcess::ExitStatus) {
                m_tests->setRunning(false);
                const QString log = m_tests->logText();
                const QList<TestCase> res = parseTestOutput(m_testRunner, log);
                if (!res.isEmpty()) m_tests->setResults(res);
                int pass = 0, fail = 0;
                for (const TestCase& t : res) {
                    if (t.status == "pass") ++pass;
                    else if (t.status == "fail") ++fail;
                }
                m_tests->appendLog(QString("\n[test bitti: çıkış %1]\n").arg(code));
                applyBuildProblems(log, "test");
                m_status->setText(testId.isEmpty()
                    ? QString("Testler: %1 geçti, %2 kaldı.").arg(pass).arg(fail)
                    : QString("%1: %2").arg(testId, fail ? "KALDI" : "GEÇTİ"));
                m_testProc->deleteLater();
                m_testProc = nullptr;
            });
    m_testProc->start(cmd.first(), cmd.mid(1));
}

QString MainWindow::testLog() const {
    return m_tests ? m_tests->logText() : QString();
}

void MainWindow::stopTests() {
    if (m_testProc && m_testProc->state() != QProcess::NotRunning) {
        m_testProc->kill();
        m_tests->setRunning(false);
        m_status->setText("Test durduruldu.");
    }
}

// ---------- Stage 14: kapsama (gcov) ----------

void MainWindow::runCoverage() {
    // .gcda dosyalarını bul (önce programı çalıştırarak üret)
    LaunchConfig lc = LaunchConfig::load(m_root);
    const QString prog = lc.resolvedProgram(m_root);
    if (!QFile::exists(prog)) {
        toast(2, "Program bulunamadı (launch.json → program). Önce derleyin.");
        return;
    }
    m_status->setText("Kapsama için program çalıştırılıyor...");
    QProcess* run = new QProcess(this);
    run->setWorkingDirectory(lc.resolvedCwd(m_root));
    connect(run, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, run](int, QProcess::ExitStatus) {
                run->deleteLater();
                collectCoverage();
            });
    run->start(prog, lc.args);
    if (!run->waitForStarted(5000)) {
        run->deleteLater();
        toast(3, "Program başlatılamadı: " + prog);
    }
}

void MainWindow::collectCoverage() {
    // Kök altındaki .gcda'lar → gcov --stdout → ısı haritası
    QStringList gcdas;
    QDirIterator it(m_root, {"*.gcda"}, QDir::Files, QDirIterator::Subdirectories);
    int guard = 0;
    while (it.hasNext() && guard++ < 60) gcdas << it.next();
    if (gcdas.isEmpty()) {
        toast(2, "Kapsama verisi yok (.gcda bulunamadı). --coverage ile derleyin.");
        return;
    }
    m_coverage.clear();
    int files = 0;
    double sum = 0;
    QSet<QString> seenSrc;
    for (const QString& gcda : std::as_const(gcdas)) {
        const QDir objDir = QFileInfo(gcda).dir();
        const QString stem = QFileInfo(gcda).completeBaseName();
        // Kaynağı bul (aynı dizin + kök taraması)
        QString src;
        for (const QString& ext : {".cpp", ".c", ".cc", ".cxx"}) {
            if (QFile::exists(objDir.absoluteFilePath(stem + ext))) {
                src = objDir.absoluteFilePath(stem + ext);
                break;
            }
        }
        if (src.isEmpty()) {
            QDirIterator sit(m_root, {stem + ".cpp", stem + ".c", stem + ".cc"},
                             QDir::Files, QDirIterator::Subdirectories);
            if (sit.hasNext()) src = sit.next();
        }
        if (src.isEmpty() || seenSrc.contains(src)) continue;
        seenSrc << src;
        QProcess gcov;
        gcov.setWorkingDirectory(QDir::tempPath());
        gcov.start("gcov", {"--stdout", "-o", objDir.absolutePath(), src});
        if (!gcov.waitForFinished(15000)) continue;
        const GcovFile gf = GcovParser::parse(
            QString::fromUtf8(gcov.readAllStandardOutput()));
        if (gf.coverable() == 0) continue;
        // Editör dosyasıyla eşleştir (tam yol ya da dosya adı)
        QString key = gf.source.isEmpty() ? src : gf.source;
        if (!QFile::exists(key)) {
            // gcov Source: satırı göreli olabilir — dosya adıyla eşleştir
            key = src;
        }
        QMap<int, int>& m = m_coverage[key];
        for (const GcovLine& l : gf.lines)
            if (l.hits >= 0) {
                if (!m.contains(l.line)) m[l.line] = (int)l.hits;
                else m[l.line] = qMax(m[l.line], (int)l.hits);
            }
        sum += gf.percent();
        ++files;
    }
    // Açık editörlere uygula (tam yol + dosya adı eşleşmesi)
    for (CodeEditor* e : allEditors()) {
        if (m_coverage.contains(e->filePath())) { applyCoverageMarks(e); continue; }
        for (auto it = m_coverage.begin(); it != m_coverage.end(); ++it) {
            if (QFileInfo(it.key()).fileName() == QFileInfo(e->filePath()).fileName()) {
                e->setCoverage(it.value());
                break;
            }
        }
    }
    if (files == 0) {
        toast(2, "Kapsama ayrıştırılamadı (gcov çıktısı boş).");
        return;
    }
    toast(1, QString("Kapsama: %%%1 (%2 dosya)").arg(sum / files, 0, 'f', 1).arg(files));
    m_status->setText(QString("Kapsama %%%1 — yeşil=çalıştı, kırmızı=kapsanmıyor.")
                          .arg(sum / files, 0, 'f', 1));
}

void MainWindow::editLaunchConfig() {
    const QString p = LaunchConfig::configPath(m_root);
    if (!QFile::exists(p)) {
        LaunchConfig d = LaunchConfig::defaults(m_root);
        d.save(m_root);
    }
    openFileAt(p, 1);
}

// ---------- Stage 14: derleme/test sorunları → Sorunlar ----------

void MainWindow::applyBuildProblems(const QString& output, const QString& source) {
    m_buildDiags.clear();
    for (const LspDiag& d : ProblemMatcher::match(output, source)) {
        QString path = d.path;
        if (!QDir::isAbsolutePath(path))
            path = QDir(m_root).absoluteFilePath(path);
        if (!QFile::exists(path)) {
            // Kök altında aynı adlı dosya ara (derleyiciler göreli yazar)
            QDirIterator it(m_root, {QFileInfo(path).fileName()}, QDir::Files,
                            QDirIterator::Subdirectories);
            if (it.hasNext()) path = it.next();
            else continue;
        }
        LspDiag dd = d;
        dd.path = QFileInfo(path).absoluteFilePath();
        m_buildDiags[dd.path] << dd;
    }
    // Boş olmayanları koru, diğer dosyaların eski girdilerini temizle
    refreshProblemView();
    if (auto* e = currentEditor())
        if (m_buildDiags.contains(e->filePath()))
            e->setDiagnostics(mergedDiags3(m_diags, m_spellDiags, m_buildDiags,
                                           e->filePath()));
    int n = 0;
    for (auto it = m_buildDiags.begin(); it != m_buildDiags.end(); ++it) n += it.value().size();
    if (n > 0) {
        m_termDock->setVisible(true);
        m_status->setText(QString("%1 derleme/test sorunu (Sorunlar panelinde)").arg(n));
    }
}

// ---------- Stage 15: AI hattı ----------

static QString aiLangFor(const QString& path) {
    const QString s = QFileInfo(path).suffix().toLower();
    if (s == "py") return "Python";
    if (s == "js" || s == "ts") return "JavaScript";
    if (s == "sh") return "Bash";
    if (s == "c" || s == "cpp" || s == "h" || s == "hpp" || s == "cc") return "C++";
    return "kod";
}

void MainWindow::askAiTask(int taskId, const QString& system, const QString& prompt,
                            const QJsonObject& opts,
                            std::function<void(QString, QString)> done) {
    // Stage 37: tüm tek seferlik AI akışları (satır içi düzeltme, belge yorumu,
    // test, commit mesajı, açıklama, inceleme) buradan geçer → sağlayıcı,
    // yönlendirme, kota, sağlık, maliyet ve failover otomatik uygulanır.
    if (m_flowBusy) {
        m_status->setText("AI meşgul, bitmesini bekleyin.");
        return;
    }
    AppSettings s = SettingsManager::instance().load();
    m_flowBusy = true;
    m_status->setText(QString("AI düşünüyor (%1)...")
                          .arg(AiProfiles::label(AiTask(qBound(0, taskId, int(AiTask::Extract))))));
    if (!m_aiRunner) {
        m_aiRunner = new AiRunner(this);
        if (m_secrets) m_aiRunner->setSecretStore(m_secrets);
    }
    AiRunner::Options o =
        AiRunner::optionsFor(AiTask(qBound(0, taskId, int(AiTask::Extract))), system);
    if (opts.contains("temperature")) o.temperature = opts.value("temperature").toDouble();
    if (opts.contains("num_predict")) o.maxTokens = opts.value("num_predict").toInt();
    // Bağlam bütçesi: model penceresinin yarısı
    if (ContextBudget::exceeds(prompt, ContextBudget::maxFor(s.contextWindow)))
        toast(2, ContextBudget::warnText(prompt, ContextBudget::maxFor(s.contextWindow)));
    const QString safe = ContextBudget::trim(prompt, ContextBudget::maxFor(s.contextWindow));

    connect(m_aiRunner, &AiRunner::finished, this,
            [this, done](const AiRunner::Result& r) {
                m_flowBusy = false;
                if (r.ok) {
                    QString note = QString("AI yanıt verdi · %1 · %2ms")
                                       .arg(AiProfiles::label(AiTask::Chat))
                                       .arg(r.ms);
                    if (r.usd > 0.0) note += QString(" · ~$%1").arg(r.usd, 0, 'f', 4);
                    m_status->setText(note);
                    done(r.text, QString());
                } else {
                    m_status->setText("AI hatası: " + r.error.left(100));
                    toast(3, "AI hatası: " + r.error.left(150));
                    done(QString(), r.error);
                }
            },
            Qt::SingleShotConnection);
    m_aiRunner->runAsync(o, safe);
}

// Stage 37: sağlayıcı çipi — ad + sağlık noktası + kalan kota
void MainWindow::updateProviderChip() {
    if (!m_chipProvider) return;
    const ProviderSpec spec = ProviderPrefs::resolve();
    const QString id = spec.id;
    const QString dot = ProviderHealth::instance().entry(id).calls == 0
                            ? QStringLiteral("⚪")
                            : (ProviderHealth::instance().isHealthy(id) ? QStringLiteral("●")
                                                                         : QStringLiteral("●"));
    const QString color = ProviderHealth::instance().entry(id).calls == 0
                              ? QStringLiteral("#858585")
                              : (ProviderHealth::instance().isHealthy(id) ? QStringLiteral("#4ec9b0")
                                                                           : QStringLiteral("#f44747"));
    QString text = QString("%1 %2").arg(dot, spec.label);
    const UsageLedger::Quota q = UsageLedger::instance().quota(id);
    if (q.limited()) {
        const UsageLedger::Day d = UsageLedger::instance().today(id);
        const QString used = q.maxCalls > 0
                                 ? QString("%1/%2 çağrı").arg(d.calls).arg(q.maxCalls)
                                 : QString("%1 token").arg(d.total());
        text += QStringLiteral(" · %1").arg(used);
    }
    // Stage 40 B10: kota %80'de sararır + günde bir kez bildirilir
    double ratio = 0.0;
    if (q.limited()) {
        const UsageLedger::Day dd = UsageLedger::instance().today(id);
        ratio = q.maxCalls > 0 ? double(dd.calls) / double(qMax(1, q.maxCalls))
                               : double(dd.total()) / double(qMax(1, q.maxTokens));
    }
    QString chipColor = color;
    if (ratio >= 0.8 && ratio < 1.0) {
        chipColor = QStringLiteral("#cca700");
        text += QStringLiteral(" · kota %1%2 dolu").arg(int(ratio * 100));
        const QString warnKey =
            QString("quota/warned_%1_%2").arg(id, QDate::currentDate().toString(Qt::ISODate));
        if (!QSettings().value(warnKey, false).toBool()) {
            QSettings().setValue(warnKey, true);
            toast(1, QString("%1 kotası %2%3 doldu — yakında istekler reddedilir.")
                           .arg(spec.label)
                           .arg(int(ratio * 100)));
        }
    }
    m_chipProvider->setText(text);
    m_chipProvider->setStyleSheet(QStringLiteral("color:%1;").arg(chipColor));
    // Stage 40 B9: RAG rozeti (yalnız ilerleme/bekleme varken görünür)
    if (m_chipRag && m_ai) {
        const QString rag = m_ai->ragBadge();
        m_chipRag->setVisible(!rag.isEmpty());
        if (!rag.isEmpty()) {
            m_chipRag->setText(rag);
            m_chipRag->setStyleSheet(QStringLiteral("color:#cca700;"));
        }
    }
    m_chipProvider->setToolTip(
        QStringLiteral("%1 · %2\nSağlık: %3\nTıkla: sağlayıcı değiştir")
            .arg(spec.id, ProviderHealth::instance().statusLine(id))
            .arg(ProviderHealth::verdict(ProviderHealth::instance().entry(id))));
}

void MainWindow::showProviderMenu() {
    QMenu menu(this);
    menu.setTitle("AI sağlayıcısı");
    SecretStore store;
    const QString active = ProviderPrefs::activeProvider();
    for (const ProviderSpec& spec : ProviderRegistry::all()) {
        const bool hasKey = !spec.requiresKey() || !store.effectiveKey(spec.id).isEmpty();
        QAction* a = menu.addAction(
            QString("%1%2  ·  %3")
                .arg(spec.id == active ? QStringLiteral("● ") : QStringLiteral("   "), spec.label,
                     hasKey ? QStringLiteral("hazır") : QStringLiteral("anahtar yok")));
        a->setEnabled(true);
        a->setCheckable(true);
        a->setChecked(spec.id == active);
        connect(a, &QAction::triggered, this, [this, spec]() {
            ProviderPrefs::setActiveProvider(spec.id);
            updateProviderChip();
            toast(0, QString("AI sağlayıcısı: %1").arg(spec.label));
        });
    }
    menu.exec(m_chipProvider->mapToGlobal(QPoint(0, -menu.sizeHint().height())));
}

void MainWindow::cancelAi() {
    if (!m_flowBusy) return;
    if (m_aiRunner && m_aiRunner->cancelActive()) {
        m_flowBusy = false;
        m_status->setText("AI isteği iptal edildi.");
        toast(1, "AI isteği iptal edildi");
    } else {
        AiRunner::cancelAll();
        m_flowBusy = false;
        m_status->setText("AI iptal edildi.");
    }
}

void MainWindow::askAi(const QString& system, const QString& prompt,
                       const QJsonObject& opts,
                       std::function<void(QString, QString)> done) {
    askAiTask(int(AiTask::Chat), system, prompt, opts, std::move(done));
}

void MainWindow::toggleGhost() {
    AppSettings s = SettingsManager::instance().load();
    s.aiGhost = !s.aiGhost;
    SettingsManager::instance().save(s);
    if (!s.aiGhost) {
        if (m_ghostTimer) m_ghostTimer->stop();
        for (CodeEditor* e : allEditors()) e->clearGhost();
    }
    toast(0, s.aiGhost ? "Hayalet tamamlama açık (Tab kabul, Esc vazgeç)"
                       : "Hayalet tamamlama kapalı");
}

void MainWindow::requestGhost() {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty() || e->largeFileMode() || e->hasGhost()) return;
    AppSettings s = SettingsManager::instance().load();
    if (!s.aiGhost) return;
    QTextCursor c = e->textCursor();
    if (c.hasSelection()) return;
    const int pos = c.position();
    const QString full = e->toPlainText();
    const QString prefix = full.left(pos);
    const QString suffix = full.mid(pos);
    // Stage 37: hayalet tamamlama da sağlayıcı-duyarsız (yerel FIM / bulut önek)
    QSettings st;
    const bool cloudEnabled = st.value("ai/ghostCloud", false).toBool();
    const QString provId = ProviderPrefs::activeProvider();
    const GhostCompletion::Mode mode = GhostCompletion::modeFor(provId, cloudEnabled);
    if (mode == GhostCompletion::Mode::Off) return;
    const int req = ++m_ghostReq;
    m_ghostPrefix = prefix;
    m_ghostEditor = e;
    QPointer<CodeEditor> guard(e);
    const QString prompt = GhostCompletion::buildPrompt(mode, prefix, suffix,
                                                        aiLangFor(e->filePath()));
    if (ContextBudget::exceeds(prompt, 2048))
        m_status->setText(ContextBudget::warnText(prompt, 2048));

    // Hayalet kendi kısa ömürlü koşucusu (sohbet akışını meşgul etmez)
    if (!m_ghostRunner) {
        m_ghostRunner = new AiRunner(this);
        if (m_secrets) m_ghostRunner->setSecretStore(m_secrets);
    }
    AiRunner::Options o = AiRunner::optionsFor(AiTask::Fim,
                                              "Kısa kod tamamlama motoru. Sadece devam kodunu yaz.");
    o.allowFailover = false;  // hayalet gecikmesi kabul edilmez
    connect(m_ghostRunner, &AiRunner::finished, this,
            [this, req, guard, prefix](const AiRunner::Result& r) {
                if (req != m_ghostReq || !r.ok || !guard || guard != currentEditor()) return;
                const QString g = GhostCompletion::clean(r.text, prefix.split('\n').last());
                if (g.isEmpty()) return;
                const int p = guard->textCursor().position();
                if (guard->toPlainText().left(p) != m_ghostPrefix) return;
                guard->setGhostText(g, m_ghostPrefix);
            },
            Qt::SingleShotConnection);
    m_ghostRunner->runAsync(o, ContextBudget::trim(prompt, 2048));
}

void MainWindow::inlineEditSelection() {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    QTextCursor c = e->textCursor();
    QString sel = c.selectedText().replace(QChar(0x2029), "\n");
    int st = c.selectionStart(), ln = c.selectionEnd() - st;
    if (sel.trimmed().isEmpty()) {
        // Seçim yoksa geçerli satır
        c.select(QTextCursor::LineUnderCursor);
        sel = c.selectedText();
        st = c.selectionStart();
        ln = sel.size();
        e->setTextCursor(c);
    }
    bool ok = false;
    const QString instruction = QInputDialog::getText(
        this, "AI ile Yeniden Yaz", "Tarif:", QLineEdit::Normal, QString(), &ok);
    if (!ok || instruction.trimmed().isEmpty()) return;
    const QString path = e->filePath();
    askAiTask(int(AiTask::Explain), "Kısa kod düzenleyici.",
              InlineEdit::buildPrompt(instruction, sel, aiLangFor(path)),
          QJsonObject(), [this, path, st, ln, sel](QString reply, QString err) {
              if (!err.isEmpty()) return;
              const QString code = InlineEdit::extractCode(reply);
              if (code.isEmpty()) return;
              ApplyEditDialog d(this);
              d.setTexts(path, sel.left(4000), code.left(8000));
              if (d.exec() == QDialog::Accepted)
                  applyAiEdit(d.newText(), false, st, ln);
          });
}

void MainWindow::fixSelectionAi() {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    QTextCursor c = e->textCursor();
    QString sel = c.selectedText().replace(QChar(0x2029), "\n");
    int st = c.selectionStart(), ln = c.selectionEnd() - st;
    if (sel.trimmed().isEmpty()) {
        c.select(QTextCursor::LineUnderCursor);
        sel = c.selectedText();
        st = c.selectionStart();
        ln = sel.size();
        e->setTextCursor(c);
    }
    const QString path = e->filePath();
    m_status->setText("AI seçimi düzeltiyor...");
    askAiTask(int(AiTask::Explain), "Kısa kod düzenleyici.",
          InlineEdit::buildPrompt("Hataları düzelt, okunabilirliği artır, davranışı koru.",
                                  sel, aiLangFor(path)),
          QJsonObject(), [this, path, st, ln, sel](QString reply, QString err) {
              if (!err.isEmpty()) return;
              const QString code = InlineEdit::extractCode(reply);
              if (code.isEmpty() || code == sel.trimmed()) {
                  m_status->setText("AI değişiklik önermedi.");
                  return;
              }
              ApplyEditDialog d(this);
              d.setTexts(path, sel.left(4000), code.left(8000));
              if (d.exec() == QDialog::Accepted)
                  applyAiEdit(d.newText(), false, st, ln);
          });
}

void MainWindow::documentFunction() {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    const QString text = e->toPlainText();
    const int line = e->textCursor().blockNumber();
    const int start = DocGen::findFuncStart(text, line);
    if (start < 0) {
        m_status->setText("İmleç üstünde fonksiyon bulunamadı.");
        return;
    }
    const QStringList lines = text.split('\n');
    QStringList slice;
    for (int i = start; i < qMin(start + 12, lines.size()); ++i) slice << lines[i];
    const QString path = e->filePath();
    askAiTask(int(AiTask::Doc), "Belge yorum yazarı.",
              DocGen::buildPrompt(slice.join('\n'), aiLangFor(path)),
          QJsonObject(), [this, path, start](QString reply, QString err) {
              if (!err.isEmpty()) return;
              const QString doc = InlineEdit::extractCode(reply);
              if (doc.isEmpty()) return;
              for (QTabWidget* t : {m_tabs, m_tabs2})
                  for (int i = 0; i < t->count(); ++i)
                      if (auto* ce = qobject_cast<CodeEditor*>(t->widget(i)))
                          if (ce->filePath() == path) {
                              QTextBlock b = ce->document()->findBlockByNumber(start);
                              QTextCursor cc(b);
                              // Girintiyi koru
                              QString indent;
                              for (QChar ch : b.text()) {
                                  if (ch == ' ' || ch == '\t') indent += ch;
                                  else break;
                              }
                              QStringList dl = doc.split('\n');
                              for (QString& dl1 : dl) dl1.prepend(indent);
                              cc.insertText(dl.join('\n') + "\n" + indent);
                              pushDocToLsp(ce);
                              ce->flashLine(start);
                              toast(1, "Belge yorumu eklendi.");
                              return;
                          }
          });
}

void MainWindow::genTestForCurrent() {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    const QString path = e->filePath();
    const QString target = TestGen::targetFor(path);
    if (QFile::exists(target)) {
        auto r = QMessageBox::question(this, "Test Üret",
                                       QFileInfo(target).fileName() + " var. Üzerine yazılsın mı?");
        if (r != QMessageBox::Yes) return;
    }
    AppSettings s = SettingsManager::instance().load();
    if (ContextBudget::exceeds(e->toPlainText(), ContextBudget::maxFor(s.contextWindow) / 2))
        toast(2, ContextBudget::warnText(e->toPlainText(),
                                         ContextBudget::maxFor(s.contextWindow) / 2));
    const QString code = ContextBudget::trim(e->toPlainText(),
                                            ContextBudget::maxFor(s.contextWindow) / 2);
    askAiTask(int(AiTask::Test), "Test yazarı.",
          TestGen::buildPrompt(code, aiLangFor(path), QFileInfo(target).fileName()),
          QJsonObject(), [this, target](QString reply, QString err) {
              if (!err.isEmpty()) return;
              const QString code = InlineEdit::extractCode(reply);
              if (code.isEmpty()) return;
              QDir().mkpath(QFileInfo(target).absolutePath());
              QFile f(target);
              if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                  toast(3, "Test dosyası yazılamadı.");
                  return;
              }
              f.write(code.toUtf8());
              f.close();
              openFileAt(target, 1);
              toast(1, "Test üretildi: " + QFileInfo(target).fileName());
          });
}

void MainWindow::commitMessageAi() {
    QProcess p(this);
    p.setWorkingDirectory(m_root);
    p.start("git", {"diff", "--cached", "--no-color"});
    if (!p.waitForFinished(8000)) return;
    const QString diff = QString::fromUtf8(p.readAllStandardOutput());
    if (diff.trimmed().isEmpty()) {
        m_status->setText("Staged değişiklik yok (önce stage'leyin).");
        toast(2, "Staged değişiklik yok.");
        return;
    }
    askAiTask(int(AiTask::Commit), "Commit mesaj yazarı.", CommitMsg::buildPrompt(diff),
              QJsonObject(),
          [this](QString reply, QString err) {
              if (!err.isEmpty()) return;
              const QString first = reply.trimmed().split('\n').first().trimmed();
              if (!CommitMsg::isValid(first)) {
                  m_status->setText("AI conventional üretmedi: " + first.left(100));
                  toast(2, "Conventional format değil — panelde düzeltin.");
                  showSidePanel(2);
                  m_git->setCommitMessage(first.left(100));
                  return;
              }
              showSidePanel(2);
              m_git->setCommitMessage(first);
              toast(1, "Commit mesajı hazır: " + first);
          });
}

void MainWindow::explainSymbol() {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty()) return;
    QTextCursor c = e->textCursor();
    QTextCursor w = c;
    w.select(QTextCursor::WordUnderCursor);
    const QString sym = w.selectedText().left(60);
    if (sym.isEmpty()) {
        m_status->setText("İmleçte sembol yok.");
        return;
    }
    QTextBlock b = c.block();
    QStringList ctx;
    for (int i = qMax(0, b.blockNumber() - 10); i <= b.blockNumber() + 10; ++i) {
        QTextBlock bb = e->document()->findBlockByNumber(i);
        if (bb.isValid()) ctx << bb.text();
    }
    const QRect r = e->cursorRect(c);
    const QString title = QString("%1 — %2").arg(QFileInfo(e->filePath()).fileName(), sym);
    askAiTask(int(AiTask::Explain), "Kod açıklayıcı.",
          QString("Şu %1 sembolünü 3-4 cümleyle Türkçe açıkla (bağlamdaki rolünü vurgula). "
                  "Kod bloğu kullanma:\nSembol: %2\nBağlam:\n%3")
              .arg(sym, sym, ctx.join('\n').left(3000)),
          QJsonObject(), [this, e, r, title](QString reply, QString err) {
              if (!err.isEmpty() || !e) return;
              const ThemeTokens tk = ThemeManager::instance().tokens();
              HoverCard::Colors hc;
              hc.bg = tk.surface.name();
              hc.border = tk.border.name();
              hc.title = tk.accent.name();
              hc.text = tk.text.name();
              hc.code = tk.synString.name();
              hc.codeBg = tk.bg.name();
              hc.dim = tk.textDim.name();
              QToolTip::showText(e->mapToGlobal(r.bottomRight()),
                                 HoverCard::render(title, reply.left(1200), hc), e);
          });
}

void MainWindow::promptLibrary() {
    auto* d = new PromptLibraryDialog(this);
    d->setAttribute(Qt::WA_DeleteOnClose);
    connect(d, &PromptLibraryDialog::runRequested, this,
            [this, d](const QString& name, const QString& prompt, const QString& taskHint) {
                d->close();
                showSidePanel(3);
                // Stage 25: {{selection}} {{file}} {{date}} genişletme
                QString sel, fp;
                if (auto* e = currentEditor()) {
                    fp = e->filePath();
                    sel = e->textCursor().selectedText().left(4000).replace(QChar(0x2029), "\n");
                }
                // Seçim/dosya bağlamını AiPanel kendisi ekler (buildContext)
                m_ai->send(PromptVars::expand(prompt, sel, fp) + "\n[" + name + "]");
                m_status->setText("İstem çalıştı: " + name);
            });
    d->show();
}

void MainWindow::applyLastAi() {
    m_ai->applyLastCodeBlock();
}

void MainWindow::pullModel() {
    AppSettings s = SettingsManager::instance().load();
    bool ok = false;
    const QString model = QInputDialog::getText(this, "Ollama Modeli İndir", "Model:",
                                                QLineEdit::Normal, s.ollamaModel, &ok);
    if (!ok || model.trimmed().isEmpty()) return;
    // Stage 25: indirme kuyruğu — biri inerken sonrakiler sıraya girer
    if (m_pullActive) {
        if (!m_pullQueue.contains(model.trimmed())) m_pullQueue << model.trimmed();
        toast(0, QString("Kuyruğa eklendi (%1. sıra)").arg(m_pullQueue.size()));
        return;
    }
    m_pullActive = true;
    startPull(model.trimmed());
}

void MainWindow::startPull(const QString& model) {
    AppSettings s = SettingsManager::instance().load();
    auto* client = new OllamaClient(this);
    client->setHost(s.ollamaHost);
    m_status->setText("İndiriliyor: " + model.trimmed());
    connect(client, &OllamaClient::pullProgress, this,
            [this](const QString& m, qint64 done, qint64 total, const QString& st) {
                if (total > 0)
                    m_status->setText(QString("%1 %2: %%%3").arg(m, st).arg(done * 100 / total));
                else
                    m_status->setText(QString("%1: %2").arg(m, st));
            });
    connect(client, &OllamaClient::pullFinished, this,
            [this, client](const QString& m) {
                client->deleteLater();
                toast(1, "Model indirildi: " + m + " (AI panelinden seçin)");
                m_status->setText("Model hazır: " + m);
                // Stage 25: kuyruktan devam
                if (!m_pullQueue.isEmpty()) startPull(m_pullQueue.takeFirst());
                else m_pullActive = false;
            });
    connect(client, &OllamaClient::error, this, [this, client](const QString& e) {
        client->deleteLater();
        toast(3, "İndirme hatası: " + e.left(150));
        if (!m_pullQueue.isEmpty()) startPull(m_pullQueue.takeFirst());
        else m_pullActive = false;
    });
    client->pull(model.trimmed());
}

// ---------- Stage 16: uzaktan geliştirme ----------

bool MainWindow::remoteConnected() const {
    return m_ssh && m_ssh->isConnected();
}

void MainWindow::updateRemoteStatus() {
    if (remoteConnected())
        m_status->setText("Uzak: " + m_remoteProfile.display()
                          + " @ " + m_remoteProfile.remoteRoot);
}

void MainWindow::remoteConnectDialog() {
    auto* d = new RemoteConnectDialog(this);
    d->setAttribute(Qt::WA_DeleteOnClose);
    connect(d, &RemoteConnectDialog::connectRequested, this, &MainWindow::remoteConnect);
    connect(d, &RemoteConnectDialog::importSshRequested, this,
            &MainWindow::importSshConfig); // Stage 30
    d->show();
}

void MainWindow::remoteConnect(const ConnectionProfile& p) {
    m_remoteProfile = p;
    m_ssh->setProfile(p);
    m_fwd->setProfile(p);
    m_status->setText("Bağlanıyor: " + p.display() + "...");
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool ok = m_ssh->testConnection(10000);
    QApplication::restoreOverrideCursor();
    if (!ok) {
        const QString e = m_ssh->lastError();
        m_status->setText("SSH bağlanamadı.");
        toast(3, "SSH bağlanamadı: " + e.left(150));
        return;
    }
    m_remoteExplorer->setSession(m_ssh, p);
    showSidePanel(5);
    updateRemoteStatus();
    toast(1, "Bağlandı: " + p.display());
}

void MainWindow::remoteDisconnect() {
    if (m_remoteTerm) m_remoteTerm->stop();
    if (m_fwd) m_fwd->stopAll();
    if (m_lspRemote) m_lspRemote->stop();
    if (m_ssh) m_ssh->disconnect();
    if (m_remoteExplorer) m_remoteExplorer->setSession(nullptr, ConnectionProfile());
    m_status->setText("Uzak bağlantı kesildi.");
    toast(1, "Uzak bağlantı kesildi.");
}

void MainWindow::remoteOpenFile(const QString& remotePath) {
    if (!remoteConnected()) {
        toast(2, "Önce uzağa bağlanın (Uzak → Bağlan).");
        remoteConnectDialog();
        return;
    }
    const QString uri = m_remoteProfile.toUri(remotePath);
    // Zaten açıksa odaklan
    for (QTabWidget* t : {m_tabs, m_tabs2})
        for (int i = 0; i < t->count(); ++i)
            if (auto* e = qobject_cast<CodeEditor*>(t->widget(i)))
                if (e->filePath() == uri) {
                    m_activeGroup = (t == m_tabs) ? 0 : 1;
                    t->setCurrentIndex(i);
                    return;
                }
    m_status->setText("Uzak dosya alınıyor: " + remotePath);
    auto r = m_ssh->readFile(remotePath);
    if (r.exit != 0) {
        toast(3, "Uzak dosya okunamadı: " + r.err.left(150));
        return;
    }
    // mtime kaydet (kaydederken çakışma denetimi için)
    QDateTime mt;
    qint64 sz = 0;
    auto st = m_ssh->exec(RemoteFileSystem::statCommand(remotePath));
    if (st.exit == 0 && RemoteFileSystem::parseStat(st.out, mt, sz))
        m_remoteMtimes[remotePath] = mt;
    if (auto* e = openEditorFor(uri)) {
        e->setContent(uri, r.out);
        m_status->setText("Uzak: " + uri);
        // Uzak LSP açıksa belgeyi tanıt
        if (m_lspRemote && m_lspRemote->isReady()) {
            const QString suf = QFileInfo(remotePath).suffix().toLower();
            m_lspRemote->didOpen(remotePath, suf == "py" ? "python" : "cpp", r.out);
        }
    }
}

bool MainWindow::remoteSaveEditor(CodeEditor* e, bool interactive) {
    if (!e || !e->isRemote()) return false;
    if (!remoteConnected()) {
        if (interactive) toast(2, "Bağlantı yok — kaydedilemedi.");
        return false;
    }
    const auto u = RemoteFileSystem::parseUri(e->filePath());
    if (!u.ok) return false;
    // Çakışma denetimi: uzaktaki bizden sonra değiştiyse sor
    QDateTime mt;
    qint64 sz = 0;
    auto st = m_ssh->exec(RemoteFileSystem::statCommand(u.path));
    if (st.exit == 0 && RemoteFileSystem::parseStat(st.out, mt, sz)) {
        const QDateTime opened = m_remoteMtimes.value(u.path);
        if (opened.isValid() && mt > opened.addMSecs(1500)) {
            if (!interactive) {
                m_status->setText("Uzak değişti, atlandı: " + u.path);
                return false;
            }
            auto ans = QMessageBox::question(
                this, "Uzak Çakışma",
                u.path + "\nuzakta daha yeni bir sürüm var. Üzerine yazılsın mı?",
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (ans != QMessageBox::Yes) return false;
        }
    }
    auto w = m_ssh->writeFile(u.path, e->toPlainText());
    if (w.exit != 0) {
        if (interactive) toast(3, "Uzak kaydetme hatası: " + w.err.left(150));
        return false;
    }
    e->document()->setModified(false);
    auto st2 = m_ssh->exec(RemoteFileSystem::statCommand(u.path));
    if (st2.exit == 0 && RemoteFileSystem::parseStat(st2.out, mt, sz))
        m_remoteMtimes[u.path] = mt;
    m_status->setText("Kaydedildi (uzak): " + u.path);
    if (interactive) toast(1, "Kaydedildi: " + QFileInfo(u.path).fileName());
    updateCursorStatus();
    return true;
}

void MainWindow::remoteOpenDialog() {
    if (!remoteConnected()) {
        toast(2, "Önce uzağa bağlanın (Uzak → Bağlan).");
        remoteConnectDialog();
        return;
    }
    bool ok = false;
    const QString path = QInputDialog::getText(
        this, "Uzak Dosya Aç", "Uzak yol:", QLineEdit::Normal,
        m_remoteProfile.remoteRoot + "/", &ok);
    if (!ok || path.trimmed().isEmpty()) return;
    remoteOpenFile(RemoteFileSystem::normalize(path.trimmed()));
}

void MainWindow::remoteTerminalShow() {
    if (!remoteConnected()) {
        toast(2, "Önce uzağa bağlanın (Uzak → Bağlan).");
        remoteConnectDialog();
        return;
    }
    m_termDock->show();
    m_bottomTabs->setCurrentWidget(m_remoteTerm);
    if (!m_remoteTerm->isRunning()) m_remoteTerm->start(m_remoteProfile);
}

void MainWindow::remoteLspStart() {
    if (!remoteConnected()) {
        toast(2, "Önce uzağa bağlanın (Uzak → Bağlan).");
        remoteConnectDialog();
        return;
    }
    bool ok = false;
    const QString cmd = QInputDialog::getText(this, "Uzak LSP", "Uzak LSP komutu:",
                                              QLineEdit::Normal, "clangd --offset-encoding=utf-8",
                                              &ok);
    if (!ok || cmd.trimmed().isEmpty()) return;
    const QStringList parts = cmd.simplified().split(' ');
    LspTransport t = LspTransport::remote(m_remoteProfile, parts.first(), parts.mid(1));
    QString prog;
    QStringList args;
    LspTransport::toProcess(t, prog, args);
    m_status->setText("Uzak LSP başlatılıyor...");
    if (!m_lspRemote->start(prog, args, m_remoteProfile.remoteRoot)) {
        toast(3, "Uzak LSP başlatılamadı.");
        return;
    }
    // Açık uzak belgeleri tanıt
    for (CodeEditor* e : allEditors())
        if (e->isRemote()) {
            const auto u = RemoteFileSystem::parseUri(e->filePath());
            if (u.ok) {
                const QString suf = QFileInfo(u.path).suffix().toLower();
                m_lspRemote->didOpen(u.path, suf == "py" ? "python" : "cpp",
                                     e->toPlainText());
            }
        }
    toast(1, "Uzak LSP çalışıyor: " + cmd);
    m_status->setText("Uzak LSP hazır.");
}

void MainWindow::remoteBuild() {
    if (!remoteConnected()) {
        toast(2, "Önce uzağa bağlanın (Uzak → Bağlan).");
        remoteConnectDialog();
        return;
    }
    bool ok = false;
    const QString cmd = QInputDialog::getText(this, "Uzakta Derle", "Komut:",
                                              QLineEdit::Normal, "make -j$(nproc)", &ok);
    if (!ok || cmd.trimmed().isEmpty()) return;
    m_termDock->show();
    m_bottomTabs->setCurrentWidget(m_terminal);
    m_status->setText("Uzakta derleniyor: " + cmd);
    auto r = m_ssh->exec(cmd, m_remoteProfile.remoteRoot, 120000);
    const QString out = r.out + r.err;
    m_terminal->appendOut(QString("$ [uzak] %1\n%2").arg(cmd, out.left(20000)));
    // Sorunları Problems paneline akıt (uzak kök → ssh:// eşlemesiyle)
    m_buildDiags.clear();
    const auto issues = RemoteTaskRunner::parseBuildOutput(out, m_remoteProfile.remoteRoot);
    for (const auto& is : issues) {
        LspDiag d;
        d.path = m_remoteProfile.toUri(is.file);
        d.line = qMax(0, is.line1 - 1);
        d.col = qMax(0, is.col1 - 1);
        d.endLine = d.line;
        d.endCol = d.col + 1;
        d.severity = (is.kind == "warning") ? 2 : 1;
        d.message = is.message;
        d.source = "uzak-derleme";
        m_buildDiags[d.path].append(d);
    }
    refreshProblemView();
    m_status->setText(QString("Uzak derleme bitti (%1, %2 sorun).")
                          .arg(RemoteTaskRunner::describeExit(r.exit))
                          .arg(issues.size()));
    if (!issues.isEmpty()) {
        m_bottomTabs->setCurrentWidget(m_problems);
        showSidePanel(4);
    }
}

void MainWindow::remoteGitStatus() {
    if (!remoteConnected()) {
        toast(2, "Önce uzağa bağlanın (Uzak → Bağlan).");
        remoteConnectDialog();
        return;
    }
    RemoteGitRunner git(m_remoteProfile);
    auto r = git.run({"status", "--short", "--branch"});
    m_termDock->show();
    m_bottomTabs->setCurrentWidget(m_terminal);
    m_terminal->appendOut(QString("$ [uzak:%1] git status\n%2%3")
                                    .arg(m_remoteProfile.host, r.out, r.err));
    m_status->setText("Uzak git durumu alındı (" + git.label() + ").");
}

void MainWindow::remoteDebugStart() {
    if (!remoteConnected()) {
        toast(2, "Önce uzağa bağlanın (Uzak → Bağlan).");
        remoteConnectDialog();
        return;
    }
    bool ok = false;
    // "program:port" — gdbserver uzakta bu portta bekleniyor olmalı
    const QString spec = QInputDialog::getText(this, "Uzak Hata Ayıklama",
        "Uzak program ve gdbserver portu (program:port):\n"
        "Örn: /home/ali/proje/app:2345\n"
        "(Uzakta önce çalıştırın: gdbserver :2345 /home/ali/proje/app)",
        QLineEdit::Normal, m_remoteProfile.remoteRoot + "/app:2345", &ok);
    if (!ok || spec.trimmed().isEmpty()) return;
    const int colon = spec.lastIndexOf(':');
    if (colon < 0) {
        toast(2, "Biçim: program:port");
        return;
    }
    const QString program = spec.left(colon).trimmed();
    const int port = spec.mid(colon + 1).trimmed().toInt();
    if (program.isEmpty() || port <= 0) {
        toast(2, "Geçersiz program/port.");
        return;
    }
    // Sembol dosyası: aynı yolu yerelde de dene (yoksa sembolsüz devam)
    QString localSym = program;
    if (!QFile::exists(localSym)) {
        // Uzak kökü yerel aynaya eşle (aynı ada sahip yerel dosya varsa)
        const QString cand = m_root + "/" + RemoteFileSystem::fileName(program);
        if (QFile::exists(cand)) localSym = cand;
        else localSym.clear();
    }
    m_termDock->show();
    m_bottomTabs->setCurrentWidget(m_debug);
    m_status->setText(QString("Uzak hedefe bağlanılıyor: %1:%2...").arg(program).arg(port));
    // gdbserver'a doğrudan değil, SSH tüneli üzerinden yerel porta bağlan
    // (güvenlik: gdbserver protokolü şifresizdir).
    ForwardRule rule;
    rule.localPort = (port % 40000) + 10000; // çakışma olasılığı düşük yerel port
    if (rule.localPort > 60000) rule.localPort = 15999;
    rule.targetHost = "localhost";
    rule.targetPort = port;
    rule.label = "gdb-" + m_remoteProfile.host + ":" + QString::number(port);
    if (!m_fwd->isActive(rule.label) && !m_fwd->start(rule)) {
        toast(3, "SSH tüneli açılamadı — gdbserver'a doğrudan bağlanılamıyor.");
        return;
    }
    debugStartRemote(localSym, "127.0.0.1", rule.localPort);
}

void MainWindow::remoteForwardShow() {
    if (!remoteConnected()) {
        toast(2, "Önce uzağa bağlanın (Uzak → Bağlan).");
        remoteConnectDialog();
        return;
    }
    m_termDock->show();
    m_bottomTabs->setCurrentWidget(m_fwdPanel);
}

void MainWindow::debugStartRemote(const QString& localSymbols, const QString& host, int port) {
    AppSettings s = SettingsManager::instance().load();
    m_termDock->setVisible(true);
    m_bottomTabs->setCurrentWidget(m_debug);
    m_debug->clearOutput();
    m_debug->setRunning(true, false);
    m_status->setText(QString("Uzak hedef: %1:%2").arg(host).arg(port));
    if (!m_gdb->isRunning() && !m_gdb->start(s.gdbPath)) {
        m_debug->setRunning(false, false);
        return;
    }
    m_gdbBpNums.clear();
    m_gdb->launchRemote(localSymbols, host, port, [this](bool ok) {
        if (!ok) {
            m_debug->setRunning(false, false);
            return;
        }
        syncBreakpointsToGdb();
        m_status->setText("Uzak program bağlandı (gdbserver).");
    });
}

// ---------- Stage 17: editör deneyimi ----------

void MainWindow::snapshotEditor(CodeEditor* e) {
    if (!e || e->isRemote() || e->filePath().isEmpty() || e->previewMode()) return;
    if (!m_localHist) {
        const QString dir = m_backups ? m_backups->dir()
            : QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        m_localHist = new LocalHistory(dir);
    }
    AppSettings s = SettingsManager::instance().load();
    m_localHist->snapshot(e->filePath(), e->toPlainText(), s.historyKeep);
}

void MainWindow::snippetPalette(bool manage) {
    auto* e = currentEditor();
    const QString suffix = e ? QFileInfo(e->filePath()).suffix() : QString();
    auto* d = new SnippetDialog(suffix, this);
    d->setAttribute(Qt::WA_DeleteOnClose);
    connect(d, &SnippetDialog::insertRequested, this, &MainWindow::insertSnippetDef);
    if (manage) d->show();
    else d->show();
    Q_UNUSED(manage);
}

void MainWindow::insertSnippetDef(const SnippetDef& s) {
    auto* e = currentEditor();
    if (!e) return;
    // Önek kelime insertSnippet içinde temizlenir
    e->insertSnippet(s.body);
    m_status->setText("Snippet: " + s.prefix);
}

void MainWindow::outlineShow() {
    showSidePanel(6);
    refreshOutline();
}

void MainWindow::refreshOutline() {
    auto* e = currentEditor();
    if (!e || !m_outline) return;
    if (e->filePath().isEmpty()) { m_outline->clear(); return; }
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c || e->isRemote()) {
        // Yedek: regex taraması
        m_outline->setFallbackText(e->toPlainText(),
                                   QFileInfo(e->filePath()).suffix());
        return;
    }
    const QString path = e->filePath();
    withLspReady(c, [this, c, path, e]() {
        if (!e || e->filePath() != path) return;
        c->requestDocSymbols(path, [this, e, path](QJsonObject res) {
            if (!e || e->filePath() != path || !m_outline) return;
            QList<SymbolNode> roots = SymbolTree::parseDocument(res);
            if (roots.isEmpty())
                m_outline->setFallbackText(e->toPlainText(),
                                           QFileInfo(path).suffix());
            else
                m_outline->setLspSymbols(roots);
        });
    });
    // Anında yedek göster (LSP gecikirse boş kalmasın)
    m_outline->setFallbackText(e->toPlainText(), QFileInfo(e->filePath()).suffix());
}

void MainWindow::timelineShow() {
    auto* e = currentEditor();
    m_termDock->show();
    m_bottomTabs->setCurrentWidget(m_timeline);
    if (e && m_timeline) m_timeline->setFile(e->filePath(), e->toPlainText());
}

void MainWindow::timelineRestorePrev() {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty() || e->isRemote() || !m_localHist) {
        m_status->setText("Geri dönülecek sürüm yok.");
        return;
    }
    const auto snaps = m_localHist->list(e->filePath());
    if (snaps.isEmpty()) {
        m_status->setText("Anlık görüntü yok.");
        return;
    }
    auto r = QMessageBox::question(this, "Önceki Sürüme Dön",
        QString("Son kayıtlı sürüme dönülsün mü?\n(%1)\nMevcut içerik önce anlık görüntülenir.")
            .arg(snaps.first().when.isValid()
                     ? snaps.first().when.toString("dd.MM HH:mm:ss")
                     : snaps.first().id));
    if (r != QMessageBox::Yes) return;
    m_localHist->snapshot(e->filePath(), e->toPlainText()); // günü kurtar
    const QString content = m_localHist->read(snaps.first());
    const int pos = e->textCursor().position();
    e->setPlainText(content);
    QTextCursor c(e->document());
    c.setPosition(qMin(pos, e->document()->characterCount() - 1));
    e->setTextCursor(c);
    e->document()->setModified(true);
    pushDocToLsp(e);
    toast(1, "Önceki sürüme dönüldü.");
}

void MainWindow::pasteAs() {
    auto* e = currentEditor();
    if (!e) return;
    QMenu m(this);
    for (PasteTransform::Kind k : PasteTransform::all())
        m.addAction(PasteTransform::title(k), [this, e, k]() {
            if (auto* cb = QApplication::clipboard()) {
                const QString t = PasteTransform::apply(k, cb->text());
                e->insertTextAtCursors(t);
            }
        });
    m.exec(QCursor::pos());
}

void MainWindow::wordComplete() {
    auto* e = currentEditor();
    if (!e) return;
    const QString prefix = e->completionPrefix();
    if (prefix.size() < 2) {
        m_status->setText("Tamamlanacak önek yok (en az 2 harf).");
        return;
    }
    QList<QPair<QString, int>> docs;
    const int curLine = e->textCursor().blockNumber();
    for (CodeEditor* o : allEditors())
        docs << qMakePair(o->toPlainText(), (o == e) ? curLine : 1000000);
    const auto cands = WordComplete::suggest(prefix, docs,
                                             QFileInfo(e->filePath()).suffix());
    if (cands.isEmpty()) {
        m_status->setText("Kelime önerisi yok.");
        return;
    }
    QList<CompletionItem> items;
    for (const WordCand& c : cands)
        items << CompletionItem{c.word, QString("x%1").arg(c.freq), c.word, 1, c.word,
                                c.word};
    QRect r = e->cursorRect(e->textCursor());
    m_completion->showItems(items, e->mapToGlobal(r.bottomRight()));
}

void MainWindow::foldingRefreshLsp() {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty() || e->isRemote()) {
        m_status->setText("LSP katlaması için yerel dosya gerekir.");
        return;
    }
    QString lang;
    LspClient* c = lspClientFor(QFileInfo(e->filePath()).suffix(), lang);
    if (!c) return;
    const QString path = e->filePath();
    const int lines = e->blockCount();
    withLspReady(c, [this, c, path, e, lines]() {
        if (!e || e->filePath() != path) return;
        c->request("textDocument/foldingRange",
                   QJsonObject{{"textDocument",
                                QJsonObject{{"uri", LspClient::pathToUri(path)}}}},
                   [this, e, path, lines](QJsonObject res) {
                       if (!e || e->filePath() != path) return;
                       QList<QPair<int, int>> ranges;
                       for (const FoldRangeLsp& r :
                            FoldingRanges::parseResult(res, lines))
                           ranges << qMakePair(r.startLine, r.endLine);
                       e->applyServerFolds(ranges);
                       m_status->setText(
                           QString("%1 katlama aralığı (LSP+girinti).").arg(ranges.size()));
                   });
    });
}

void MainWindow::searchExcludeFocus() {
    showSidePanel(1);
    m_search->focusExclude();
}

void MainWindow::hunkMenuAtCursor() {
    auto* e = currentEditor();
    if (!e || e->filePath().isEmpty() || e->isRemote()) return;
    const int line1 = e->textCursor().blockNumber() + 1;
    const auto marks = e->gitMarks();
    if (!marks.contains(line1)) {
        m_status->setText("İmleç satırında değişiklik yok.");
        return;
    }
    QFileInfo fi(e->filePath());
    QProcess p(this);
    p.setWorkingDirectory(fi.absolutePath());
    p.start("git", {"diff", "--no-color", "-U3", "--", fi.fileName()});
    if (!p.waitForFinished(8000) || p.exitCode() != 0) return;
    const QString diff = QString::fromUtf8(p.readAllStandardOutput());
    const auto files = DiffEngine::parseUnified(diff);
    if (files.isEmpty()) return;
    const FileDiff& fd = files.first();
    int pick = -1;
    for (int i = 0; i < fd.hunks.size(); ++i) {
        const DiffHunk& h = fd.hunks[i];
        if (line1 >= h.newStart && line1 < h.newStart + qMax(1, h.newCount)) {
            pick = i;
            break;
        }
    }
    if (pick < 0) {
        m_status->setText("Hunk bulunamadı.");
        return;
    }
    QMenu m(this);
    m.addAction(QString("Farkı göster (%1 satır)").arg(fd.hunks[pick].lines.size()),
                [this, fi, diff]() {
                    DiffDialog d(this);
                    d.setRepoDiff(fi.absolutePath(), diff, false);
                    d.exec();
                    if (auto* ce = currentEditor()) refreshGitMarks(ce);
                });
    m.addAction("Hunk'u geri al", [this, e, fi, fd, pick]() {
        snapshotEditor(e);
        QProcess gp(this);
        gp.setWorkingDirectory(fi.absolutePath());
        gp.start("git", {"apply", "-R", "--unidiff-zero", "-"});
        gp.write(DiffEngine::buildPatch(fd, {pick}).toUtf8());
        gp.closeWriteChannel();
        gp.waitForFinished(8000);
        if (gp.exitCode() != 0) {
            toast(3, "Geri alma başarısız: "
                         + QString::fromUtf8(gp.readAllStandardError()).left(120));
            return;
        }
        e->reloadFromDisk();
        refreshGitMarks(e);
        toast(1, "Hunk geri alındı.");
    });
    m.addAction("Hunk'u stage'le", [this, e, fi, fd, pick]() {
        QProcess gp(this);
        gp.setWorkingDirectory(fi.absolutePath());
        gp.start("git", {"apply", "--cached", "--unidiff-zero", "-"});
        gp.write(DiffEngine::buildPatch(fd, {pick}).toUtf8());
        gp.closeWriteChannel();
        gp.waitForFinished(8000);
        if (gp.exitCode() != 0) {
            toast(3, "Stage başarısız: "
                         + QString::fromUtf8(gp.readAllStandardError()).left(120));
            return;
        }
        refreshGitMarks(e);
        toast(1, "Hunk stage'lendi.");
    });
    m.exec(QCursor::pos());
}

// Stage 46: bekleyen yer tutucuyu gerçek düzenleyiciyle değiştir
void MainWindow::materializePending(QWidget* w) {
    if (!w) return;
    const QString path = w->property("pendingPath").toString();
    if (path.isEmpty()) return;
    QTabWidget* owner = nullptr;
    int idx = -1;
    for (QTabWidget* t : {m_tabs, m_tabs2}) {
        const int i = t->indexOf(w);
        if (i >= 0) {
            owner = t;
            idx = i;
            break;
        }
    }
    if (!owner) return;
    auto* e = new CodeEditor(this);
    applyEditorSettingsTo(e);
    if (!e->loadFile(path)) {
        delete e;
        owner->setTabText(idx, QFileInfo(path).fileName() + " (açılamadı)");
        w->setProperty("pendingPath", QString());
        return;
    }
    if (m_pendingCursors.contains(path)) {
        e->setCursorOffset(m_pendingCursors.take(path));
    }
    if (m_pendingFolds.contains(path)) {
        e->setFoldedLines(m_pendingFolds.take(path));
    }
    // Stage 32: büyük dosya kipi
    AppSettings cfg = SettingsManager::instance().load();
    const int mode = PerfTools::largeFileMode(QFileInfo(path).size(), cfg.largeFileMb, 8);
    if (mode == 2 && !e->isReadOnly()) {
        e->setReadOnly(true);
        toast(2, "Büyük dosya salt-okunur açıldı (8 MB+)");
    }
    // Stage 46: removeTab komşuyu otomatik seçip currentChanged ateşler;
    // araya giren gerçekleme zincirini engelle (tek sekme ilkesi).
    const bool blocked = owner->blockSignals(true);
    owner->removeTab(idx);
    owner->blockSignals(blocked);
    w->deleteLater();
    const int group = (owner == m_tabs) ? 0 : 1;
    addEditorTab(e, QFileInfo(path).fileName(), path, group);
    owner->setCurrentWidget(e);
    setupLspFor(path, e->toPlainText());
}

bool MainWindow::isPendingTab(QWidget* w) const {
    return w && !w->property("pendingPath").toString().isEmpty();
}

int MainWindow::pendingTabCount() const {
    int n = 0;
    for (QTabWidget* t : {m_tabs, m_tabs2})
        for (int i = 0; i < t->count(); ++i)
            if (!t->widget(i)->property("pendingPath").toString().isEmpty()) ++n;
    return n;
}

int MainWindow::editorTabCount() const {
    int n = 0;
    for (QTabWidget* t : {m_tabs, m_tabs2})
        for (int i = 0; i < t->count(); ++i)
            if (qobject_cast<CodeEditor*>(t->widget(i))) ++n;
    return n;
}

// --- Stage 49: birlikte çalışma (asgari bağlantı) ---
CollabSession* MainWindow::collab() {
    if (!m_collab) {
        m_collab = new CollabSession(this);
        connect(m_collab, &CollabSession::peerJoined, this, [this](const QString& u) {
            toast(0, QString("%1 katıldı").arg(u));
            m_status->setText(QString("Birlikte: %1").arg(collabPeers().join(", ")));
        });
        connect(m_collab, &CollabSession::peerLeft, this, [this](const QString& u) {
            toast(0, QString("%1 ayrıldı").arg(u));
            renderPeerCursors();
        });
        connect(m_collab, &CollabSession::cursorsChanged, this,
                &MainWindow::renderPeerCursors);
        connect(m_collab, &CollabSession::textMerged, this,
                &MainWindow::applyCollabText);
        connect(m_collab, &CollabSession::sessionError, this, [this](const QString& m) {
            toast(2, m);
        });
        connect(m_collab, &CollabSession::activeChanged, this, [this](bool on) {
            if (!on) renderPeerCursors();
        });
    }
    return m_collab;
}

QStringList MainWindow::collabPeers() const {
    if (!m_collab || !m_collab->isActive()) return {};
    return m_collab->peers();
}

void MainWindow::collabHost() {
    CollabSession* c = collab();
    if (c->isActive()) {
        toast(1, "Zaten bir oturumdasın (önce ayrıl).");
        return;
    }
    const QString user = QInputDialog::getText(this, "Oturum Barındır", "Adın:");
    if (user.trimmed().isEmpty()) return;
    const int port = c->host(user.trimmed());
    if (port <= 0) {
        toast(3, "Oturum açılamadı.");
        return;
    }
    CodeEditor* e = currentEditor();
    if (e) c->setBaseText(e->toPlainText());
    const QString url = QString("ws://<bu-bilgisayar>:%1").arg(port);
    QApplication::clipboard()->setText(url);
    toast(0, QString("Oturum açık (port %1) — adres panoda, davetlilerle paylaş.").arg(port));
    m_status->setText(QString("Birlikte (evsahibi, port %1)").arg(port));
}

void MainWindow::collabJoin() {
    CollabSession* c = collab();
    if (c->isActive()) {
        toast(1, "Zaten bir oturumdasın (önce ayrıl).");
        return;
    }
    const QString url =
        QInputDialog::getText(this, "Oturuma Katıl", "Adres (ws://bilgisayar:port):");
    if (url.trimmed().isEmpty()) return;
    const QString user = QInputDialog::getText(this, "Oturuma Katıl", "Adın:");
    if (user.trimmed().isEmpty()) return;
    if (!c->join(url.trimmed(), user.trimmed()))
        toast(3, "Katılınamadı.");
    else
        m_status->setText("Birlikte: bağlanıyor...");
}

void MainWindow::collabLeave() {
    if (!m_collab || !m_collab->isActive()) return;
    m_collab->leave();
    renderPeerCursors();
    m_status->setText("Birlikte: kapalı");
    toast(1, "Oturumdan ayrıldın.");
}

void MainWindow::renderPeerCursors() {
    CodeEditor* e = currentEditor();
    if (!e) return;
    QList<QTextEdit::ExtraSelection> sel = e->extraSelections();
    // Eski eş imleçlerini temizle (kullanıcı seçimleri korunur)
    sel.erase(std::remove_if(sel.begin(), sel.end(),
                             [](const QTextEdit::ExtraSelection& s) {
                                 return s.format.property(QTextFormat::UserProperty + 41).isValid();
                             }),
              sel.end());
    if (m_collab && m_collab->isActive()) {
        static const QStringList colors = {"#ff6b6b", "#4ec9b0", "#cca700",
                                           "#9d7bff", "#4aa8ff"};
        int i = 0;
        const auto curs = m_collab->cursors();
        for (auto it = curs.constBegin(); it != curs.constEnd(); ++it, ++i) {
            QTextCursor c(e->document());
            const int block = qBound(0, it.value().first - 1, e->blockCount() - 1);
            c.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, block);
            c.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor,
                           qMax(0, it.value().second - 1));
            QTextEdit::ExtraSelection xs;
            xs.cursor = c;
            xs.format.setBackground(QColor(colors.at(i % colors.size())));
            xs.format.setProperty(QTextFormat::UserProperty + 41, it.key());
            xs.format.setToolTip(it.key());
            sel << xs;
        }
    }
    e->setExtraSelections(sel);
}

void MainWindow::applyCollabText(const QString& text, const QString& from) {
    Q_UNUSED(from);
    CodeEditor* e = currentEditor();
    if (!e || m_collabApplying) return;
    // İmleci korumaya çalışarak uygula
    QTextCursor c = e->textCursor();
    const int pos = c.position();
    m_collabApplying = true;
    e->setPlainText(text);
    c = e->textCursor();
    c.setPosition(qBound(0, pos, e->document()->characterCount() - 1));
    e->setTextCursor(c);
    m_collabApplying = false;
    if (m_collab) m_collab->setBaseText(text);
}

void MainWindow::publishCollabEdit() {
    if (!m_collab || !m_collab->isActive() || m_collabApplying) return;
    if (CodeEditor* e = currentEditor()) m_collab->publishText(e->toPlainText());
}

void MainWindow::publishCollabCursor() {
    if (!m_collab || !m_collab->isActive()) return;
    if (CodeEditor* e = currentEditor()) {
        const QTextCursor c = e->textCursor();
        m_collab->publishCursor(c.blockNumber() + 1, c.columnNumber() + 1);
    }
}
