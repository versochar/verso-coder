#pragma once
#include <QList>
#include <QString>

// Stage 30: 3-yönlü birleştirme ayrıştırma (`<<<<<<<`/`|||||||`/`=======`/`>>>>>>>`).
// Saf mantık (test edilebilir).
struct MergeHunk {
    int startLine = 0; // 1-based (<<<<<<< satırı)
    int endLine = 0;   // 1-based (>>>>>>> satırı)
    QStringList ours;  // ======= öncesi (||||||| varsa sonrası taban)
    QStringList base;  // ||||||| ile ======= arası (yoksa boş)
    QStringList theirs;// ======= sonrası
    QString oursLabel;
    QString theirsLabel;
};

class MergeParse {
public:
    static QList<MergeHunk> find(const QString& text);
    // Çözüm: 0=bizimki, 1=onlarınki, 2=her ikisi (bizimki üstte)
    static QStringList resolve(const MergeHunk& h, int choice);
    // Tüm dosyayı seçimlerle çöz (choices: hunk başına 0/1/2)
    static QString applyAll(const QString& text, const QList<int>& choices);
};
