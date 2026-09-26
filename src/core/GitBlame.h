#pragma once
#include <QDateTime>
#include <QList>
#include <QMap>
#include <QString>

// Stage 24: git blame ayrıştırma (`git blame --line-porcelain`).
// Saf mantık (test edilebilir); çalıştırma MainWindow/CodeEditor'da.
struct BlameLine {
    int line = 0; // 1-based
    QString author;
    qint64 authorTime = 0;
};

class GitBlame {
public:
    // porcelain çıktısı → satır listesi
    static QList<BlameLine> parse(const QString& porcelain);
    // Satırın yaşı (gün); bilinmiyorsa -1
    static int ageDays(const BlameLine& b, qint64 nowSecs);
    // Isı rengi: 0 (yeni) → 1 (eski); oran için maxAgeDays üst sınır
    static double heat(const BlameLine& b, qint64 nowSecs, int maxAgeDays = 365);
};
