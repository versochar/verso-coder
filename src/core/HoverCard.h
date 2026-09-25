#pragma once
#include <QString>

// Stage 11: zengin hover kartı — LSP hover yanıtını başlıklı, renkli,
// kod bloklu zengin HTML'e çevirir (saf mantık, test edilebilir).
class HoverCard {
public:
    struct Colors {
        QString bg, border, title, text, code, codeBg, dim;
    };
    static QString render(const QString& title, const QString& body, const Colors& c);
    // ``` fences → <pre>, diğer satırlar kaçışlı <br> listesi
    static QString bodyToHtml(const QString& body, const Colors& c);
};
