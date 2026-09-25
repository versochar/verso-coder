#include "ScopeChain.h"

int ScopeChain::indentOf(const QString& line, int tabWidth) {
    int n = 0;
    for (QChar ch : line) {
        if (ch == ' ') ++n;
        else if (ch == '\t') n += tabWidth;
        else break;
    }
    return n;
}

bool ScopeChain::isHeaderLine(const QString& trimmed) {
    if (trimmed.isEmpty()) return false;
    // Erişim belirteçleri ve case etiketleri kapsam değildir (gürültü)
    if (trimmed == "public:" || trimmed == "private:" || trimmed == "protected:" ||
        trimmed == "signals:" || trimmed == "slots:" || trimmed == "default:" ||
        trimmed.startsWith("case "))
        return false;
    if (trimmed.endsWith('{') || trimmed.endsWith(':')) return true;
    if (trimmed.startsWith("class ") || trimmed.startsWith("struct ") ||
        trimmed.startsWith("def ") || trimmed.startsWith("namespace ") ||
        trimmed.startsWith("fn ") || trimmed.startsWith("func "))
        return true;
    return false;
}

QList<ScopeFrame> ScopeChain::chain(const QStringList& lines, int line0, int tabWidth) {
    QList<ScopeFrame> out;
    if (line0 < 0 || line0 >= lines.size()) return out;
    int curIndent = -1;
    // Başlangıç girintisi: imleç satırının kendisi (boşsa üstteki dolu satır)
    int probe = line0;
    while (probe >= 0 && lines[probe].trimmed().isEmpty()) --probe;
    if (probe < 0) return out;
    curIndent = indentOf(lines[probe], tabWidth);
    for (int i = probe; i >= 0; --i) {
        const QString& ln = lines[i];
        if (ln.trimmed().isEmpty()) continue;
        const int ind = indentOf(ln, tabWidth);
        if (ind < curIndent && isHeaderLine(ln.trimmed())) {
            ScopeFrame f;
            f.line0 = i;
            f.text = ln.trimmed().left(80);
            f.indent = ind;
            out.prepend(f);
            curIndent = ind;
            if (out.size() >= 4) break; // en fazla 4 seviye göster
        }
    }
    return out;
}

QString ScopeChain::label(const QList<ScopeFrame>& frames) {
    QStringList parts;
    for (const ScopeFrame& f : frames) {
        QString t = f.text;
        // gürültüyü kısalt: { ve : sonrasını at
        int cut = t.indexOf('{');
        if (cut > 0) t = t.left(cut).trimmed();
        if (t.endsWith(':')) t.chop(1);
        parts << t;
    }
    return parts.join(" › ");
}
