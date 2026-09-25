#include "Spelling.h"
#include <QRegularExpression>

// Basit tarayıcı: //... #... /*...*/ ve "..." '...' `...` aralıkları.
// Durum makinesi; kaçış karakterlerini (\" \\ ...) atlar.
QList<QPair<int, int>> Spelling::commentStringSpans(const QString& text) {
    QList<QPair<int, int>> out;
    int n = text.size();
    if (n > 1000000) return out;
    int i = 0;
    auto addSpan = [&](int s, int e) {
        if (e > s) out.append({s, e - s});
    };
    while (i < n) {
        QChar c = text[i];
        if (c == '/' && i + 1 < n && text[i + 1] == '/') {
            int s = i;
            while (i < n && text[i] != '\n') ++i;
            addSpan(s, i);
        } else if (c == '#') {
            // shebang dahil tüm # yorumları (basitlik için)
            int s = i;
            while (i < n && text[i] != '\n') ++i;
            addSpan(s, i);
        } else if (c == '/' && i + 1 < n && text[i + 1] == '*') {
            int s = i;
            i += 2;
            while (i + 1 < n && !(text[i] == '*' && text[i + 1] == '/')) ++i;
            i = qMin(n, i + 2);
            addSpan(s, i);
        } else if (c == '"' || c == '\'' || c == '`') {
            int s = i;
            QChar q = c;
            ++i;
            while (i < n && text[i] != '\n') {
                if (text[i] == '\\') { i += 2; continue; }
                if (text[i] == q) { ++i; break; }
                ++i;
            }
            addSpan(s, i);
        } else {
            ++i;
        }
    }
    return out;
}

QList<SpellHit> Spelling::checkText(const QString& text, SpellChecker& sc, int maxHits) {
    QList<SpellHit> out;
    if (!sc.isOk()) return out;
    static const QRegularExpression wordRx("[\\p{L}]{3,}");
    for (const auto& span : commentStringSpans(text)) {
        QString frag = text.mid(span.first, span.second);
        for (auto it = wordRx.globalMatch(frag); it.hasNext();) {
            auto m = it.next();
            QString w = m.captured();
            if (w.toLower() == w && sc.check(w)) continue;
            if (!sc.check(w) && !sc.check(w.toLower())) {
                SpellHit h;
                h.pos = span.first + m.capturedStart();
                h.len = m.capturedLength();
                h.word = w;
                h.suggestions = sc.suggest(w, 5);
                out.append(h);
                if (out.size() >= maxHits) return out;
            }
        }
    }
    return out;
}
