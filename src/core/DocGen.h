#pragma once
#include <QString>

// Stage 15: dokümantasyon yorumu — fonksiyon başı bulma + istem.
class DocGen {
public:
    // Verilen satırdan yukarı tarayarak fonksiyon başlangıcını bul (0-based, -1 yok)
    static int findFuncStart(const QString& text, int line0);
    // Dil uzantısına göre satır yorumu öneki ("//" ya da "#")
    static QString commentPrefix(const QString& suffix);
    static QString buildPrompt(const QString& code, const QString& lang);
};
