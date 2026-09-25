#pragma once
#include <QList>
#include <QMap>
#include <QString>

struct UiCommand {
    QString id;       // örn. "nav.quickOpen"
    QString title;    // görünen ad
    QString defKeys;  // varsayılan kısayol (boş olabilir)
};

// Komut paleti + kısayol ayarları için tek komut listesi.
inline QList<UiCommand> defaultCommands() {
    return {
        {"file.openFolder", "Klasör Aç", "Ctrl+O"},
        {"file.new", "Yeni Dosya", "Ctrl+N"},
        {"file.save", "Kaydet", "Ctrl+S"},
        {"file.saveAll", "Tümünü Kaydet", "Ctrl+Shift+S"},
        {"nav.quickOpen", "Hızlı Aç", "Ctrl+P"},
        {"nav.palette", "Komut Paleti", "Ctrl+Shift+P"},
        {"nav.search", "Projede Ara", "Ctrl+Shift+F"},
        {"nav.gotoLine", "Satıra Git", "Ctrl+G"},
        {"view.minimap", "Minimap Aç/Kapa", ""},
        {"view.terminal", "Terminal Aç/Kapa", "Ctrl+`"},
        {"run.build", "Derle & Çalıştır", "F5"},
        {"lsp.definition", "Tanıma Git", "F12"},
        {"lsp.hover", "Sembol Bilgisi", "Ctrl+K"},
        {"nav.splitRight", "Editörü Böl", "Ctrl+\\"},
        {"nav.moveTab", "Sekmeyi Diğer Gruba Taşı", ""},
        {"edit.moveUp", "Satırı Yukarı Taşı", "Alt+Up"},
        {"edit.moveDown", "Satırı Aşağı Taşı", "Alt+Down"},
        {"edit.duplicate", "Satırı Çoğalt", "Shift+Alt+Down"},
        {"edit.sort", "Satırları Sırala", "Ctrl+Alt+S"},
        {"edit.trim", "Sondaki Boşlukları Temizle", ""},
        {"edit.fold", "Katla", "Ctrl+Shift+BracketLeft"},
        {"edit.unfold", "Aç", "Ctrl+Shift+BracketRight"},
        {"edit.foldAll", "Tümünü Katla", ""},
        {"edit.unfoldAll", "Tümünü Aç", ""},
        {"edit.addNext", "Sonraki Eşleşmeyi Seç", "Ctrl+D"},
        {"edit.spell", "Yazımı Denetle", "F7"},
        {"large.loadFull", "Tamamını Yükle", ""},
        // Stage 6: proje & git derinliği
        {"proj.compare", "Klasör Karşılaştır", ""},
        {"session.saveSnapshot", "Anlık Görüntü Kaydet", ""},
        {"session.loadSnapshot", "Anlık Görüntü Yükle", ""},
        {"view.tasks", "Görevler (TODO) Paneli", ""},
        // Stage 7: AI ajan 2.0
        {"ai.agent", "AI Ajan Modu Aç/Kapa", ""},
        // Stage 8: platform & dağıtım
        {"help.about", "Sistem Teşhisi / Hakkında", "F1"},
        {"task.run", "Görevi Çalıştır (tasks.json)", ""},
        {"task.panel", "Görev Panelini Aç", ""},
        // Stage 9: görünüm
        {"view.theme", "Tema Galerisi", "Ctrl+K, Ctrl+T"},
        {"view.git", "Git Panelini Aç", ""},
        {"view.problems", "Sorunlar Panelini Aç", ""},
        {"file.settings", "Ayarlar", "Ctrl+,"},
        // Stage 10: mikro-etkileşim & cila
        {"view.zen", "Zen Modu (geçiş)", "F11"},
        {"view.zoomIn", "Yakınlaştır", "Ctrl+="},
        {"view.zoomOut", "Uzaklaştır", "Ctrl+-"},
        {"view.zoomReset", "Yakınlaştırmayı Sıfırla", "Ctrl+0"},
        {"ui.exportProfile", "Görünüm Profilini Dışa Aktar...", ""},
        {"ui.importProfile", "Görünüm Profili İçe Aktar...", ""},
        // Stage 11: editör görsel derinliği
        {"edit.find", "Bul (editör içi)", "Ctrl+F"},
        {"edit.findNext", "Sonraki Eşleşme", "F3"},
        {"edit.findPrev", "Önceki Eşleşme", "Shift+F3"},
        {"edit.expandSel", "Akıllı Seçimi Genişlet", "Shift+Alt+Right"},
        // Stage 12: uyarlanabilir arayüz
        {"view.themeEditor", "Tema Düzenleyici...", ""},
        {"view.cycleDensity", "Yoğunluk Değiştir (kompakt/rahat/geniş)", ""},
        {"view.focusMode", "Odak Modu (geçiş)", ""},
        {"view.titleBar", "Özel Başlık Çubuğu (geçiş)", ""},
        {"view.highContrast", "Yüksek Kontrast Teması", ""},
        {"ui.exportPng", "Pencereyi PNG Olarak Kaydet...", ""},
        {"ui.profiles", "Görünüm Profilleri", ""},
        {"ui.saveProfile", "Geçerli Görünümü Profil Olarak Kaydet...", ""},
        // Stage 13: dil zekâsı
        {"edit.complete", "Tamamlamayı Öner", "Ctrl+Space"},
        {"lsp.references", "Tüm Referanslar", "Shift+F12"},
        {"lsp.rename", "Sembolü Yeniden Adlandır", "F2"},
        {"lsp.codeAction", "Hızlı Düzeltme...", "Ctrl+."},
        {"lsp.signature", "İmza Yardımı", "Ctrl+Shift+Space"},
        {"lsp.docSymbols", "Belge Simgeleri (outline)", "Ctrl+Shift+O"},
        {"lsp.wsSymbols", "Çalışma Alanı Simgesi", "Ctrl+T"},
        {"format.document", "Belgeyi Biçimlendir", "Shift+Alt+F"},
        {"lsp.calls", "Çağrı Hiyerarşisi", ""},
        {"view.inlayHints", "Satır İçi İpuçları (aç/kapa)", ""},
        {"view.semantic", "Semantik Renklendirme (aç/kapa)", ""},
        // Stage 14: hata ayıklama & test
        {"debug.start", "Hata Ayıklamayı Başlat", "Ctrl+F5"},
        {"debug.stop", "Hata Ayıklamayı Durdur", "Shift+F5"},
        {"debug.continue", "Devam Et", "F6"},
        {"debug.stepOver", "Adım Üstü (next)", "F10"},
        {"debug.stepInto", "Adım İçi (step)", "Ctrl+F11"},
        {"debug.stepOut", "Bitir (finish)", "Shift+F11"},
        {"debug.toggleBp", "Kesme Noktası Aç/Kapa", "F9"},
        {"test.discover", "Testleri Keşfet", ""},
        {"test.runAll", "Tüm Testleri Çalıştır", ""},
        {"test.coverage", "Kapsama Çalıştır (gcov)", ""},
        {"debug.launch", "launch.json Düzenle", ""},
        // Stage 15: AI hattı
        {"ai.ghost", "Hayalet Tamamlama (aç/kapa)", ""},
        {"ai.inlineEdit", "AI ile Yeniden Yaz (seçim)...", ""},
        {"ai.fixSel", "AI ile Düzelt (seçim)", ""},
        {"ai.document", "Fonksiyona Belge Yorumu", ""},
        {"ai.genTest", "Test Üret...", ""},
        {"ai.commit", "AI Commit Mesajı", ""},
        {"ai.explain", "Sembolü Açıkla (balon)", ""},
        {"ai.prompts", "İstem Kitaplığı...", ""},
        {"ai.applyLast", "Son AI Kodunu Uygula", ""},
        {"ai.pullModel", "Ollama Modeli İndir...", ""},
        // Stage 16: uzaktan geliştirme
        {"remote.connect", "Uzağa Bağlan...", ""},
        {"remote.disconnect", "Uzak Bağlantıyı Kes", ""},
        {"remote.explorer", "Uzak Gezgin", ""},
        {"remote.open", "Uzak Dosya Aç...", ""},
        {"remote.terminal", "Uzak Terminal", ""},
        {"remote.lsp", "Uzak LSP Başlat", ""},
        {"remote.build", "Uzakta Derle...", ""},
        {"remote.git", "Uzak Git Durumu", ""},
        {"remote.debug", "Uzak Hedefte Hata Ayıkla...", ""},
        {"remote.forward", "Port Yönlendirme", ""},
        // Stage 17: editör deneyimi
        {"snippet.insert", "Snippet Ekle...", ""},
        {"snippet.new", "Yeni Snippet...", ""},
        {"edit.pasteAs", "Özel Yapıştır...", ""},
        {"edit.wordComplete", "Kelime Tamamla", ""},
        {"outline.show", "Outline Paneli", ""},
        {"fold.refreshLsp", "Katlama Aralıklarını Tazele (LSP)", ""},
        {"history.timeline", "Zaman Çizelgesi", ""},
        {"history.restorePrev", "Önceki Sürüme Dön", ""},
        {"search.exclude", "Değiştirmede Hariç Tut...", ""},
        {"git.hunk", "Hunk Menüsü (imleç satırı)", ""},
    };
}

// Ayarlardaki override yoksa varsayılanı döner.
inline QString effectiveKeys(const QMap<QString, QString>& overrides, const UiCommand& c) {
    return overrides.value(c.id, c.defKeys);
}
