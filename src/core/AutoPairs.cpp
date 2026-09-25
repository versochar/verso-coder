#include "AutoPairs.h"

bool AutoPairs::isOpener(QChar c) {
    return c == '(' || c == '[' || c == '{' || c == '"' || c == '\'';
}

QChar AutoPairs::matching(QChar c) {
    if (c == '(') return ')';
    if (c == '[') return ']';
    if (c == '{') return '}';
    return c; // tırnaklar kendine kapanır
}

AutoPairs::KeyResult AutoPairs::onOpen(QChar open, QChar close, const QString& lineBefore,
                                       const QString& lineAfter, bool inString,
                                       bool inComment, bool enabled) {
    KeyResult r;
    if (!enabled || inComment) return r;
    // Tırnak: kelime karakterinden sonra geliyorsa kapatma (don't → d''on't engeli)
    if ((open == '"' || open == '\'') && !lineBefore.isEmpty()) {
        const QChar b = lineBefore.back();
        if (b.isLetterOrNumber() || b == '_') return r;
        if (inString && open == '\'') {
            // dize içinde tek tırnak: kapatma
            r.handled = true;
            r.insert = QString(open);
            return r;
        }
    }
    // Sonraki karakter kelimeyse kapatma (foo(|bar → foo((|bar engeli)
    if (!lineAfter.isEmpty()) {
        const QChar a = lineAfter.front();
        if (a.isLetterOrNumber() || a == '_' || a == close) {
            if (a == close) return r; // skip mantığı çağıranda
            if (open != '"' && open != '\'') return r;
        }
    }
    r.handled = true;
    r.insert = QString(open) + QString(close);
    r.cursorBack = 1;
    return r;
}

bool AutoPairs::shouldSkip(QChar close, const QString& lineAfter) {
    return !lineAfter.isEmpty() && lineAfter.front() == close;
}

AutoPairs::KeyResult AutoPairs::wrapSelection(QChar open, QChar close, bool hasSelection) {
    KeyResult r;
    r.handled = true;
    if (hasSelection) {
        r.insert = QString(open) + QString(QChar(0xFFFF)) + QString(close); // \uFFFF = seçim yeri
    } else {
        r.insert = QString(open) + QString(close);
        r.cursorBack = 1;
    }
    return r;
}

QString AutoPairs::enterIndent(const QString& lineBefore, const QString& lineAfter,
                               const QString& baseIndent, bool useSpaces, int tabWidth) {
    const QString unit = useSpaces ? QString(tabWidth, ' ') : "\t";
    const QString b = lineBefore.trimmed();
    const QString a = lineAfter.trimmed();
    const bool openBrace = b.endsWith('{') || b.endsWith('(') || b.endsWith('[')
        || b.endsWith(':');
    const bool closeFirst = a.startsWith('}') || a.startsWith(')') || a.startsWith(']');
    if (openBrace && closeFirst)
        return "\n" + baseIndent + unit + "\n" + baseIndent; // araya satır, kapanışı hizala
    if (openBrace) return "\n" + baseIndent + unit;
    return "\n" + baseIndent;
}
