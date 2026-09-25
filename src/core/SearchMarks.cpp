#include "SearchMarks.h"

QList<FindHit> SearchMarks::findAll(const QString& text, const QString& needle,
                                      bool caseSensitive, bool wholeWord, int maxHits) {
    QList<FindHit> out;
    if (needle.isEmpty() || text.size() > 2000000) return out;
    const Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    // Satır başlangıç ofsetleri (satır numarası için)
    QList<int> lineStarts;
    lineStarts << 0;
    for (int i = 0; i < text.size(); ++i)
        if (text[i] == '\n') lineStarts << (i + 1);
    int from = 0;
    int lineIdx = 0;
    while (out.size() < maxHits) {
        int idx = text.indexOf(needle, from, cs);
        if (idx < 0) break;
        if (wholeWord) {
            const bool leftOk = (idx == 0) || !isWordChar(text[idx - 1]);
            const int e = idx + needle.size();
            const bool rightOk = (e >= text.size()) || !isWordChar(text[e]);
            if (!leftOk || !rightOk) { from = idx + 1; continue; }
        }
        while (lineIdx + 1 < lineStarts.size() && lineStarts[lineIdx + 1] <= idx) ++lineIdx;
        out.append({idx, idx + needle.size(), lineIdx});
        from = idx + qMax(1, needle.size());
    }
    return out;
}
