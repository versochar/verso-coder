#pragma once
#include "SpellChecker.h"
#include <QList>
#include <QPair>
#include <QString>

struct SpellHit {
    int pos = 0;
    int len = 0;
    QString word;
    QStringList suggestions;
};

// Yorum ve string sabitlerindeki kelimeleri hunspell ile denetler.
// commentStringSpans: metindeki yorum/string aralıkları (pos,len) — test edilebilir.
class Spelling {
public:
    static QList<QPair<int, int>> commentStringSpans(const QString& text);
    static QList<SpellHit> checkText(const QString& text, SpellChecker& sc, int maxHits = 200);
};
