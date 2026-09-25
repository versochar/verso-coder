#include "OutlineFallback.h"
#include <QRegularExpression>

static int indentOf(const QString& line) {
    int n = 0;
    for (QChar c : line) {
        if (c == ' ') ++n;
        else if (c == '\t') n += 4;
        else break;
    }
    return n;
}

QList<OutlineItem> OutlineFallback::scan(const QString& text, const QString& suffix) {
    QList<OutlineItem> out;
    const QString s = suffix.toLower();
    const QStringList lines = text.split('\n');
    static QRegularExpression classRe(
        R"(^\s*(?:class|struct)\s+(\w+))");
    static QRegularExpression funcRe(
        R"(^\s*(?:[\w:\<\>\*&]+\s+)+(\w+)\s*\([^;]*\)\s*(?:const\s*)?(?:\{|:)?\s*$)");
    static QRegularExpression pyDefRe(R"(^\s*(def|class)\s+(\w+))");
    static QRegularExpression jsFuncRe(
        R"(^\s*(?:function\s+(\w+)|(?:const|let|var)\s+(\w+)\s*=\s*(?:async\s*)?\(|(\w+)\s*\(.*\)\s*\{))");
    static QRegularExpression shFuncRe(R"(^\s*(?:function\s+)?(\w+)\s*\(\s*\)\s*(?:\{)?)");
    for (int i = 0; i < lines.size(); ++i) {
        const QString ln = lines[i];
        const QString t = ln.trimmed();
        if (t.isEmpty() || t.startsWith("//") || t.startsWith("#") || t.startsWith("*"))
            continue;
        OutlineItem it;
        it.line0 = i;
        it.indent = indentOf(ln);
        bool hit = false;
        if (s == "py") {
            auto m = pyDefRe.match(ln);
            if (m.hasMatch()) {
                it.kind = (m.captured(1) == "class") ? "class" : "function";
                it.name = m.captured(2);
                hit = true;
            }
        } else if (s == "js" || s == "ts") {
            auto m = jsFuncRe.match(ln);
            if (m.hasMatch()) {
                it.kind = "function";
                it.name = !m.captured(1).isEmpty() ? m.captured(1)
                    : (!m.captured(2).isEmpty() ? m.captured(2) : m.captured(3));
                hit = !it.name.isEmpty();
            }
        } else if (s == "sh") {
            auto m = shFuncRe.match(ln);
            if (m.hasMatch() && t.contains('(')) {
                it.kind = "function";
                it.name = m.captured(1);
                hit = true;
            }
        } else {
            auto mc = classRe.match(ln);
            if (mc.hasMatch()) {
                it.kind = "class";
                it.name = mc.captured(1);
                hit = true;
            } else {
                auto mf = funcRe.match(ln);
                if (mf.hasMatch() && !t.endsWith(';')) {
                    it.kind = (it.indent > 0) ? "method" : "function";
                    it.name = mf.captured(1);
                    hit = true;
                }
            }
        }
        if (hit) out << it;
    }
    return out;
}

QString OutlineFallback::container(const QList<OutlineItem>& items, int index) {
    if (index < 0 || index >= items.size()) return QString();
    QStringList chain;
    int indent = items[index].indent;
    for (int i = index - 1; i >= 0; --i) {
        if (items[i].indent < indent
            && (items[i].kind == "class" || items[i].kind == "function"
                || items[i].kind == "method")) {
            chain.prepend(items[i].name);
            indent = items[i].indent;
        }
    }
    return chain.join(" › ");
}
