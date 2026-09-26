#pragma once
#include <QList>
#include <QString>

// Stage 33: uzun sohbetleri özetleyerek bağlamı sıkıştırma.
struct ConvTurn {
    QString role; // "user" | "ai"
    QString text;
};

class ConversationSummarizer {
public:
    // Özet gerekli mi? (mevcut özet + turlar token eşiğini aşıyorsa)
    static bool needed(const QList<ConvTurn>& turns, const QString& existingSummary,
                       int maxTokens, int keepRecent = 6);

    // Özetlenecek (eski) turlar; son keepRecent tur hariç.
    static QList<ConvTurn> olderTurns(const QList<ConvTurn>& turns, int keepRecent);

    // Model için özet istemi.
    static QString summarizePrompt(const QList<ConvTurn>& turns, int maxChars = 8000);
    // Özet + son turları tek bağlam metninde birleştir.
    static QString merge(const QString& summary, const QList<ConvTurn>& recent,
                         int maxChars = 12000);
};
