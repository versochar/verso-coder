#include "SettingsManager.h"
#include <QDir>
#include <QSettings>

SettingsManager& SettingsManager::instance() {
    static SettingsManager m;
    return m;
}

AppSettings SettingsManager::load() const {
    QSettings q("Verso", "VersoCoder");
    AppSettings s;
    s.language      = q.value("language", "tr").toString();
    s.theme         = q.value("theme", "dark").toString();
    s.ollamaHost    = q.value("ollama/host", "http://localhost:11434").toString();
    s.ollamaModel   = q.value("ollama/model", "llama3.1").toString();
    s.contextWindow = q.value("ollama/num_ctx", 4096).toInt();
    s.temperature   = q.value("ollama/temperature", 0.7).toDouble();
    s.cpuThreads    = q.value("ollama/num_thread", 4).toInt();
    s.gpuLayers     = q.value("ollama/num_gpu", 999).toInt();
    s.gpuBackend    = q.value("ollama/backend", "CUDA").toString();
    s.systemPrompt  = q.value("ollama/system",
        "You are a helpful coding assistant. Answer concisely with code examples.").toString();
    s.fontSize      = q.value("editor/fontSize", 11).toInt();
    s.tabWidth      = q.value("editor/tabWidth", 4).toInt();
    s.autoSave      = q.value("editor/autoSave", true).toBool();
    s.autoSaveIntervalMs = q.value("editor/autoSaveMs", 2000).toInt();
    s.restoreSession = q.value("editor/restoreSession", true).toBool();
    s.lastRoot      = q.value("editor/lastRoot", QDir::homePath()).toString();
    s.sessionFiles  = q.value("session/files").toStringList();
    s.sessionActive = q.value("session/active", 0).toInt();
    s.sessionCursors = q.value("session/cursors").toStringList();
    s.largeFileMb   = q.value("editor/largeFileMb", 2).toInt();
    s.lspEnabled    = q.value("editor/lspEnabled", true).toBool();
    s.gdbPath       = q.value("editor/gdbPath", "gdb").toString();
    s.terminalShell = q.value("editor/terminalShell", "bash").toString();
    q.beginGroup("shortcuts");
    for (const QString& k : q.childKeys()) s.shortcuts[k] = q.value(k).toString();
    q.endGroup();
    s.useEditorConfig = q.value("editor/useEditorConfig", true).toBool();
    s.spellLang     = q.value("editor/spellLang", "auto").toString();
    s.sessionFiles2  = q.value("session/files2").toStringList();
    s.sessionActive2 = q.value("session/active2", 0).toInt();
    s.sessionCursors2 = q.value("session/cursors2").toStringList();
    s.sessionFolds   = q.value("session/folds").toStringList();
    s.aiStreaming   = q.value("ai/streaming", true).toBool();
    s.aiGhost       = q.value("ai/ghost", false).toBool();
    s.contextMode   = q.value("ai/contextMode", "file").toString();
    s.activeProfile = q.value("ai/activeProfile", "default").toString();
    s.profilesJson  = q.value("ai/profiles", "[]").toString();
    s.version           = q.value("app/version", "1.0.0").toString();
    s.m_firstRun        = q.value("app/firstRun", true).toBool();
    // Stage 9: görünüm
    s.accentColor    = q.value("ui/accent", "").toString();
    s.uiFontFamily   = q.value("ui/fontFamily", "").toString();
    s.uiFontSize     = q.value("ui/fontSize", 13).toInt();
    s.lineHeight     = q.value("ui/lineHeight", 1.0).toDouble();
    s.letterSpacing  = q.value("ui/letterSpacing", 0.0).toDouble();
    s.ligatures      = q.value("ui/ligatures", true).toBool();
    s.cursorWidth    = q.value("ui/cursorWidth", 2).toInt();
    s.lineHighlightOn = q.value("ui/lineHighlight", true).toBool();
    // Stage 10
    s.reducedMotion  = q.value("ui/reducedMotion", false).toBool();
    s.layoutPreset   = q.value("ui/layoutPreset", "standart").toString();
    // Stage 11
    s.showWhitespace = q.value("editor/showWhitespace", false).toBool();
    s.rulerColumn    = q.value("editor/rulerColumn", 0).toInt();
    s.stickyScroll   = q.value("editor/stickyScroll", true).toBool();
    // Stage 12
    s.density        = q.value("ui/density", "comfortable").toString();
    s.autoThemeMode  = q.value("ui/autoThemeMode", "off").toString();
    s.dayTheme       = q.value("ui/dayTheme", "light").toString();
    s.nightTheme     = q.value("ui/nightTheme", "dark").toString();
    s.colorVision    = q.value("ui/colorVision", "none").toString();
    s.focusMode      = q.value("editor/focusMode", false).toBool();
    s.customTitleBar = q.value("ui/customTitleBar", false).toBool();
    s.chipGit        = q.value("ui/chipGit", true).toBool();
    s.chipProblems   = q.value("ui/chipProblems", true).toBool();
    s.chipCursor     = q.value("ui/chipCursor", true).toBool();
    s.chipLang       = q.value("ui/chipLang", true).toBool();
    s.chipEol        = q.value("ui/chipEol", true).toBool();
    s.chipEnc        = q.value("ui/chipEnc", true).toBool();
    s.sidePages      = q.value("ui/sidePages",
                               QStringList({"explorer", "search", "git", "ai", "problems"}))
                           .toStringList();
    s.bottomOrder    = q.value("ui/bottomOrder").toStringList();
    s.activeUiProfile = q.value("ui/activeUiProfile").toString();
    // Stage 13
    s.formatOnSave      = q.value("editor/formatOnSave", false).toBool();
    s.inlayHints        = q.value("editor/inlayHints", false).toBool();
    s.semanticHighlight = q.value("editor/semanticHighlight", true).toBool();
    s.autoComplete      = q.value("editor/autoComplete", true).toBool();
    s.autoClose         = q.value("editor/autoClose", true).toBool();
    s.historyKeep       = q.value("history/keep", 50).toInt();
    return s;
}

