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
    src/core/AgentLoop.cpp
    src/core/PatchQueue.cpp
    src/core/ContextResolver.cpp
    src/core/ChatStore.cpp
    src/core/TokenStats.cpp
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
    src/widgets/PromptLibraryDialog.cpp
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
    src/core/AgentLoop.h
    src/core/PatchQueue.h
    src/core/ContextResolver.h
    src/core/ChatStore.h
    src/core/TokenStats.h
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
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE src ${HUNSPELL_INCLUDE_DIRS})
if(LIBSSH2_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${LIBSSH2_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBSSH2_LIBRARIES})
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Network Qt6::Concurrent Qt6::Svg Qt6::Qml Qt6::WebSockets ${HUNSPELL_LIBRARIES})

# --- Stage 19: birim testleri (saf mantık; widgets yok) ---
add_executable(test_stage19
    tests/test_stage19.cpp
    src/core/TaskChain.cpp
    src/core/PluginEngine.cpp
    src/core/CollabMerge.cpp
    src/core/CollabSession.cpp
    src/core/GitVersionManager.cpp
    src/core/CacheCleaner.cpp
    src/core/ThreadMonitor.cpp
    src/core/LanguageSupport.cpp
)
target_include_directories(test_stage19 PRIVATE src)
target_link_libraries(test_stage19 PRIVATE Qt6::Core Qt6::Network Qt6::Concurrent Qt6::Qml Qt6::WebSockets)
add_test(NAME stage19 COMMAND test_stage19)

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
install(FILES ${CMAKE_SOURCE_DIR}/packaging/verso-coder.appdata.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
