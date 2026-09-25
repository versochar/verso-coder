#pragma once
#include <QList>
#include <QString>

// Stage 10: satır içi (kelime bazlı) diff vurgusu.
// Değişen diff satırlarında yalnızca farklı kelimeleri boyayan güvenli HTML üretir.
class WordDiff {
public:
    // newLine'u HTML olarak döndürür; oldLine'da karşılığı bulunmayan
    // kelimeler <span style="background:..."> ile sarılır.
    static QString highlightChanges(const QString& oldLine, const QString& newLine,
                                    const QString& bgHex, const QString& fgHex);

    // Saf mantık (test için): newLine'daki değişen kelime indeksleri
    static QList<int> changedTokenIndexes(const QString& oldLine, const QString& newLine);

    // Kelime tokenizasyonu: kelime karakterleri + geri kalanlar ayrı token
    static QStringList tokenize(const QString& line);
};
