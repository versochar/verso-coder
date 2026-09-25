#include "DiffGutter.h"
#include <QRegularExpression>

QMap<int, char> DiffGutter::changedLines(const QString& diffText) {
    QMap<int, char> out;
    static const QRegularExpression hunkRe("^@@ -\\d+(?:,\\d+)? \\+(\\d+)(?:,(\\d+))? @@");
    int newLine = 0;      // sıradaki yeni-dosya satırı (1-based)
    int pendingDel = 0;   // aynı konumdaki silme sayısı (değişim tespiti için)
    int pendingAt = 0;    // silmelerin ait olduğu satır
    bool inHunk = false;
    auto flushDel = [&]() {
        if (pendingDel > 0 && !out.contains(pendingAt))
            out[pendingAt] = 'd';
        pendingDel = 0;
    };
    const QStringList lines = diffText.split('\n');
    for (const QString& ln : lines) {
        if (ln.startsWith("@@")) {
            flushDel();
            auto m = hunkRe.match(ln);
            if (!m.hasMatch()) { inHunk = false; continue; }
            newLine = m.captured(1).toInt();
            inHunk = true;
            continue;
        }
        if (!inHunk || ln.isEmpty()) continue;
        const QChar k = ln[0];
        if (k == ' ') { flushDel(); ++newLine; }
        else if (k == '-') {
            if (pendingDel == 0) pendingAt = newLine;
            ++pendingDel;
        } else if (k == '+') {
            if (pendingDel > 0) {
                // silme+ekleme aynı konumda → değişen satır
                out[newLine] = 'm';
                --pendingDel;
                if (pendingDel == 0) pendingAt = 0;
            } else {
                out[newLine] = 'a';
            }
            ++newLine;
        } else if (k == '\\') {
            // "\ No newline at end of file" — yok say
        } else {
            flushDel(); // yeni dosya başlığı vb.
        }
    }
    flushDel();
    return out;
}
