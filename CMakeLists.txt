cmake_minimum_required(VERSION 3.16)
project(verso-coder VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Concurrent Svg Qml WebSockets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(HUNSPELL REQUIRED hunspell)
pkg_check_modules(LIBSSH2 libssh2)
if(LIBSSH2_FOUND)
    add_compile_definitions(VERSO_HAVE_LIBSSH2)
endif()

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/core/SettingsManager.cpp
    src/core/ThemeManager.cpp
    src/core/LanguageManager.cpp
    src/core/OllamaClient.cpp
    src/core/RagIndexer.cpp
    src/core/LspClient.cpp
    src/core/EditorConfig.cpp
    src/core/SpellChecker.cpp
    src/core/Spelling.cpp
    src/core/GitIgnore.cpp
    src/core/TrashManager.cpp
    src/core/ProjectSessions.cpp
    src/core/FileIcons.cpp
    src/core/AgentTools.cpp
    src/core/PathGuard.cpp
    src/core/VersoPaths.cpp
    src/core/UpdateChecker.cpp
    src/core/PluginEngine.cpp
    src/core/LeakWatch.cpp
    src/core/RagProgress.cpp
    src/core/SetupAdvisor.cpp
    src/core/CommandAudit.cpp
    src/core/AgentLoop.cpp
    src/core/PatchQueue.cpp
    src/core/ContextResolver.cpp
    src/core/ChatStore.cpp
    src/core/TokenStats.cpp
    src/core/ModelCapabilities.cpp
    src/core/ImageUtil.cpp
    src/core/Embedding.cpp
    src/core/EmbeddingClient.cpp
    src/core/HybridRanker.cpp
    src/core/ResponseScorer.cpp
    src/core/ConversationSummarizer.cpp
    src/core/AgentPolicy.cpp
    src/core/AgentBudget.cpp
    src/core/AgentMemory.cpp
    src/core/SkillRegistry.cpp
    src/core/SkillChain.cpp
    src/core/AgentReflection.cpp
    src/core/ProjectHealth.cpp
    src/core/AgentRunStore.cpp
    src/core/ai/LlmProvider.cpp
    src/core/ai/SecretStore.cpp
    src/core/ai/AiMessage.cpp
    src/core/ai/LlmClient.cpp
    src/core/ai/providers/ProviderCodec.cpp
    src/core/ai/providers/SseParser.cpp
    src/core/ai/AiToolBridge.cpp
    src/core/ai/ProviderPrefs.cpp
    src/core/ai/ProviderPricing.cpp
    src/core/ai/UsageLedger.cpp
    src/core/ai/ProviderHealth.cpp
    src/core/ai/TaskRouter.cpp
    src/core/ai/EmbedCache.cpp
    src/core/ai/EmbedBridge.cpp
    src/core/ai/AgentLlmAdapter.cpp
    src/core/ai/ProviderBench.cpp
    src/core/ai/AiRunner.cpp
    src/core/ai/ModelPool.cpp
    src/core/StartupArgs.cpp
    src/core/ToolchainProbe.cpp
    src/core/TaskRunner.cpp
    src/core/PerfMonitor.cpp
    src/core/KeymapPresets.cpp
    src/core/BackupManager.cpp
    src/core/WorkspaceConfig.cpp
    src/core/SettingsIO.cpp
    src/core/AboutInfo.cpp
    src/core/ThemeTokens.cpp
    src/core/AccentColor.cpp
    src/core/UiMetrics.cpp
    src/core/Typography.cpp
    src/core/QssBuilder.cpp
    src/core/ThemeStore.cpp
    src/core/IconTheme.cpp
    src/core/Animator.cpp
    src/core/ToastManager.cpp
    src/core/LayoutPresets.cpp
    src/core/UiProfile.cpp
    src/core/A11yCheck.cpp
    src/core/GridCheck.cpp
    src/core/LspServers.cpp
    src/core/MergeParse.cpp
    src/core/SshConfig.cpp
    src/core/Stability.cpp
    src/core/BackupManager.cpp
    src/core/CrashHandler.cpp
    src/core/PerfTools.cpp
    src/core/WorkspaceSymbols.cpp
    src/core/CodeMetrics.cpp
    src/core/DuplicateFinder.cpp
    src/core/GitBlame.cpp
    src/core/PromptVars.cpp
    src/core/MacroRecorder.cpp
    src/core/ExternalTools.cpp
    src/core/TaskChain.cpp
    src/core/PluginEngine.cpp
    src/core/CollabMerge.cpp
    src/core/CollabSession.cpp
    src/core/GitVersionManager.cpp
    src/core/CacheCleaner.cpp
    src/core/ThreadMonitor.cpp
    src/core/LanguageSupport.cpp
    src/core/WordDiff.cpp
    src/core/BracketDepth.cpp
    src/core/ScopeChain.cpp
    src/core/SearchMarks.cpp
    src/core/DiffGutter.cpp
    src/core/HoverCard.cpp
    src/core/SelectionGrow.cpp
    src/core/Density.cpp
    src/core/ColorBlind.cpp
    src/core/AutoTheme.cpp
    src/core/ProfileStore.cpp
    src/core/ThemeValidator.cpp
    src/core/CompletionList.cpp
    src/core/SymbolTree.cpp
    src/core/LocationSet.cpp
    src/core/TextEdits.cpp
    src/core/CodeActionList.cpp
    src/core/SignatureHelp.cpp
    src/core/InlayHintList.cpp
    src/core/SemanticTokens.cpp
    src/core/CallHierarchyTree.cpp
    src/core/MiParser.cpp
    src/core/BreakpointStore.cpp
    src/core/TestParser.cpp
    src/core/TestDiscovery.cpp
    src/core/GcovParser.cpp
    src/core/ProblemMatcher.cpp
    src/core/LaunchConfig.cpp
    src/core/GdbDriver.cpp
    src/core/ConnectionProfile.cpp
    src/core/SshSession.cpp
    src/core/RemoteFileSystem.cpp
    src/core/RemoteTaskRunner.cpp
    src/core/LspTransport.cpp
    src/core/GitRunner.cpp
    src/core/PortForwarder.cpp
    src/core/SnippetEngine.cpp
    src/core/SnippetManager.cpp
    src/core/FoldingRanges.cpp
    src/core/BlockSelect.cpp
    src/core/AutoPairs.cpp
    src/core/PasteTransform.cpp
    src/core/OutlineFallback.cpp
    src/core/LocalHistory.cpp
    src/core/ReplaceEngine.cpp
    src/core/WordComplete.cpp
    src/core/GhostCompletion.cpp
    src/core/InlineEdit.cpp
    src/core/DocGen.cpp
    src/core/TestGen.cpp
    src/core/CommitMsg.cpp
    src/core/PromptLibrary.cpp
    src/core/ContextBudget.cpp
    src/widgets/CodeEditor.cpp
    src/widgets/ExplorerPanel.cpp
    src/widgets/SearchPanel.cpp
    src/widgets/BreadcrumbBar.cpp
    src/widgets/Minimap.cpp
    src/widgets/GitPanel.cpp
    src/widgets/AiPanel.cpp
    src/widgets/SettingsDialog.cpp
    src/widgets/QuickOpenDialog.cpp
    src/widgets/ApplyEditDialog.cpp
    src/widgets/ProblemsPanel.cpp
    src/widgets/CommandPalette.cpp
    src/widgets/TerminalPanel.cpp
    src/widgets/DiffDialog.cpp
    src/widgets/GitTools.cpp
    src/widgets/TodoPanel.cpp
    src/widgets/CompareDialog.cpp
    src/widgets/PatchReviewDialog.cpp
    src/widgets/DiagnosticsDialog.cpp
    src/widgets/TaskPanel.cpp
    src/widgets/ThemeGalleryDialog.cpp
    src/widgets/WelcomeView.cpp
    src/widgets/EmptyState.cpp
    src/widgets/EditorFindBar.cpp
    src/widgets/ThemeEditorDialog.cpp
    src/widgets/TitleBar.cpp
    src/widgets/CompletionPopup.cpp
    src/widgets/SymbolPickerDialog.cpp
    src/widgets/ReferencesDialog.cpp
    src/widgets/RenamePreviewDialog.cpp
    src/widgets/CallHierarchyDialog.cpp
    src/widgets/DebugPanel.cpp
    src/widgets/TestExplorer.cpp
    src/widgets/RemoteConnectDialog.cpp
    src/widgets/RemoteExplorer.cpp
    src/widgets/RemoteTerminal.cpp
    src/widgets/PortForwardPanel.cpp
    src/widgets/OutlinePanel.cpp
    src/widgets/TimelinePanel.cpp
    src/widgets/SnippetDialog.cpp
    src/widgets/NewFileDialog.cpp
    src/widgets/FirstRunDialog.cpp
    src/widgets/ShortcutDialog.cpp
    src/widgets/SymbolSearchDialog.cpp
    src/widgets/MetricsDialog.cpp
    src/widgets/BulkRenameDialog.cpp
    src/widgets/BookmarkDialog.cpp
    src/widgets/MarkdownPreviewDialog.cpp
    src/widgets/ImageViewerDialog.cpp
    src/widgets/HexViewDialog.cpp
    src/widgets/GitHistoryDialog.cpp
    src/widgets/GitTagsDialog.cpp
    src/widgets/PluginManagerDialog.cpp
    src/widgets/MergeEditorDialog.cpp
    src/widgets/SearchEditorDialog.cpp
    src/widgets/NewProjectDialog.cpp
    src/widgets/ClipboardDialog.cpp
    src/widgets/PromptLibraryDialog.cpp
    src/widgets/ModelArenaDialog.cpp
    src/widgets/AgentPanelDialog.cpp
)
set(HEADERS
    src/MainWindow.h
    src/core/Commands.h
    src/core/SettingsManager.h
    src/core/ThemeManager.h
    src/core/LanguageManager.h
    src/core/OllamaClient.h
    src/core/RagIndexer.h
    src/core/LspClient.h
    src/core/EditorConfig.h
    src/core/SpellChecker.h
    src/core/Spelling.h
    src/core/GitIgnore.h
    src/core/TrashManager.h
    src/core/ProjectSessions.h
    src/core/FileIcons.h
    src/core/AgentTools.h
    src/core/PathGuard.h
    src/core/VersoPaths.h
    src/core/UpdateChecker.h
    src/core/PluginEngine.h
    src/core/LeakWatch.h
    src/core/RagProgress.h
    src/core/SetupAdvisor.h
    src/core/CommandAudit.h
    src/core/AgentLoop.h
    src/core/PatchQueue.h
    src/core/ContextResolver.h
    src/core/ChatStore.h
    src/core/TokenStats.h
    src/core/ModelCapabilities.h
    src/core/ImageUtil.h
    src/core/Embedding.h
    src/core/EmbeddingClient.h
    src/core/HybridRanker.h
    src/core/ResponseScorer.h
    src/core/ConversationSummarizer.h
    src/core/AgentPolicy.h
    src/core/AgentBudget.h
    src/core/AgentMemory.h
    src/core/SkillRegistry.h
    src/core/SkillChain.h
    src/core/AgentReflection.h
    src/core/ProjectHealth.h
    src/core/AgentRunStore.h
    src/core/ai/LlmProvider.h
    src/core/ai/SecretStore.h
    src/core/ai/AiMessage.h
    src/core/ai/LlmClient.h
    src/core/ai/providers/ProviderCodec.h
    src/core/ai/providers/SseParser.h
    src/core/ai/AiToolBridge.h
    src/core/ai/ProviderPrefs.h
    src/core/ai/ProviderPricing.h
    src/core/ai/UsageLedger.h
    src/core/ai/ProviderHealth.h
    src/core/ai/TaskRouter.h
    src/core/ai/EmbedCache.h
    src/core/ai/EmbedBridge.h
    src/core/ai/AgentLlmAdapter.h
    src/core/ai/ProviderBench.h
    src/core/ai/AiRunner.h
    src/core/ai/ModelPool.h
    src/core/ai/AiProfiles.h
    src/core/StartupArgs.h
    src/core/ToolchainProbe.h
    src/core/TaskRunner.h
    src/core/PerfMonitor.h
    src/core/KeymapPresets.h
    src/core/BackupManager.h
    src/core/WorkspaceConfig.h
    src/core/SettingsIO.h
    src/core/AboutInfo.h
    src/core/ThemeTokens.h
    src/core/AccentColor.h
    src/core/UiMetrics.h
    src/core/Typography.h
    src/core/QssBuilder.h
    src/core/ThemeStore.h
    src/core/IconTheme.h
    src/core/Animator.h
    src/core/ToastManager.h
    src/core/LayoutPresets.h
    src/core/UiProfile.h
    src/core/TaskChain.h
    src/core/PluginEngine.h
    src/core/CollabMerge.h
    src/core/CollabSession.h
    src/core/GitVersionManager.h
    src/core/CacheCleaner.h
    src/core/ThreadMonitor.h
    src/core/LanguageSupport.h
    src/core/ClipboardRing.h
    src/core/ShortcutCheck.h
    src/core/A11yCheck.h
    src/core/GridCheck.h
    src/core/LspServers.h
    src/core/MergeParse.h
    src/core/SshConfig.h
    src/core/Stability.h
    src/core/BackupManager.h
    src/core/CrashHandler.h
    src/core/PerfTools.h
    src/core/WorkspaceSymbols.h
    src/core/CodeMetrics.h
    src/core/DuplicateFinder.h
    src/core/GitBlame.h
    src/core/PromptVars.h
    src/core/MacroRecorder.h
    src/core/ExternalTools.h
    src/core/WordDiff.h
    src/core/BracketDepth.h
    src/core/ScopeChain.h
    src/core/SearchMarks.h
    src/core/DiffGutter.h
    src/core/HoverCard.h
    src/core/SelectionGrow.h
    src/core/Density.h
    src/core/ColorBlind.h
    src/core/AutoTheme.h
    src/core/ProfileStore.h
    src/core/ThemeValidator.h
    src/core/CompletionList.h
    src/core/SymbolTree.h
    src/core/LocationSet.h
    src/core/TextEdits.h
    src/core/CodeActionList.h
    src/core/SignatureHelp.h
    src/core/InlayHintList.h
    src/core/SemanticTokens.h
    src/core/CallHierarchyTree.h
    src/core/MiParser.h
    src/core/BreakpointStore.h
    src/core/TestParser.h
    src/core/TestDiscovery.h
    src/core/GcovParser.h
    src/core/ProblemMatcher.h
    src/core/LaunchConfig.h
    src/core/GdbDriver.h
    src/core/ConnectionProfile.h
    src/core/SshSession.h
    src/core/RemoteFileSystem.h
    src/core/RemoteTaskRunner.h
    src/core/LspTransport.h
    src/core/GitRunner.h
    src/core/PortForwarder.h
    src/core/SnippetEngine.h
    src/core/SnippetManager.h
    src/core/FoldingRanges.h
    src/core/BlockSelect.h
    src/core/AutoPairs.h
    src/core/PasteTransform.h
    src/core/OutlineFallback.h
    src/core/LocalHistory.h
    src/core/ReplaceEngine.h
    src/core/WordComplete.h
    src/core/GhostCompletion.h
    src/core/InlineEdit.h
    src/core/DocGen.h
    src/core/TestGen.h
    src/core/CommitMsg.h
    src/core/PromptLibrary.h
    src/core/ContextBudget.h
    src/widgets/CodeEditor.h
    src/widgets/ExplorerPanel.h
    src/widgets/SearchPanel.h
    src/widgets/BreadcrumbBar.h
    src/widgets/Minimap.h
    src/widgets/GitPanel.h
    src/widgets/AiPanel.h
    src/widgets/SettingsDialog.h
    src/widgets/QuickOpenDialog.h
    src/widgets/ApplyEditDialog.h
    src/widgets/ProblemsPanel.h
    src/widgets/CommandPalette.h
    src/widgets/TerminalPanel.h
    src/widgets/DiffDialog.h
    src/widgets/GitTools.h
    src/widgets/TodoPanel.h
    src/widgets/CompareDialog.h
    src/widgets/PatchReviewDialog.h
    src/widgets/DiagnosticsDialog.h
    src/widgets/TaskPanel.h
    src/widgets/ThemeGalleryDialog.h
    src/widgets/WelcomeView.h
    src/widgets/EmptyState.h
    src/widgets/EditorFindBar.h
    src/widgets/ThemeEditorDialog.h
    src/widgets/TitleBar.h
    src/widgets/CompletionPopup.h
    src/widgets/SymbolPickerDialog.h
    src/widgets/ReferencesDialog.h
    src/widgets/RenamePreviewDialog.h
    src/widgets/CallHierarchyDialog.h
    src/widgets/DebugPanel.h
    src/widgets/TestExplorer.h
    src/widgets/RemoteConnectDialog.h
    src/widgets/RemoteExplorer.h
    src/widgets/RemoteTerminal.h
    src/widgets/PortForwardPanel.h
    src/widgets/OutlinePanel.h
    src/widgets/TimelinePanel.h
    src/widgets/SnippetDialog.h
    src/widgets/PromptLibraryDialog.h
    src/widgets/ModelArenaDialog.h
    src/widgets/AgentPanelDialog.h
    src/widgets/NewFileDialog.h
    src/widgets/FirstRunDialog.h
    src/widgets/ShortcutDialog.h
    src/widgets/SymbolSearchDialog.h
    src/widgets/MetricsDialog.h
    src/widgets/BulkRenameDialog.h
    src/widgets/BookmarkDialog.h
    src/widgets/MarkdownPreviewDialog.h
    src/widgets/ImageViewerDialog.h
    src/widgets/HexViewDialog.h
    src/widgets/GitHistoryDialog.h
    src/widgets/GitTagsDialog.h
    src/widgets/PluginManagerDialog.h
    src/widgets/MergeEditorDialog.h
    src/widgets/SearchEditorDialog.h
    src/widgets/NewProjectDialog.h
    src/widgets/ClipboardDialog.h
    src/core/BookmarkStore.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE src ${HUNSPELL_INCLUDE_DIRS})
