#include "ResponseScorer.h"
#include <QRegularExpression>

ResponseScore ResponseScorer::score(const QString& text) {
    ResponseScore s;
    const QString t = text.trimmed();
    if (t.isEmpty()) return s;
    s.hasCode = t.contains("```") || t.contains("\n    ") || t.contains("\n\t");
    s.hasFileRef = QRegularExpression("\\b[\\w./-]+\\.(cpp|h|hpp|c|py|js|ts|qml|json|md|cmake)\\b")
                       .match(t)
                       .hasMatch();
    s.hasError = t.contains("hata", Qt::CaseInsensitive) ||
                 t.contains("error", Qt::CaseInsensitive) ||
                 t.contains("başarısız", Qt::CaseInsensitive);
    s.tooShort = t.size() < 40;

    int sc = 30;                      // taban
    if (s.hasCode) sc += 25;
    if (s.hasFileRef) sc += 15;
    if (t.size() >= 200) sc += 15;
    else if (t.size() >= 80) sc += 8;
    if (s.tooShort) sc -= 15;
    if (s.hasError) sc -= 5;          // hata bildirimi bilgilendirici ama "iyi" sayılmaz
    if (t.endsWith('?') || t.contains("emin değilim", Qt::CaseInsensitive)) sc -= 10;
    s.score = qBound(0, sc, 100);
    return s;
}

int ResponseScorer::combine(int heuristic, int userRating, int userWeight) {
    if (userRating <= 0) return qBound(0, heuristic, 100);
    const int user = qBound(0, userRating, 5) * 20; // 0..100
    const int w = qBound(0, userWeight, 100);
    return qBound(0, (heuristic * (100 - w) + user * w) / 100, 100);
}

QString ResponseScorer::label(int score) {
    if (score >= 80) return "çok iyi";
    if (score >= 60) return "iyi";
    if (score >= 40) return "orta";
    return "zayıf";
}
