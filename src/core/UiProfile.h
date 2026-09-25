#pragma once
#include <QString>
#include <QStringList>

class AppSettings;

// Stage 10: kişiselleştirme profili — kullanıcının tüm görünüm tercihlerini
// (tema, accent, tipografi, düzen...) tek JSON dosyasında dışa/içe aktarır.
// İçe aktarma yalnızca görsel alanlara dokunur; AI/LSP ayarları korunur.
class UiProfile {
public:
    static QString exportJson(const AppSettings& s);
    // json → s'in görsel alanları; geçersizse false döner ve s dokunulmaz kalır
    static bool applyJson(AppSettings& s, const QString& json);

    static QStringList visualKeys(); // profilin kapsadığı ayar anahtarları
    static const char* profileFormatVersion();
};
