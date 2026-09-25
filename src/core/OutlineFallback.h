#pragma once
#include <QList>
#include <QString>

// Stage 17: LSP yokken outline yedeği — regex sınıf/fonksiyon taraması (saf).
struct OutlineItem {
    QString name;
    QString kind; // "class" | "function" | "method" | "var"
    int line0 = 0;
    int indent = 0;
};

class OutlineFallback {
public:
    // C++/Python/JS/Sh için girinti + imza desenleri
    static QList<OutlineItem> scan(const QString& text, const QString& suffix);
    // Girintiye göre ebeveyn zinciri ("A › metot")
    static QString container(const QList<OutlineItem>& items, int index);
};
