#include "AgentReflection.h"
#include <QHash>
#include <algorithm>

QString StepReflection::verdictLabel(const QString& verdict) {
    if (verdict == "good") return "iyi gidiyor";
    if (verdict == "retry") return "yeniden dene";
    if (verdict == "stalled") return "takıldı";
    return verdict;
}

QPair<QString, bool> AgentReflection::parseObservation(const QString& observation) {
    // Biçim: "[read_file] çıktı..."  → (ad, başarılı)
    QString s = observation.trimmed();
    if (!s.startsWith('[')) return {QString(), true};
    const int end = s.indexOf(']');
    if (end < 0) return {QString(), true};
    const QString name = s.mid(1, end - 1).trimmed();
    const QString rest = s.mid(end + 1);
    const bool ok = !rest.contains("HATA:") && !rest.contains("reddetti") &&
                    !rest.contains("devre dışı") && !rest.contains("kullanıcı yazmayı reddetti") &&
                    !rest.contains("bilinmeyen araç");
    return {name, ok};
}

StepReflection AgentReflection::reflect(const QString& assistantText,
                                        const QStringList& observations, int step, int maxSteps) {
    StepReflection r;
    r.step = step;
    r.hadTool = !observations.isEmpty();

    int okCount = 0, errCount = 0, denyCount = 0;
    for (const QString& o : observations) {
        if (o.contains("kullanıcı reddetti") || o.contains("reddetti")) ++denyCount;
        const auto parsed = parseObservation(o);
        if (parsed.first.isEmpty()) continue;
        if (parsed.second) ++okCount; else ++errCount;
    }
    r.hadError = errCount > 0;
    r.wasDenied = denyCount > 0;

    int score = 55;
    if (r.hadTool) score += 20;                  // gözlem toplandı
    if (okCount > 0) score += 15;
    if (errCount > 0) score -= 25 * qMin(errCount, 2);
    if (r.wasDenied) score -= 20;
    if (assistantText.trimmed().size() < 15) score -= 10; // boş/anlamsız yanıt
    if (maxSteps > 0 && step >= maxSteps) score -= 10;   // son adımdayız, tehlike
    r.score = qBound(0, score, 100);

    if (r.wasDenied) {
        r.verdict = "stalled";
        r.advice = "Kullanıcı bu işlemi reddetti; aynı yaklaşımı tekrarlama, alternatif üret.";
    } else if (r.hadError) {
        r.verdict = "retry";
        r.advice = "Gözlemlerde hata var; farklı bir yol ya da önce okuma yap.";
    } else if (r.hadTool) {
        r.verdict = "good";
        r.advice = "Araç çağrıları başarılı; bir sonraki adıma geç.";
    } else {
        r.verdict = "good";
        r.advice = "Araç çağrısı yok; nihai yanıt verildi.";
    }
    return r;
}

QList<AgentReflection::ToolStat> AgentReflection::stats(
    const QList<QPair<QString, bool>>& results) {
    QHash<QString, ToolStat> acc;
    for (const auto& p : results) {
        if (p.first.isEmpty()) continue;
        ToolStat& s = acc[p.first];
        s.name = p.first;
        if (p.second) ++s.ok; else ++s.fail;
    }
    QList<ToolStat> out = acc.values();
    std::sort(out.begin(), out.end(), [](const ToolStat& a, const ToolStat& b) {
        if (a.total() != b.total()) return a.total() > b.total();
        return a.name < b.name;
    });
    return out;
}

QString AgentReflection::penaltyLine(const QList<ToolStat>& stats, double minRate) {
    QStringList low;
    for (const ToolStat& s : stats) {
        if (s.total() < 2) continue;         // az örneklemle karar verme
        if (s.rate() < minRate) low << QString("%1 (%2%%)").arg(s.name).arg(int(s.rate() * 100));
    }
    if (low.isEmpty()) return {};
    return QString("[UYARI] Şu araçlar sürekli başarısız: %1. Bunları tekrar tekrar denemek yerine "
                   "başka bir yol seç.")
        .arg(low.join(", "));
}

bool AgentReflection::shouldRemember(const StepReflection& r) {
    // Yalnız kalıcı işe yarayan dersler: tekrar eden hata veya reddedilme.
    return r.hadError || r.wasDenied;
}

QString AgentReflection::rememberText(const QString& goal, const StepReflection& r) {
    const QString g = goal.trimmed().left(120);
    if (r.wasDenied)
        return QString("Bu görevde (%1) kullanıcı benzer bir işlemi reddetti; doğrulamadan yazma.")
            .arg(g);
    return QString("Bu görevde (%1) şu adım hata verdi: %2").arg(g, r.advice);
}
