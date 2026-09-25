#pragma once
#include <QString>

// Stage 17: otomatik çift kapatma + akıllı Enter (saf karar mantığı).
// CodeEditor tuş olayları sorar, uygular.
class AutoPairs {
public:
    struct KeyResult {
        bool handled = false; // true: varsayılan işlem yerine bunu uygula
        QString insert;       // eklenecek metin
        int cursorBack = 0;   // imleç kaç karakter geri alınır
    };
    // Kapanış adayı: açılış + bağlam (dize/yorum içinde mi?) → sonuç
    static KeyResult onOpen(QChar open, QChar close, const QString& lineBefore,
                            const QString& lineAfter, bool inString, bool inComment,
                            bool enabled);
    // Kapanış karakteri yazılıp eşleşen zaten varsa: üstünden atla mı?
    static bool shouldSkip(QChar close, const QString& lineAfter);
    // Seçim sarma: "(seçim)" — seçim yoksa çift ekleyip ortaya gir
    static KeyResult wrapSelection(QChar open, QChar close, bool hasSelection);
    // Enter: kapanış parantezinden önceyse araya girintili satır aç
    static QString enterIndent(const QString& lineBefore, const QString& lineAfter,
                               const QString& baseIndent, bool useSpaces, int tabWidth);
    static bool isOpener(QChar c);
    static QChar matching(QChar c);
};
