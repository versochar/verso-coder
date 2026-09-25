#pragma once
#include <QString>
#include <QStringList>

// Stage 15: hayalet tamamlama — FIM istemi + yanıt temizliği + tetik kuralları.
class GhostCompletion {
public:
    // Önekteki son N satır + sonekteki ilk M satırı ile FIM istemi kur
    static QString buildPrompt(const QString& prefix, const QString& suffix,
                               const QString& lang);
    // Yanıtı tek hayalet öneriye indir (fence temizle, en fazla 3 satır)
    static QString clean(const QString& reply, const QString& prefixEnd);
    // İmleçten önceki karakter tetiklemeye uygun mu? (kelime içi, nokta, parantez...)
    static bool shouldTrigger(QChar before, QChar before2);
    // Hayalet metin hâlâ geçerli mi? (önek değişmediyse)
    static bool stillValid(const QString& currentPrefix, const QString& ghostPrefix);
};