if(LIBSSH2_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${LIBSSH2_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBSSH2_LIBRARIES})
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Network Qt6::Concurrent Qt6::Svg Qt6::Qml Qt6::WebSockets ${HUNSPELL_LIBRARIES})
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Stage 31: çökme izinde sembol adları (backtrace_symbols)
    target_link_options(${PROJECT_NAME} PRIVATE "LINKER:--export-dynamic")
endif()

# --- Stage 19: birim testleri (saf mantık; widgets yok) ---
add_executable(test_stage19
    tests/test_stage19.cpp
    src/core/TaskChain.cpp
    src/core/PluginEngine.cpp
    src/core/PathGuard.cpp
    src/core/CommandAudit.cpp
    src/core/CollabMerge.cpp
    src/core/CollabSession.cpp
    src/core/GitVersionManager.cpp
    src/core/CacheCleaner.cpp
    src/core/ai/ModelPool.cpp
    src/core/ModelCapabilities.cpp
    src/core/ai/ProviderPricing.cpp
    src/core/ai/LlmProvider.cpp
    src/core/ai/SecretStore.cpp
    src/core/ai/AiMessage.cpp
    src/core/ThreadMonitor.cpp
    src/core/LanguageSupport.cpp
)
target_include_directories(test_stage19 PRIVATE src)
target_link_libraries(test_stage19 PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Concurrent Qt6::Qml Qt6::WebSockets)
add_test(NAME stage19 COMMAND test_stage19)

