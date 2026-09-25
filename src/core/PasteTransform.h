#pragma once
#include <QString>
#include <QStringList>

// Stage 17: "özel yapıştır" dönüşümleri (saf) — pano metnini yerleştirmeden dönüştür.
class PasteTransform {
public:
    enum class Kind {
        Plain,       // düz
        JoinLines,   // satırları tek satırda birleştir
        JoinComma,   // "a", "b" biçimi
        Unique,      // yinelenen satırları at
        Sort,        // satırları sırala
        SortUnique,  // sırala + tekilleştir
        Upper,       // BÜYÜK HARF
        Lower,       // küçük harf
        Quote,       // her satırı "..." içine al
        Trim,        // satır başı/son boşlukları kırp
    };
    static QString apply(Kind k, const QString& text);
    static QString title(Kind k);
    static QList<Kind> all();
};
