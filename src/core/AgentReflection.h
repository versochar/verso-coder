#pragma once
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

// Stage 34: ajan refleksiyonu — adım sonrası öz-değerlendirme.
// Model çağrısı yapmadan, gözlem metinlerinden saf puanlama yapar.
struct StepReflection {
    int step = 0;
    bool hadTool = false;
    bool hadError = false;
    bool wasDenied = false;
    int score = 0;    // 0..100
    QString verdict;  // good | retry | stalled
    QString advice;
    static QString verdictLabel(const QString& verdict);
};

class AgentReflection {
public:
    // Gözlem listesinden puan çıkar.
    static StepReflection reflect(const QString& assistantText, const QStringList& observations,
                                  int step = 1, int maxSteps = 5);
    // Araç başarı istatistiği: (araç adı, başarılı mı).
    struct ToolStat {
        QString name;
        int ok = 0;
        int fail = 0;
        int total() const { return ok + fail; }
        double rate() const { return total() == 0 ? 0.0 : double(ok) / double(total()); }
    };
    static QList<ToolStat> stats(const QList<QPair<QString, bool>>& results);
    // Düşük başarılı araçlar için ajan istemine eklenecek uyarı satırı.
    static QString penaltyLine(const QList<ToolStat>& stats, double minRate = 0.5);
    // Gözlem metninden araç adı/sonucu ayrıştır (AgentTools çıktısı biçiminde).
    static QPair<QString, bool> parseObservation(const QString& observation);
    // Belleğe yazılmaya değer mi?
    static bool shouldRemember(const StepReflection& r);
    static QString rememberText(const QString& goal, const StepReflection& r);
};
