#pragma once
#include <QColor>
#include <QList>
#include <QString>

// Stage 11: gökkuşağı parantez derinlik analizi (saf mantık).
// Dize/içi yorum ve satır yorumlarındaki parantezler yok sayılır.
struct BracketMark {
    int pos = 0;    // metindeki konum
    int depth = 0;  // çiftin iç içelik seviyesi (en dış = 0)
    bool open = true;
};

class BracketDepth {
public:
    static QList<BracketMark> marks(const QString& text, int maxChars = 300000);
    // 6 renkli gökkuşağı paleti (koyu/açık tema varyantı)
    static QList<QColor> palette(bool dark);
    static QColor colorFor(int depth, bool dark);
};
