#include "BracketDepth.h"

QList<QColor> BracketDepth::palette(bool dark) {
    if (dark)
        return {QColor("#ffd700"), QColor("#da70d6"), QColor("#179fff"),
                QColor("#7ee787"), QColor("#ffa657"), QColor("#56d4dd")};
    return {QColor("#9a6a00"), QColor("#8a2f8a"), QColor("#0451a5"),
            QColor("#116329"), QColor("#a34a00"), QColor("#0e6e73")};
}

QColor BracketDepth::colorFor(int depth, bool dark) {
    const QList<QColor> p = palette(dark);
    return p[qAbs(depth) % p.size()];
}

QList<BracketMark> BracketDepth::marks(const QString& text, int maxChars) {
    QList<BracketMark> out;
    const int n = qMin(text.size(), maxChars);
    struct Open { QChar ch; int pos; };
    QList<Open> stack;
    auto mateOf = [](QChar c) -> QChar {
        if (c == '(') return ')';
        if (c == '[') return ']';
        if (c == '{') return '}';
        return QChar();
    };
    auto openOf = [](QChar c) -> QChar {
        if (c == ')') return '(';
        if (c == ']') return '[';
        if (c == '}') return '{';
        return QChar();
    };
    QChar strQuote;      // 0 = dize dışında
    bool lineComment = false;
    bool blockComment = false;
    bool escaped = false;
    for (int i = 0; i < n; ++i) {
        const QChar c = text[i];
        const QChar nx = (i + 1 < n) ? text[i + 1] : QChar();
        if (lineComment) {
            if (c == '\n') lineComment = false;
            continue;
        }
        if (blockComment) {
            if (c == '*' && nx == '/') { blockComment = false; ++i; }
            continue;
        }
        if (!strQuote.isNull()) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == strQuote) strQuote = QChar();
            continue;
        }
        if (c == '"' || c == '\'') { strQuote = c; continue; }
        if (c == '/' && nx == '/') { lineComment = true; ++i; continue; }
        if (c == '/' && nx == '*') { blockComment = true; ++i; continue; }
        if (c == '(' || c == '[' || c == '{') {
            stack.append({c, i});
        } else if (c == ')' || c == ']' || c == '}') {
            const QChar want = openOf(c);
            // En yakın eşleşen açılışı bul (aradakiler kapanmamış sayılır)
            for (int s = stack.size() - 1; s >= 0; --s) {
                if (stack[s].ch == want) {
                    const int depth = s; // iç içelik seviyesi
                    out.append({stack[s].pos, depth, true});
                    out.append({i, depth, false});
                    stack.remove(s);
                    break;
                }
            }
        }
        if (out.size() > 6000) break; // güvenlik cap'i
    }
    return out;
}
