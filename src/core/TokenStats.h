#pragma once
#include <QString>
#include <QtGlobal>

// Oturum boyunca toplanan token kullanımı + yaklaşık maliyet.
struct TokenStats {
    qint64 promptTokens = 0;
    qint64 evalTokens = 0;
    int calls = 0;

    void add(int prompt, int eval, int callCount = 1) {
        promptTokens += prompt;
        evalTokens += eval;
        calls += callCount;
    }
    qint64 total() const { return promptTokens + evalTokens; }

    // USD / 1M token (yerel Ollama'da 0; bilinmeyen modeller 0).
    static double priceIn(const QString& model);
    static double priceOut(const QString& model);
    double estimateCostUsd(const QString& model) const;
    QString summary(const QString& model = QString()) const;
};
