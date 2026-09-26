#include "CodeMetrics.h"
#include <QRegularExpression>

CodeMetrics CodeMetrics::analyze(const QString& filePath, const QString& text,
                                 int functionCount) {
    CodeMetrics m;
    const QStringList lines = text.split('\n');
    m.lines = lines.size();
    m.functions = functionCount;
    static QRegularExpression comment(R"(^\s*(//|#|\*|/\*))");
    static QRegularExpression branch(
        R"(\b(if|for|while|switch|catch|case)\b|&&|\|\||\?)");
    for (const QString& ln : lines) {
        const QString t = ln.trimmed();
        if (t.isEmpty() || comment.match(t).hasMatch()) continue;
        ++m.codeLines;
        auto it = branch.globalMatch(t);
        while (it.hasNext()) {
            it.next();
            ++m.branches;
        }
    }
    return m;
}