void SettingsManager::save(const AppSettings& s) const {
    QSettings q("Verso", "VersoCoder");
    q.setValue("language", s.language);
    q.setValue("theme", s.theme);
    q.setValue("ollama/host", s.ollamaHost);
    q.setValue("ollama/model", s.ollamaModel);
    q.setValue("ollama/num_ctx", s.contextWindow);
    q.setValue("ollama/temperature", s.temperature);
    q.setValue("ollama/num_thread", s.cpuThreads);
    q.setValue("ollama/num_gpu", s.gpuLayers);
    q.setValue("ollama/backend", s.gpuBackend);
    q.setValue("ollama/system", s.systemPrompt);
    q.setValue("editor/fontSize", s.fontSize);
    q.setValue("editor/tabWidth", s.tabWidth);
    q.setValue("editor/autoSave", s.autoSave);
    q.setValue("editor/autoSaveMs", s.autoSaveIntervalMs);
    q.setValue("editor/restoreSession", s.restoreSession);
    q.setValue("editor/lastRoot", s.lastRoot);
    q.setValue("session/files", s.sessionFiles);
    q.setValue("session/active", s.sessionActive);
    q.setValue("session/cursors", s.sessionCursors);
    q.setValue("editor/largeFileMb", s.largeFileMb);
    q.setValue("editor/lspEnabled", s.lspEnabled);
    q.setValue("editor/gdbPath", s.gdbPath);
    q.setValue("editor/terminalShell", s.terminalShell);
    q.beginGroup("shortcuts");
    for (auto it = s.shortcuts.begin(); it != s.shortcuts.end(); ++it) q.setValue(it.key(), it.value());
    q.endGroup();
    q.setValue("editor/useEditorConfig", s.useEditorConfig);
    q.setValue("editor/spellLang", s.spellLang);
    q.setValue("session/files2", s.sessionFiles2);
    q.setValue("session/active2", s.sessionActive2);
    q.setValue("session/cursors2", s.sessionCursors2);
    q.setValue("session/folds", s.sessionFolds);
    q.setValue("ai/streaming", s.aiStreaming);
    q.setValue("ai/ghost", s.aiGhost);
    q.setValue("ai/contextMode", s.contextMode);
    q.setValue("ai/activeProfile", s.activeProfile);
    q.setValue("ai/profiles", s.profilesJson);
    // Stage 9: görünüm
    q.setValue("ui/accent", s.accentColor);
    q.setValue("ui/fontFamily", s.uiFontFamily);
    q.setValue("ui/fontSize", s.uiFontSize);
    q.setValue("ui/lineHeight", s.lineHeight);
    q.setValue("ui/letterSpacing", s.letterSpacing);
    q.setValue("ui/ligatures", s.ligatures);
    q.setValue("ui/cursorWidth", s.cursorWidth);
    q.setValue("ui/lineHighlight", s.lineHighlightOn);
    // Stage 10
    q.setValue("ui/reducedMotion", s.reducedMotion);
    q.setValue("ui/layoutPreset", s.layoutPreset);
    // Stage 11
    q.setValue("editor/showWhitespace", s.showWhitespace);
    q.setValue("editor/rulerColumn", s.rulerColumn);
    q.setValue("editor/stickyScroll", s.stickyScroll);
    // Stage 12
    q.setValue("ui/density", s.density);
    q.setValue("ui/autoThemeMode", s.autoThemeMode);
    q.setValue("ui/dayTheme", s.dayTheme);
    q.setValue("ui/nightTheme", s.nightTheme);
    q.setValue("ui/colorVision", s.colorVision);
    q.setValue("editor/focusMode", s.focusMode);
    q.setValue("ui/customTitleBar", s.customTitleBar);
    q.setValue("ui/chipGit", s.chipGit);
    q.setValue("ui/chipProblems", s.chipProblems);
    q.setValue("ui/chipCursor", s.chipCursor);
    q.setValue("ui/chipLang", s.chipLang);
    q.setValue("ui/chipEol", s.chipEol);
    q.setValue("ui/chipEnc", s.chipEnc);
    q.setValue("ui/sidePages", s.sidePages);
    q.setValue("ui/bottomOrder", s.bottomOrder);
    q.setValue("ui/activeUiProfile", s.activeUiProfile);
    // Stage 13
    q.setValue("editor/formatOnSave", s.formatOnSave);
    q.setValue("editor/inlayHints", s.inlayHints);
    q.setValue("editor/semanticHighlight", s.semanticHighlight);
    q.setValue("editor/autoComplete", s.autoComplete);
    q.setValue("editor/autoClose", s.autoClose);
    q.setValue("history/keep", s.historyKeep);
    q.setValue("app/version", s.version);
    q.setValue("app/firstRun", s.m_firstRun);
}
