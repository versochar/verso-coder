#include "ProblemMatcher.h"
#include <QRegularExpression>
#include <QSet>

QList<LspDiag> ProblemMatcher::matchGcc(const QString& output, const QString& source) {
    QList<LspDiag> out;
    // "dosya:12:34: error: mesaj" / "warning:" / "fatal error:" / "note:"
    static QRegularExpression re(
        R"(^([^:\n]+?):(\d+)(?::(\d+))?:\s*((?:fatal\s+)?error|warning|note):\s*(.+)$)");
    for (const QString& ln : output.split('\n')) {
        auto m = re.match(ln.trimmed());
        if (!m.hasMatch()) continue;
        LspDiag d;
        d.path = m.captured(1);
        d.line = qMax(0, m.captured(2).toInt() - 1);
        d.col = m.captured(3).isEmpty() ? 0 : qMax(0, m.captured(3).toInt() - 1);
        d.endLine = d.line;
        d.endCol = d.col + 1;
        const QString sev = m.captured(4);
        d.severity = sev.contains("error") ? 1 : (sev == "warning" ? 2 : 4);
        d.message = m.captured(5).trimmed().left(300);
        d.source = source;
        out << d;
    }
    return out;
}

QList<LspDiag> ProblemMatcher::matchPython(const QString& output) {
    QList<LspDiag> out;
    // '  File "/yol/x.py", line 12, in func' + son 'ValueError: mesaj'
    static QRegularExpression fre(R"RX(File\s+"([^"]+)",\s+line\s+(\d+))RX");
    static QRegularExpression ere(R"(^(\w*(?:Error|Exception|Warning)\w*):?\s*(.*)$)");
    QString lastFile;
    int lastLine = -1;
    QString lastErr;
    const QStringList lines = output.split('\n');
    for (const QString& ln : lines) {
        auto m = fre.match(ln);
        if (m.hasMatch()) {
            lastFile = m.captured(1);
            lastLine = m.captured(2).toInt() - 1;
            continue;
        }
        auto e = ere.match(ln.trimmed());
        if (e.hasMatch() && !ln.startsWith(" ") && !ln.startsWith("\t"))
            lastErr = (e.captured(1) + " " + e.captured(2)).trimmed().left(300);
    }
    if (!lastFile.isEmpty() && lastLine >= 0) {
        LspDiag d;
        d.path = lastFile;
        d.line = lastLine;
        d.endLine = lastLine;
        d.endCol = 1;
        d.severity = 1;
        d.message = lastErr.isEmpty() ? "Python hatası" : lastErr;
        d.source = "python";
        out << d;
    }
    return out;
}

QList<LspDiag> ProblemMatcher::matchGeneric(const QString& output, const QString& source) {
    QList<LspDiag> out;
    // "dosya:satır: mesaj" / "dosya(satır): mesaj" / "dosya:satır:sütun: mesaj"
    static QRegularExpression re(
        R"(^([^:\s][^:]*?):(\d+)(?::(\d+))?\s*[:\-]\s*(.+)$)");
    for (const QString& ln : output.split('\n')) {
        const QString t = ln.trimmed();
        if (t.size() > 300 || t.size() < 6) continue;
        // gcc/clang/python biçimleri zaten eşleşti — çifte sayma
        if (t.contains(": error") || t.contains(": warning") || t.contains("note:")
            || t.startsWith("File \"") || t.contains("Error:") || t.contains("Traceback"))
            continue;
        auto m = re.match(t);
        if (!m.hasMatch()) continue;
        LspDiag d;
        d.path = m.captured(1);
        d.line = qMax(0, m.captured(2).toInt() - 1);
        d.col = m.captured(3).isEmpty() ? 0 : qMax(0, m.captured(3).toInt() - 1);
        d.endLine = d.line;
        d.endCol = d.col + 1;
        d.severity = 1;
        d.message = m.captured(4).trimmed().left(300);
        d.source = source;
        out << d;
    }
    return out;
}

QList<LspDiag> ProblemMatcher::match(const QString& output, const QString& source) {
    QList<LspDiag> out = matchGcc(output, source);
    out << matchPython(output);
    // generic yalnızca gcc'nin yakalayamadıklarını ekler (tekilleştir)
    QSet<QString> seen;
    for (const LspDiag& d : out)
        seen << QString("%1:%2:%3").arg(d.path).arg(d.line).arg(d.message);
    for (const LspDiag& d : matchGeneric(output, source)) {
        const QString k = QString("%1:%2:%3").arg(d.path).arg(d.line).arg(d.message);
        if (!seen.contains(k)) { seen << k; out << d; }
    }
    return out;
}
