#include "MergeParse.h"

QList<MergeHunk> MergeParse::find(const QString& text) {
    QList<MergeHunk> out;
    const QStringList lines = text.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        if (!lines[i].startsWith("<<<<<<< ")) continue;
        MergeHunk h;
        h.startLine = i + 1;
        h.oursLabel = lines[i].mid(8).trimmed();
        int j = i + 1;
        // ours (+ taban ayrımı)
        for (; j < lines.size(); ++j) {
            if (lines[j].startsWith("||||||| ")) {
                // tabanı topla
                ++j;
                for (; j < lines.size(); ++j) {
                    if (lines[j].startsWith("=======")) break;
                    h.base << lines[j];
                }
                break;
            }
            if (lines[j].startsWith("=======")) break;
            h.ours << lines[j];
        }
        if (j >= lines.size()) break; // bozuk işaret
        ++j;
        for (; j < lines.size(); ++j) {
            if (lines[j].startsWith(">>>>>>> ")) {
                h.theirsLabel = lines[j].mid(8).trimmed();
                h.endLine = j + 1;
                break;
            }
            h.theirs << lines[j];
        }
        if (h.endLine <= 0) break;
        out << h;
        i = j;
    }
    return out;
}

QStringList MergeParse::resolve(const MergeHunk& h, int choice) {
    if (choice == 1) return h.theirs;
    if (choice == 2) return h.ours + h.theirs;
    return h.ours;
}

QString MergeParse::applyAll(const QString& text, const QList<int>& choices) {
    const QList<MergeHunk> hunks = find(text);
    if (hunks.isEmpty()) return text;
    const QStringList lines = text.split('\n');
    QStringList out;
    int cur = 0; // 0-based işlenen satır
    for (int hi = 0; hi < hunks.size(); ++hi) {
        const MergeHunk& h = hunks[hi];
        const int choice = (hi < choices.size()) ? choices[hi] : 0;
        while (cur < h.startLine - 1 && cur < lines.size()) out << lines[cur++];
        out << resolve(h, choice);
        cur = h.endLine; // >>>>>>> satırını atla (0-based karşılığı)
    }
    while (cur < lines.size()) out << lines[cur++];
    return out.join('\n');
}
