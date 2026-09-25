#include "LanguageManager.h"

LanguageManager& LanguageManager::instance() {
    static LanguageManager m;
    return m;
}

LanguageManager::LanguageManager() {
    // key -> { tr, en }
    auto add = [&](const QString& k, const QString& tr, const QString& en) {
        m_dict[k] = {{"tr", tr}, {"en", en}};
    };
    add("explorer", "GEZGİN", "EXPLORER");
    add("source_control", "KAYNAK DENETİMİ", "SOURCE CONTROL");
    add("ai_assistant", "AI ASİSTAN", "AI ASSISTANT");
    add("files", "Dosyalar", "Files");
    add("open_folder", "Klasör Aç", "Open Folder");
    add("save", "Kaydet", "Save");
    add("save_all", "Tümünü Kaydet", "Save All");
    add("commit", "Commit", "Commit");
    add("push", "Push", "Push");
    add("pull", "Pull", "Pull");
    add("refresh", "Yenile", "Refresh");
    add("stage_all", "Tümünü Stage'le", "Stage All");
    add("changes", "Değişiklikler", "Changes");
    add("message", "Commit mesajı...", "Commit message...");
    add("ask_ai", "AI'a sor...", "Ask AI...");
    add("send", "Gönder", "Send");
    add("model", "Model", "Model");
    add("settings", "Ayarlar", "Settings");
    add("general", "Genel", "General");
    add("ai_connection", "AI Bağlantısı", "AI Connection");
    add("language", "Dil", "Language");
    add("theme", "Tema", "Theme");
    add("host", "Ollama Adresi", "Ollama Host");
    add("context_window", "Context Window (num_ctx)", "Context Window (num_ctx)");
    add("temperature", "Temperature", "Temperature");
    add("cpu_threads", "CPU Threads (num_thread)", "CPU Threads (num_thread)");
    add("gpu_offload", "GPU Offload (num_gpu)", "GPU Offload (num_gpu)");
    add("gpu_backend", "GPU Backend", "GPU Backend");
    add("system_prompt", "Sistem Promptu", "System Prompt");
    add("test", "Bağlantıyı Test Et", "Test Connection");
    add("no_repo", "Git deposu bulunamadı.", "No git repository found.");
    add("statusbar_ready", "Hazır", "Ready");
}

void LanguageManager::setLanguage(const QString& lang) {
    if (lang == "tr" || lang == "en") m_lang = lang;
}

QString LanguageManager::t(const QString& key) const {
    auto it = m_dict.find(key);
    if (it == m_dict.end()) return key;
    return it->value(m_lang, key);
}
