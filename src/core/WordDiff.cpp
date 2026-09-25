#include "WordDiff.h"
#include <QRegularExpression>

static const QRegularExpression kWordRe("(\\w+)");

QStringList WordDiff::tokenize(const QString& line) {
    QStringList out;
    int pos = 0;
    auto it = kWordRe.globalMatch(line);
    while (it.hasNext()) {
        auto m = it.next();
        if (m.capturedStart() > pos)
            out << line.mid(pos, m.capturedStart() - pos); // ayraç
        out << m.captured(1);                              // kelime
        pos = m.capturedEnd();
    }
    if (pos < line.size()) out << line.mid(pos);
    return out;
}

// Basit LCS tablosu: token dizileri kısa olduğu için O(n*m) yeterli
static QList<bool> matchedMask(const QStringList& a, const QStringList& b) {
    const int n = a.size(), m = b.size();
    QList<QList<int>> dp(n + 1, QList<int>(m + 1, 0));
    for (int i = n - 1; i >= 0; --i)
        for (int j = m - 1; j >= 0; --j)
            dp[i][j] = (a[i] == b[j]) ? dp[i + 1][j + 1] + 1 : qMax(dp[i + 1][j], dp[i][j + 1]);
    QList<bool> mask(m, false);
    int i = 0, j = 0;
    while (i < n && j < m) {
        if (a[i] == b[j]) { mask[j] = true; ++i; ++j; }
        else if (dp[i + 1][j] >= dp[i][j + 1]) ++i;
        else ++j;
    }
    return mask;
}

QList<int> WordDiff::changedTokenIndexes(const QString& oldLine, const QString& newLine) {
    const QStringList a = tokenize(oldLine);
    const QStringList b = tokenize(newLine);
    const QList<bool> mask = matchedMask(a, b);
    QList<int> out;
    for (int j = 0; j < b.size(); ++j) {
        // yalnız gerçek "kelime" tokenları sayılır (harf/rakam içerenler)
        if (!mask[j] && kWordRe.match(b[j]).hasMatch())
            out << j;
    }
    return out;
}

QString WordDiff::highlightChanges(const QString& oldLine, const QString& newLine,
                                   const QString& bgHex, const QString& fgHex) {
    const QStringList b = tokenize(newLine);
    const QList<int> changed = changedTokenIndexes(oldLine, newLine);
    if (changed.isEmpty()) return newLine.toHtmlEscaped();

    QString html;
    for (int j = 0; j < b.size(); ++j) {
        const QString esc = b[j].toHtmlEscaped();
        if (changed.contains(j))
            html += QString("<span style=\"background:%1;color:%2;font-weight:bold\">%3</span>")
                        .arg(bgHex, fgHex, esc);
        else
            html += esc;
    }
    return html;
}
