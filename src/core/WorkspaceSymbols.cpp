#include "WorkspaceSymbols.h"
#include <QFileInfo>
#include <QRegularExpression>

QList<WorkspaceSymbol> WorkspaceSymbols::scanFile(const QString& filePath,
                                                  const QString& text) {
    QList<WorkspaceSymbol> out;
    const QString suf = QFileInfo(filePath).suffix().toLower();
    QList<QPair<QString, QRegularExpression>> pats;
    if (suf == "py") {
        pats.append({"func", QRegularExpression(R"(^\s*def\s+([A-Za-z_]\w*)\s*\()")});
        pats.append({"class", QRegularExpression(R"(^\s*class\s+([A-Za-z_]\w*))")});
    } else if (suf == "js" || suf == "ts" || suf == "jsx" || suf == "tsx") {
        pats.append({"func", QRegularExpression(R"(^\s*(?:async\s+)?function\s+([A-Za-z_$][\w$]*))")});
        pats.append({"func", QRegularExpression(R"(^\s*(?:const|let|var)\s+([A-Za-z_$][\w$]*)\s*=\s*(?:async\s*)?\()")});
        pats.append({"func", QRegularExpression(R"(^\s*(?:async\s+)?([A-Za-z_$][\w$]*)\s*\([^)]*\)\s*\{)")});
        pats.append({"class", QRegularExpression(R"(^\s*(?:export\s+default\s+|export\s+)?class\s+([A-Za-z_$][\w$]*))")});
    } else {
        // C/C++/Java/Go/Rust/C# — satır başı tanım sezgisi
        pats.append({"func",
                 QRegularExpression(
                     R"(^\s*(?:[\w:<>,*&]+\s+)+([A-Za-z_]\w*)\s*\([^;]*\)\s*(?:const\s*)?\{?)")});
        pats.append({"class",
                 QRegularExpression(R"(^\s*(?:class|struct|enum(?:\s+class)?|interface)\s+([A-Za-z_]\w*))")});
    }
    const QStringList lines = text.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        const QString ln = lines[i];
        if (ln.trimmed().startsWith("//") || ln.trimmed().startsWith('#') ||
            ln.trimmed().startsWith('*'))
            continue;
        for (const auto& p : pats) {
            auto m = p.second.match(ln);
            if (m.hasMatch() && !m.captured(1).isEmpty()) {
                WorkspaceSymbol s;
                s.name = m.captured(1);
                s.kind = p.first;
                s.file = filePath;
                s.line = i + 1;
                out << s;
                break;
            }
        }
    }
    return out;
}

QList<WorkspaceSymbol> WorkspaceSymbols::scanFiles(const QMap<QString, QString>& pathToText) {
    QList<WorkspaceSymbol> out;
    for (auto it = pathToText.constBegin(); it != pathToText.constEnd(); ++it)
        out << scanFile(it.key(), it.value());
    return out;
}

QList<WorkspaceSymbol> WorkspaceSymbols::query(const QList<WorkspaceSymbol>& all,
                                               const QString& pattern, int max) {
    QList<WorkspaceSymbol> out;
    const QString p = pattern.trimmed().toLower();
    if (p.isEmpty()) return out;
    for (const WorkspaceSymbol& s : all) {
        if (s.name.toLower().contains(p)) {
            out << s;
            if (out.size() >= max) break;
        }
    }
    return out;
}

int WorkspaceSymbols::refCount(const QString& text, const QString& name, int defLine) {
    QRegularExpression re(QString(R"(\b%1\b)").arg(QRegularExpression::escape(name)));
    int n = 0;
    const QStringList lines = text.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        if (i + 1 == defLine) continue;
        auto it = re.globalMatch(lines[i]);
        while (it.hasNext()) {
            it.next();
            ++n;
        }
    }
    return n;
}