# --- Stage 20: yeni dosya dilleri (saf mantık) ---
add_executable(test_stage20
    tests/test_stage20.cpp
    src/core/LanguageSupport.cpp
)
target_include_directories(test_stage20 PRIVATE src)
target_link_libraries(test_stage20 PRIVATE Qt6::Core)
add_test(NAME stage20 COMMAND test_stage20)

# --- Stage 21: pano + çakışma + skor + bütçe + şablon ---
add_executable(test_stage21
    tests/test_stage21.cpp
    src/widgets/CommandPalette.cpp
    src/core/ContextBudget.cpp
    src/core/CommitMsg.cpp
)
target_include_directories(test_stage21 PRIVATE src)
target_link_libraries(test_stage21 PRIVATE Qt6::Core Qt6::Widgets)
add_test(NAME stage21 COMMAND test_stage21)

# --- Stage 22: erişilebilirlik + çeviri kapsama ---
add_executable(test_stage22
    tests/test_stage22.cpp
    src/core/A11yCheck.cpp
    src/core/LanguageManager.cpp
    src/core/ThemeTokens.cpp
)
target_include_directories(test_stage22 PRIVATE src)
target_link_libraries(test_stage22 PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
add_test(NAME stage22 COMMAND test_stage22)

# --- Stage 23: piksel-hizalı ızgara ---
add_executable(test_stage23
    tests/test_stage23.cpp
    src/core/GridCheck.cpp
)
target_include_directories(test_stage23 PRIVATE src)
target_link_libraries(test_stage23 PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
add_test(NAME stage23 COMMAND test_stage23)

# --- Stage 24: sembol/metrik/kopya/blame/todo ---
add_executable(test_stage24
    tests/test_stage24.cpp
    src/core/WorkspaceSymbols.cpp
    src/core/CodeMetrics.cpp
    src/core/DuplicateFinder.cpp
    src/core/GitBlame.cpp
    src/widgets/TodoPanel.cpp
)
target_include_directories(test_stage24 PRIVATE src)
target_link_libraries(test_stage24 PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Concurrent)
add_test(NAME stage24 COMMAND test_stage24)

# --- Stage 25: istem/makro/araç ---
add_executable(test_stage25
    tests/test_stage25.cpp
    src/core/PromptVars.cpp
    src/core/MacroRecorder.cpp
    src/core/ExternalTools.cpp
)
target_include_directories(test_stage25 PRIVATE src)
target_link_libraries(test_stage25 PRIVATE Qt6::Core)
add_test(NAME stage25 COMMAND test_stage25)

# --- Stage 26: MI + launch ---
add_executable(test_stage26
    tests/test_stage26.cpp
    src/core/MiParser.cpp
    src/core/GdbDriver.cpp
    src/core/LaunchConfig.cpp
)
target_include_directories(test_stage26 PRIVATE src)
target_link_libraries(test_stage26 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)
add_test(NAME stage26 COMMAND test_stage26)

# --- Stage 27: sunucu tablosu + venv + URI ---
add_executable(test_stage27
    tests/test_stage27.cpp
    src/core/LspServers.cpp
    src/core/LspClient.cpp
)
target_include_directories(test_stage27 PRIVATE src)
target_link_libraries(test_stage27 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)
add_test(NAME stage27 COMMAND test_stage27)

# --- Stage 28: yer imi + yorum + girdi + shebang ---
add_executable(test_stage28
    tests/test_stage28.cpp
    src/core/LanguageSupport.cpp
    src/core/TaskChain.cpp
)
target_include_directories(test_stage28 PRIVATE src)
target_link_libraries(test_stage28 PRIVATE Qt6::Core Qt6::Widgets)
add_test(NAME stage28 COMMAND test_stage28)

# --- Stage 29: eklenti v2 ---
add_executable(test_stage29
    tests/test_stage29.cpp
    src/core/PluginEngine.cpp
    src/core/PathGuard.cpp
    src/core/CommandAudit.cpp
)
target_include_directories(test_stage29 PRIVATE src)
target_link_libraries(test_stage29 PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Qml)
add_test(NAME stage29 COMMAND test_stage29)

# --- Stage 30: birleştirme + ssh + pano + zincir ---
add_executable(test_stage30
    tests/test_stage30.cpp
    src/core/MergeParse.cpp
    src/core/SshConfig.cpp
    src/core/TaskChain.cpp
)
target_include_directories(test_stage30 PRIVATE src)
target_link_libraries(test_stage30 PRIVATE Qt6::Core Qt6::Widgets)
add_test(NAME stage30 COMMAND test_stage30)

# --- Stage 31: bütünlük + geri çekilme + rotasyon + kota + MI fuzz ---
add_executable(test_stage31
    tests/test_stage31.cpp
    src/core/Stability.cpp
    src/core/BackupManager.cpp
    src/core/CrashHandler.cpp
    src/core/MiParser.cpp
)
target_include_directories(test_stage31 PRIVATE src)
target_link_libraries(test_stage31 PRIVATE Qt6::Core Qt6::Widgets)
add_test(NAME stage31 COMMAND test_stage31)

# --- Stage 32: hız yardımcıları + artımlı RAG + ters indeks ---
add_executable(test_stage32
    tests/test_stage32.cpp
    src/core/PerfTools.cpp
    src/core/RagIndexer.cpp
    src/core/Embedding.cpp
    src/core/HybridRanker.cpp
)
target_include_directories(test_stage32 PRIVATE src)
target_link_libraries(test_stage32 PRIVATE Qt6::Core Qt6::Widgets Qt6::Concurrent)
add_test(NAME stage32 COMMAND test_stage32)

# --- Stage 33: yetenek/görü/gömme/hibrit/dallanma ---
add_executable(test_stage33
    tests/test_stage33.cpp
    src/core/ModelCapabilities.cpp
    src/core/ai/ProviderPricing.cpp
    src/core/ai/LlmProvider.cpp
    src/core/ai/SecretStore.cpp
    src/core/ai/AiMessage.cpp
    src/core/ImageUtil.cpp
    src/core/Embedding.cpp
    src/core/EmbeddingClient.cpp
    src/core/HybridRanker.cpp
    src/core/ResponseScorer.cpp
    src/core/ConversationSummarizer.cpp
    src/core/ContextBudget.cpp
    src/core/ChatStore.cpp
)
target_include_directories(test_stage33 PRIVATE src)
target_link_libraries(test_stage33 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)
add_test(NAME stage33 COMMAND test_stage33)

# --- Stage 34: politika/bütçe/beceri/bellek/refleksiyon/otonomi ---
add_executable(test_stage34
    tests/test_stage34.cpp
    src/core/AgentPolicy.cpp
    src/core/AgentBudget.cpp
    src/core/AgentMemory.cpp
    src/core/SkillRegistry.cpp
    src/core/SkillChain.cpp
    src/core/AgentReflection.cpp
    src/core/ProjectHealth.cpp
    src/core/AgentRunStore.cpp
    src/core/AgentTools.cpp
    src/core/PathGuard.cpp
    src/core/CommandAudit.cpp
    src/core/AgentLoop.cpp
    src/core/PatchQueue.cpp
)
target_include_directories(test_stage34 PRIVATE src)
target_link_libraries(test_stage34 PRIVATE Qt6::Core Qt6::Widgets)
add_test(NAME stage34 COMMAND test_stage34)

# --- Stage 35: çok sağlayıcılı AI ---
add_executable(test_stage35
    tests/test_stage35.cpp
    src/core/ai/LlmProvider.cpp
    src/core/ai/SecretStore.cpp
    src/core/ai/AiMessage.cpp
    src/core/ai/AiToolBridge.cpp
    src/core/ai/ProviderPrefs.cpp
    src/core/ai/ProviderPricing.cpp
    src/core/ai/UsageLedger.cpp
    src/core/ai/ProviderHealth.cpp
    src/core/ai/TaskRouter.cpp
    src/core/ai/ModelPool.cpp
    src/core/ModelCapabilities.cpp
    src/core/ai/EmbedCache.cpp
    src/core/ai/EmbedBridge.cpp
    src/core/ai/AgentLlmAdapter.cpp
    src/core/ai/ProviderBench.cpp
    src/core/ai/LlmClient.cpp
    src/core/ai/providers/ProviderCodec.cpp
    src/core/ai/providers/SseParser.cpp
    src/core/AgentTools.cpp
    src/core/PathGuard.cpp
    src/core/CommandAudit.cpp
    src/core/AgentPolicy.cpp
    src/core/PatchQueue.cpp
    src/core/ProjectHealth.cpp
)
target_include_directories(test_stage35 PRIVATE src)
target_link_libraries(test_stage35 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)
add_test(NAME stage35 COMMAND test_stage35)

# --- Stage 36: ekonomi, kota, sağlık/failover, yönlendirme, gömme köprüsü ---
add_executable(test_stage36
    tests/test_stage36.cpp
    src/core/TokenStats.cpp
    src/core/ModelCapabilities.cpp
    src/core/ai/LlmProvider.cpp
    src/core/ai/SecretStore.cpp
    src/core/ai/AiMessage.cpp
    src/core/ai/AiToolBridge.cpp
    src/core/ai/ProviderPrefs.cpp
    src/core/ai/ProviderPricing.cpp
    src/core/ai/UsageLedger.cpp
    src/core/ai/ProviderHealth.cpp
    src/core/ai/TaskRouter.cpp
    src/core/ai/EmbedCache.cpp
    src/core/ai/EmbedBridge.cpp
    src/core/ai/AgentLlmAdapter.cpp
    src/core/ai/ProviderBench.cpp
    src/core/ai/LlmClient.cpp
    src/core/ai/providers/ProviderCodec.cpp
    src/core/ai/providers/SseParser.cpp
    src/core/AgentTools.cpp
    src/core/AgentPolicy.cpp
    src/core/AgentBudget.cpp
    src/core/AgentMemory.cpp
    src/core/AgentReflection.cpp
    src/core/AgentLoop.cpp
    src/core/PatchQueue.cpp
    src/core/ProjectHealth.cpp
    src/core/PathGuard.cpp
    src/core/CommandAudit.cpp
    src/core/ai/ModelPool.cpp
)
target_include_directories(test_stage36 PRIVATE src)
target_link_libraries(test_stage36 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)
add_test(NAME stage36 COMMAND test_stage36)

# --- Stage 37: tek AI cephesi (AiRunner) + görev profilleri ---
add_executable(test_stage37
    tests/test_stage37.cpp
    src/core/ai/ModelPool.cpp
    src/widgets/ModelArenaDialog.cpp
    src/core/ai/AiRunner.cpp
    src/core/ai/LlmProvider.cpp
    src/core/ai/SecretStore.cpp
    src/core/ai/AiMessage.cpp
    src/core/ai/AiToolBridge.cpp
    src/core/ai/ProviderPrefs.cpp
    src/core/ai/ProviderPricing.cpp
    src/core/ai/ProviderHealth.cpp
    src/core/ai/UsageLedger.cpp
    src/core/ai/TaskRouter.cpp
    src/core/ai/EmbedCache.cpp
    src/core/ai/EmbedBridge.cpp
    src/core/ai/ProviderBench.cpp
    src/core/ai/LlmClient.cpp
    src/core/ai/providers/ProviderCodec.cpp
    src/core/ai/providers/SseParser.cpp
    src/core/GhostCompletion.cpp
    src/core/PromptLibrary.cpp
    src/core/ResponseScorer.cpp
    src/core/ConversationSummarizer.cpp
    src/core/ContextBudget.cpp
    src/core/ModelCapabilities.cpp
    src/core/TokenStats.cpp
    src/core/AgentTools.cpp
    src/core/PathGuard.cpp
    src/core/CommandAudit.cpp
    src/core/AgentPolicy.cpp
    src/core/PatchQueue.cpp
    src/core/ProjectHealth.cpp
)
target_include_directories(test_stage37 PRIVATE src)
target_link_libraries(test_stage37 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Concurrent)
add_test(NAME stage37 COMMAND test_stage37)

# QSS dosyalarını binary yanına kopyala
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources)

# --- Stage 8: kurulum / dağıtım ---
include(GNUInstallDirs)
enable_testing()
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
install(FILES ${CMAKE_SOURCE_DIR}/packaging/verso-coder.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
# Stage 50-debug: simge kurulumu (desktop dosyası Icon=verso-coder diyordu ama
# simge hiç kurulmuyordu) + kurulum öneki tanımı (GNUInstallDirs sonrası)
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/${PROJECT_NAME})
install(FILES ${CMAKE_SOURCE_DIR}/resources/icon.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
        RENAME verso-coder.svg)
target_compile_definitions(${PROJECT_NAME} PRIVATE VERSO_DATA_DIR="${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}")
install(FILES ${CMAKE_SOURCE_DIR}/packaging/io.verso.Coder.appdata.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

# --- Stage 37: canlı uçtan uca AI doğrulama aracı (geliştirici aracı; ctest'e girmez) ---
add_executable(ai-probe EXCLUDE_FROM_ALL
    tools/ai_probe.cpp
    src/core/ModelCapabilities.cpp
    src/core/ai/ModelPool.cpp
    src/core/ai/AiRunner.cpp
    src/core/ai/LlmProvider.cpp
    src/core/ai/SecretStore.cpp
    src/core/ai/AiMessage.cpp
    src/core/ai/AiToolBridge.cpp
    src/core/ai/ProviderPrefs.cpp
    src/core/ai/ProviderPricing.cpp
    src/core/ai/ProviderHealth.cpp
    src/core/ai/UsageLedger.cpp
    src/core/ai/TaskRouter.cpp
    src/core/ai/EmbedCache.cpp
    src/core/ai/EmbedBridge.cpp
    src/core/ai/AgentLlmAdapter.cpp
    src/core/ai/ProviderBench.cpp
    src/core/ai/LlmClient.cpp
    src/core/ai/providers/ProviderCodec.cpp
    src/core/ai/providers/SseParser.cpp
    src/core/AgentTools.cpp
    src/core/AboutInfo.cpp
    src/core/AgentLoop.cpp
    src/core/AgentPolicy.cpp
    src/core/AgentBudget.cpp
    src/core/AgentMemory.cpp
    src/core/AgentReflection.cpp
    src/core/PatchQueue.cpp
    src/core/ProjectHealth.cpp
    src/core/PathGuard.cpp
    src/core/CommandAudit.cpp
    src/core/RagProgress.cpp
    src/core/SetupAdvisor.cpp
)
target_include_directories(ai-probe PRIVATE src)
target_link_libraries(ai-probe PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)

# --- Stage 38: arayüz (widget) testleri — gerçek diyaloglar, offscreen ---
find_package(Qt6 COMPONENTS Test QUIET)
if(Qt6Test_FOUND)
    add_executable(test_ui_widgets
        tests/ui/test_ui_widgets.cpp
        src/core/PathGuard.cpp
        src/core/CommandAudit.cpp
        src/core/AgentTools.cpp
        src/core/AgentPolicy.cpp
        src/core/AgentBudget.cpp
        src/core/AgentMemory.cpp
        src/core/AgentReflection.cpp
        src/core/SkillRegistry.cpp
        src/core/SkillChain.cpp
        src/core/AgentRunStore.cpp
        src/core/PatchQueue.cpp
        src/core/ProjectHealth.cpp
        src/core/ModelCapabilities.cpp
        src/core/TokenStats.cpp
        src/core/ContextBudget.cpp
        src/core/ResponseScorer.cpp
        src/core/Embedding.cpp
        src/core/EmbeddingClient.cpp
        src/core/RagIndexer.cpp
        src/core/HybridRanker.cpp
        src/core/ChatStore.cpp
        src/core/ImageUtil.cpp
        src/core/SettingsManager.cpp
        src/core/ThemeManager.cpp
    src/core/VersoPaths.cpp
        src/core/ThemeTokens.cpp
        src/core/ThemeStore.cpp
        src/core/ai/LlmProvider.cpp
        src/core/ai/SecretStore.cpp
        src/core/ai/AiMessage.cpp
        src/core/ai/AiToolBridge.cpp
        src/core/ai/ProviderPrefs.cpp
        src/core/ai/ProviderPricing.cpp
        src/core/ai/ProviderHealth.cpp
        src/core/ai/UsageLedger.cpp
        src/core/ai/TaskRouter.cpp
        src/core/ai/ModelPool.cpp
        src/core/ai/EmbedCache.cpp
        src/core/ai/EmbedBridge.cpp
        src/core/ai/AgentLlmAdapter.cpp
        src/core/ai/ProviderBench.cpp
        src/core/ai/AiRunner.cpp
        src/core/ai/LlmClient.cpp
        src/core/ai/providers/ProviderCodec.cpp
        src/core/ai/providers/SseParser.cpp
        src/widgets/SettingsDialog.cpp
    src/core/SetupAdvisor.cpp
    src/core/CacheCleaner.cpp
        src/widgets/ModelArenaDialog.cpp
        src/widgets/AgentPanelDialog.cpp
        src/widgets/PatchReviewDialog.cpp
        src/widgets/EmptyState.cpp
        src/core/IconTheme.cpp
        src/core/PerfTools.cpp
        src/core/SettingsIO.cpp
        src/core/AccentColor.cpp
        src/core/Animator.cpp
        src/core/ColorBlind.cpp
        src/core/KeymapPresets.cpp
        src/core/OllamaClient.cpp
        src/core/Density.cpp
        src/core/UiMetrics.cpp
        src/core/Typography.cpp
        src/core/QssBuilder.cpp
    )
    target_include_directories(test_ui_widgets PRIVATE src)
    target_link_libraries(test_ui_widgets PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Concurrent Qt6::Svg Qt6::Test)

    add_test(NAME ui_widgets COMMAND test_ui_widgets)
    set_tests_properties(ui_widgets PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;XDG_DATA_HOME=${CMAKE_CURRENT_BINARY_DIR}/ui-home;XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/ui-home")
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ui-home)

    # --- Stage 38: duman otomasyonu (MainWindow'ı gerçekten kurar) ---
    # Uygulamanın KENDİ kaynak listesi kullanılır (${SOURCES}); böylece
    # panel bağımlılıkları elle kopyalanmaz ve iki hedef asla ayrışmaz.
    set(VERSO_UI_SOURCES ${SOURCES})
    list(REMOVE_ITEM VERSO_UI_SOURCES src/main.cpp) # ikinci main olurdu
    # Stage 38: yeni çekirdek dosyalar SOURCES'a zaten ekli
    add_executable(test_ui_smoke tests/ui/test_ui_smoke.cpp ${VERSO_UI_SOURCES})
    target_include_directories(test_ui_smoke PRIVATE src)
    target_link_libraries(test_ui_smoke PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Concurrent Qt6::Svg Qt6::Qml Qt6::WebSockets Qt6::Test ${HUNSPELL_LIBRARIES})
    target_include_directories(test_ui_smoke PRIVATE ${HUNSPELL_INCLUDE_DIRS})
    if(LIBSSH2_FOUND)
        target_link_libraries(test_ui_smoke PRIVATE ${LIBSSH2_LIBRARIES})
        target_compile_definitions(test_ui_smoke PRIVATE VERSO_HAVE_LIBSSH2)
    endif()
    add_test(NAME ui_smoke COMMAND test_ui_smoke)
    set_tests_properties(ui_smoke PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;XDG_DATA_HOME=${CMAKE_CURRENT_BINARY_DIR}/ui-smoke-home;XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/ui-smoke-home")
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ui-smoke-home)
endif()

# --- Stage 38: güvenlik, doğrulama, dayanıklılık ---
add_executable(test_stage38
    tests/test_stage38.cpp
    src/core/PathGuard.cpp
    src/core/CommandAudit.cpp
    src/core/LeakWatch.cpp
    src/core/RagProgress.cpp
    src/core/SetupAdvisor.cpp
    src/core/CacheCleaner.cpp
    src/core/AgentTools.cpp
    src/core/AgentPolicy.cpp
    src/core/AgentBudget.cpp
    src/core/AgentMemory.cpp
    src/core/AgentReflection.cpp
    src/core/SkillRegistry.cpp
    src/core/SkillChain.cpp
    src/core/AgentRunStore.cpp
    src/core/PatchQueue.cpp
    src/core/ProjectHealth.cpp
    src/core/ModelCapabilities.cpp
    src/core/TokenStats.cpp
    src/core/ContextBudget.cpp
    src/core/ai/LlmProvider.cpp
    src/core/ai/SecretStore.cpp
    src/core/ai/AiMessage.cpp
    src/core/ai/AiToolBridge.cpp
    src/core/ai/ProviderPrefs.cpp
    src/core/ai/ProviderPricing.cpp
    src/core/ai/ProviderHealth.cpp
    src/core/ai/UsageLedger.cpp
    src/core/ai/TaskRouter.cpp
    src/core/ai/ModelPool.cpp
    src/core/ai/EmbedCache.cpp
    src/core/ai/EmbedBridge.cpp
    src/core/ai/AgentLlmAdapter.cpp
    src/core/ai/ProviderBench.cpp
    src/core/ai/AiRunner.cpp
    src/core/ai/LlmClient.cpp
    src/core/ai/providers/ProviderCodec.cpp
    src/core/ai/providers/SseParser.cpp
)
target_include_directories(test_stage38 PRIVATE src)
target_link_libraries(test_stage38 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)
add_test(NAME stage38 COMMAND test_stage38)

# --- Stage 39: canlı doğrulama + kalan sertleştirme ---
add_executable(test_stage39
    tests/test_stage39.cpp
    src/core/PathGuard.cpp
    src/core/CommandAudit.cpp
    src/core/LeakWatch.cpp
    src/core/RagProgress.cpp
    src/core/SetupAdvisor.cpp
    src/core/CacheCleaner.cpp
    src/core/AgentTools.cpp
    src/core/AgentPolicy.cpp
    src/core/AgentBudget.cpp
    src/core/AgentMemory.cpp
    src/core/AgentReflection.cpp
    src/core/SkillRegistry.cpp
    src/core/SkillChain.cpp
    src/core/AgentRunStore.cpp
    src/core/PatchQueue.cpp
    src/core/ProjectHealth.cpp
    src/core/ModelCapabilities.cpp
    src/core/TokenStats.cpp
    src/core/ContextBudget.cpp
    src/core/ai/LlmProvider.cpp
    src/core/ai/SecretStore.cpp
    src/core/ai/AiMessage.cpp
    src/core/ai/AiToolBridge.cpp
    src/core/ai/ProviderPrefs.cpp
    src/core/ai/ProviderPricing.cpp
    src/core/ai/ProviderHealth.cpp
    src/core/ai/UsageLedger.cpp
    src/core/ai/TaskRouter.cpp
    src/core/ai/ModelPool.cpp
    src/core/ai/EmbedCache.cpp
    src/core/ai/EmbedBridge.cpp
    src/core/ai/AgentLlmAdapter.cpp
    src/core/ai/ProviderBench.cpp
    src/core/ai/AiRunner.cpp
    src/core/ai/LlmClient.cpp
    src/core/ai/providers/ProviderCodec.cpp
    src/core/ai/providers/SseParser.cpp
)
target_include_directories(test_stage39 PRIVATE src)
target_link_libraries(test_stage39 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)
add_test(NAME stage39 COMMAND test_stage39)

add_executable(test_stage40
    tests/test_stage40.cpp
    src/core/PathGuard.cpp
    src/core/CommandAudit.cpp
    src/core/LeakWatch.cpp
    src/core/RagProgress.cpp
    src/core/SetupAdvisor.cpp
    src/core/CacheCleaner.cpp
    src/core/AgentTools.cpp
    src/core/AgentPolicy.cpp
    src/core/AgentBudget.cpp
    src/core/AgentMemory.cpp
    src/core/AgentReflection.cpp
    src/core/SkillRegistry.cpp
    src/core/SkillChain.cpp
    src/core/AgentRunStore.cpp
    src/core/PatchQueue.cpp
    src/core/ProjectHealth.cpp
    src/core/ModelCapabilities.cpp
    src/core/TokenStats.cpp
    src/core/ContextBudget.cpp
    src/core/ai/LlmProvider.cpp
    src/core/ai/SecretStore.cpp
    src/core/ai/AiMessage.cpp
    src/core/ai/AiToolBridge.cpp
    src/core/ai/ProviderPrefs.cpp
    src/core/ai/ProviderPricing.cpp
    src/core/ai/ProviderHealth.cpp
    src/core/ai/UsageLedger.cpp
    src/core/ai/TaskRouter.cpp
    src/core/ai/ModelPool.cpp
    src/core/ai/EmbedCache.cpp
    src/core/ai/EmbedBridge.cpp
    src/core/ai/AgentLlmAdapter.cpp
    src/core/ai/ProviderBench.cpp
    src/core/ai/AiRunner.cpp
    src/core/ai/LlmClient.cpp
    src/core/ai/providers/ProviderCodec.cpp
    src/core/ai/providers/SseParser.cpp
)
target_include_directories(test_stage40 PRIVATE src)
target_link_libraries(test_stage40 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)
add_test(NAME stage40 COMMAND test_stage40)

add_executable(test_stage41
    tests/test_stage41.cpp
    src/core/PluginEngine.cpp
    src/core/PathGuard.cpp
    src/core/CommandAudit.cpp
    src/core/LeakWatch.cpp
    src/core/RagProgress.cpp
    src/core/SetupAdvisor.cpp
    src/core/CacheCleaner.cpp
    src/core/AgentTools.cpp
    src/core/AgentPolicy.cpp
    src/core/AgentBudget.cpp
    src/core/AgentMemory.cpp
    src/core/AgentReflection.cpp
    src/core/SkillRegistry.cpp
    src/core/SkillChain.cpp
    src/core/AgentRunStore.cpp
    src/core/PatchQueue.cpp
    src/core/ProjectHealth.cpp
    src/core/ModelCapabilities.cpp
    src/core/TokenStats.cpp
    src/core/ContextBudget.cpp
    src/core/ai/LlmProvider.cpp
    src/core/ai/SecretStore.cpp
    src/core/ai/AiMessage.cpp
    src/core/ai/AiToolBridge.cpp
    src/core/ai/ProviderPrefs.cpp
    src/core/ai/ProviderPricing.cpp
    src/core/ai/ProviderHealth.cpp
    src/core/ai/UsageLedger.cpp
    src/core/ai/TaskRouter.cpp
    src/core/ai/ModelPool.cpp
    src/core/ai/EmbedCache.cpp
    src/core/ai/EmbedBridge.cpp
    src/core/ai/AgentLlmAdapter.cpp
    src/core/ai/ProviderBench.cpp
    src/core/ai/AiRunner.cpp
    src/core/ai/LlmClient.cpp
    src/core/ai/providers/ProviderCodec.cpp
    src/core/ai/providers/SseParser.cpp
)
target_include_directories(test_stage41 PRIVATE src)
target_link_libraries(test_stage41 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Qml)
add_test(NAME stage41 COMMAND test_stage41)

add_executable(test_stage42
    tests/test_stage42.cpp
    src/core/PluginEngine.cpp
    src/core/LspClient.cpp
    src/core/LspTransport.cpp
    src/core/SshSession.cpp
    src/core/PathGuard.cpp
    src/core/CommandAudit.cpp
    src/core/LeakWatch.cpp
    src/core/RagProgress.cpp
    src/core/SetupAdvisor.cpp
    src/core/CacheCleaner.cpp
    src/core/AgentTools.cpp
    src/core/AgentPolicy.cpp
    src/core/AgentBudget.cpp
    src/core/AgentMemory.cpp
    src/core/AgentReflection.cpp
    src/core/SkillRegistry.cpp
    src/core/SkillChain.cpp
    src/core/AgentRunStore.cpp
    src/core/PatchQueue.cpp
    src/core/ProjectHealth.cpp
    src/core/ModelCapabilities.cpp
    src/core/TokenStats.cpp
    src/core/ContextBudget.cpp
    src/core/ai/LlmProvider.cpp
    src/core/ai/SecretStore.cpp
    src/core/ai/AiMessage.cpp
    src/core/ai/AiToolBridge.cpp
    src/core/ai/ProviderPrefs.cpp
    src/core/ai/ProviderPricing.cpp
    src/core/ai/ProviderHealth.cpp
    src/core/ai/UsageLedger.cpp
    src/core/ai/TaskRouter.cpp
    src/core/ai/ModelPool.cpp
    src/core/ai/EmbedCache.cpp
    src/core/ai/EmbedBridge.cpp
    src/core/ai/AgentLlmAdapter.cpp
    src/core/ai/ProviderBench.cpp
    src/core/ai/AiRunner.cpp
    src/core/ai/LlmClient.cpp
    src/core/ai/providers/ProviderCodec.cpp
    src/core/ai/providers/SseParser.cpp
)
target_include_directories(test_stage42 PRIVATE src)
target_link_libraries(test_stage42 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Qml)
add_test(NAME stage42 COMMAND test_stage42)

add_executable(test_stage43
    tests/test_stage43.cpp
    src/core/PluginEngine.cpp
    src/core/GitRunner.cpp
    src/core/SshSession.cpp
    src/core/ConnectionProfile.cpp
    src/core/PathGuard.cpp
    src/core/CommandAudit.cpp
    src/core/LeakWatch.cpp
    src/core/RagProgress.cpp
    src/core/SetupAdvisor.cpp
    src/core/CacheCleaner.cpp
    src/core/AgentTools.cpp
    src/core/AgentPolicy.cpp
    src/core/AgentBudget.cpp
    src/core/AgentMemory.cpp
    src/core/AgentReflection.cpp
    src/core/SkillRegistry.cpp
    src/core/SkillChain.cpp
    src/core/AgentRunStore.cpp
    src/core/PatchQueue.cpp
    src/core/ProjectHealth.cpp
    src/core/ModelCapabilities.cpp
    src/core/TokenStats.cpp
    src/core/ContextBudget.cpp
    src/core/ai/LlmProvider.cpp
    src/core/ai/SecretStore.cpp
    src/core/ai/AiMessage.cpp
    src/core/ai/AiToolBridge.cpp
    src/core/ai/ProviderPrefs.cpp
    src/core/ai/ProviderPricing.cpp
    src/core/ai/ProviderHealth.cpp
    src/core/ai/UsageLedger.cpp
    src/core/ai/TaskRouter.cpp
    src/core/ai/ModelPool.cpp
    src/core/ai/EmbedCache.cpp
    src/core/ai/EmbedBridge.cpp
    src/core/ai/AgentLlmAdapter.cpp
    src/core/ai/ProviderBench.cpp
    src/core/ai/AiRunner.cpp
    src/core/ai/LlmClient.cpp
    src/core/ai/providers/ProviderCodec.cpp
    src/core/ai/providers/SseParser.cpp
)
target_include_directories(test_stage43 PRIVATE src)
target_link_libraries(test_stage43 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Qml)
add_test(NAME stage43 COMMAND test_stage43)

add_executable(test_stage44
    tests/test_stage44.cpp
    src/core/PluginEngine.cpp
    src/core/SshSession.cpp
    src/core/ConnectionProfile.cpp
    src/core/PathGuard.cpp
    src/core/CommandAudit.cpp
    src/core/LeakWatch.cpp
    src/core/RagProgress.cpp
    src/core/SetupAdvisor.cpp
    src/core/CacheCleaner.cpp
    src/core/AgentTools.cpp
    src/core/AgentPolicy.cpp
    src/core/AgentBudget.cpp
    src/core/AgentMemory.cpp
    src/core/AgentReflection.cpp
    src/core/SkillRegistry.cpp
    src/core/SkillChain.cpp
    src/core/AgentRunStore.cpp
    src/core/PatchQueue.cpp
    src/core/ProjectHealth.cpp
    src/core/ModelCapabilities.cpp
    src/core/TokenStats.cpp
    src/core/ContextBudget.cpp
    src/core/ai/LlmProvider.cpp
    src/core/ai/SecretStore.cpp
    src/core/ai/AiMessage.cpp
    src/core/ai/AiToolBridge.cpp
    src/core/ai/ProviderPrefs.cpp
    src/core/ai/ProviderPricing.cpp
    src/core/ai/ProviderHealth.cpp
    src/core/ai/UsageLedger.cpp
    src/core/ai/TaskRouter.cpp
    src/core/ai/ModelPool.cpp
    src/core/ai/EmbedCache.cpp
    src/core/ai/EmbedBridge.cpp
    src/core/ai/AgentLlmAdapter.cpp
    src/core/ai/ProviderBench.cpp
    src/core/ai/AiRunner.cpp
    src/core/ai/LlmClient.cpp
    src/core/ai/providers/ProviderCodec.cpp
    src/core/ai/providers/SseParser.cpp
)
target_include_directories(test_stage44 PRIVATE src)
target_link_libraries(test_stage44 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Qml)
add_test(NAME stage44 COMMAND test_stage44)

add_executable(test_stage45
    tests/test_stage45.cpp
    src/core/PluginEngine.cpp
    src/core/AgentLoop.cpp
    src/core/PathGuard.cpp
    src/core/CommandAudit.cpp
    src/core/LeakWatch.cpp
    src/core/RagProgress.cpp
    src/core/SetupAdvisor.cpp
    src/core/CacheCleaner.cpp
    src/core/AgentTools.cpp
    src/core/AgentPolicy.cpp
    src/core/AgentBudget.cpp
    src/core/AgentMemory.cpp
    src/core/AgentReflection.cpp
    src/core/SkillRegistry.cpp
    src/core/SkillChain.cpp
    src/core/AgentRunStore.cpp
    src/core/PatchQueue.cpp
    src/core/ProjectHealth.cpp
    src/core/ModelCapabilities.cpp
    src/core/TokenStats.cpp
    src/core/ContextBudget.cpp
    src/core/ai/LlmProvider.cpp
    src/core/ai/SecretStore.cpp
    src/core/ai/AiMessage.cpp
    src/core/ai/AiToolBridge.cpp
    src/core/ai/ProviderPrefs.cpp
    src/core/ai/ProviderPricing.cpp
    src/core/ai/ProviderHealth.cpp
    src/core/ai/UsageLedger.cpp
    src/core/ai/TaskRouter.cpp
    src/core/ai/ModelPool.cpp
    src/core/ai/EmbedCache.cpp
    src/core/ai/EmbedBridge.cpp
    src/core/ai/AgentLlmAdapter.cpp
    src/core/ai/ProviderBench.cpp
    src/core/ai/AiRunner.cpp
    src/core/ai/LlmClient.cpp
    src/core/ai/providers/ProviderCodec.cpp
    src/core/ai/providers/SseParser.cpp
)
target_include_directories(test_stage45 PRIVATE src)
target_link_libraries(test_stage45 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Qml)
add_test(NAME stage45 COMMAND test_stage45)

add_executable(test_stage46
    tests/test_stage46.cpp
    src/widgets/CodeEditor.cpp
    src/core/PathGuard.cpp
    src/core/CommandAudit.cpp
    src/core/LeakWatch.cpp
    src/core/SettingsManager.cpp
    src/core/SettingsIO.cpp
    src/core/AutoPairs.cpp
    src/core/BlockSelect.cpp
    src/core/BracketDepth.cpp
    src/core/EditorConfig.cpp
    src/core/ScopeChain.cpp
    src/core/SelectionGrow.cpp
    src/core/SemanticTokens.cpp
    src/core/SnippetEngine.cpp
    src/core/SpellChecker.cpp
    src/core/Spelling.cpp
    src/core/Typography.cpp
    src/core/ThemeTokens.cpp
    src/core/ThemeManager.cpp
    src/core/VersoPaths.cpp
    src/core/ThemeStore.cpp
    src/core/GitBlame.cpp
    src/core/GitIgnore.cpp
    src/core/ColorBlind.cpp
    src/core/QssBuilder.cpp
    src/core/AccentColor.cpp
    src/core/Density.cpp
    src/core/UiMetrics.cpp
    src/core/RagProgress.cpp
    src/core/SetupAdvisor.cpp
    src/core/CacheCleaner.cpp
    src/core/AgentTools.cpp
    src/core/AgentPolicy.cpp
    src/core/AgentBudget.cpp
    src/core/AgentMemory.cpp
    src/core/AgentReflection.cpp
    src/core/SkillRegistry.cpp
    src/core/SkillChain.cpp
    src/core/AgentRunStore.cpp
    src/core/PatchQueue.cpp
    src/core/ProjectHealth.cpp
    src/core/ModelCapabilities.cpp
    src/core/TokenStats.cpp
    src/core/ContextBudget.cpp
    src/core/ai/LlmProvider.cpp
    src/core/ai/SecretStore.cpp
    src/core/ai/AiMessage.cpp
    src/core/ai/AiToolBridge.cpp
    src/core/ai/ProviderPrefs.cpp
    src/core/ai/ProviderPricing.cpp
    src/core/ai/ProviderHealth.cpp
    src/core/ai/UsageLedger.cpp
    src/core/ai/TaskRouter.cpp
    src/core/ai/ModelPool.cpp
    src/core/ai/EmbedCache.cpp
    src/core/ai/EmbedBridge.cpp
    src/core/ai/AgentLlmAdapter.cpp
    src/core/ai/ProviderBench.cpp
    src/core/ai/AiRunner.cpp
    src/core/ai/LlmClient.cpp
    src/core/ai/providers/ProviderCodec.cpp
    src/core/ai/providers/SseParser.cpp
)
target_include_directories(test_stage46 PRIVATE src)
target_link_libraries(test_stage46 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Qml Qt6::Test ${HUNSPELL_LIBRARIES})
target_include_directories(test_stage46 PRIVATE ${HUNSPELL_INCLUDE_DIRS})
add_test(NAME stage46 COMMAND test_stage46)

add_executable(test_stage47
    tests/test_stage47.cpp
    src/core/A11yCheck.cpp
    src/core/ColorBlind.cpp
    src/core/LanguageManager.cpp
    src/core/ThemeTokens.cpp
)
target_include_directories(test_stage47 PRIVATE src)
target_link_libraries(test_stage47 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Qml Qt6::Test ${HUNSPELL_LIBRARIES})
target_include_directories(test_stage47 PRIVATE ${HUNSPELL_INCLUDE_DIRS})
add_test(NAME stage47 COMMAND test_stage47)

add_executable(test_stage48
    tests/test_stage48.cpp
    src/core/UpdateChecker.cpp
    src/core/StartupArgs.cpp
    src/core/CrashHandler.cpp
)
target_include_directories(test_stage48 PRIVATE src)
target_link_libraries(test_stage48 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Qml Qt6::Test ${HUNSPELL_LIBRARIES})
target_include_directories(test_stage48 PRIVATE ${HUNSPELL_INCLUDE_DIRS})
add_test(NAME stage48 COMMAND test_stage48)

add_executable(test_stage49
    tests/test_stage49.cpp
    src/core/CollabSession.cpp
    src/core/CollabMerge.cpp
)
target_include_directories(test_stage49 PRIVATE src)
target_link_libraries(test_stage49 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Qml Qt6::Test Qt6::WebSockets ${HUNSPELL_LIBRARIES})
target_include_directories(test_stage49 PRIVATE ${HUNSPELL_INCLUDE_DIRS})
add_test(NAME stage49 COMMAND test_stage49)

add_executable(test_stage50
    tests/test_stage50.cpp
)
target_include_directories(test_stage50 PRIVATE src)
target_link_libraries(test_stage50 PRIVATE Qt6::Core)
add_test(NAME stage50 COMMAND test_stage50)
