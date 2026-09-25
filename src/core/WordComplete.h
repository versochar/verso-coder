#pragma once
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

// Stage 17: kelime tamamlaması (saf) — açık belgelerdeki tanımlayıcılar.
// LSP yokken/yedeği; sık + uzun + önek eşleşen önce.
struct WordCand {
    QString word;
    int freq = 0;
    int dist = 0; // imlece uzaklık (satır farkı)
};

class WordComplete {
public:
    // Metinden kelimeleri topla (en az minLen, anahtar sözcükler hariç)
    static QMap<QString, int> collect(const QString& text, int minLen = 4);
    static QSet<QString> keywords(const QString& suffix);
    // Adayları puanla: önek eşleşme, sıklık, uzunluk, yakınlık
    static QList<WordCand> suggest(const QString& prefix,
                                   const QList<QPair<QString, int>>& docs,
                                   const QString& suffix, int maxOut = 30);
    static bool isIdentChar(QChar c);
};
