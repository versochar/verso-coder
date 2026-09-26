#pragma once
#include <QString>

// Stage 33: yanıt kalite puanı — sezgisel + kullanıcı oyu.
struct ResponseScore {
    int score = 0;      // 0..100 sezgisel
    int userRating = 0; // 0..5 (0 = oy yok)
    bool hasCode = false;
    bool hasFileRef = false;
    bool hasError = false;
    bool tooShort = false;
};

class ResponseScorer {
public:
    static ResponseScore score(const QString& text);
    // Sezgisel puan ile kullanıcı oyunu birleştir (oy yoksa sezgisel kalır).
    static int combine(int heuristic, int userRating, int userWeight = 40);
    static QString label(int score); // "zayıf" | "orta" | "iyi" | "çok iyi"
};
