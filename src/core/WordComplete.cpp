#include "WordComplete.h"
#include <QMap>
#include <QRegularExpression>
#include <algorithm>

bool WordComplete::isIdentChar(QChar c) {
    return c.isLetterOrNumber() || c == '_';
}

QSet<QString> WordComplete::keywords(const QString& suffix) {
    const QString s = suffix.toLower();
    if (s == "py")
        return {"def", "class", "return", "import", "from", "for", "while", "if",
                "elif", "else", "try", "except", "with", "lambda", "pass", "None",
                "True", "False", "self", "print"};
    if (s == "js" || s == "ts")
        return {"function", "return", "const", "let", "var", "if", "else", "for",
                "while", "new", "this", "true", "false", "null"};
    return {"int", "void", "class", "struct", "return", "if", "else", "for",
            "while", "const", "static", "public", "private", "new", "delete",
            "true", "false", "nullptr", "include", "namespace", "using"};
}

QMap<QString, int> WordComplete::collect(const QString& text, int minLen) {
    QMap<QString, int> out;
    static QRegularExpression re(R"(\b[A-Za-z_][A-Za-z0-9_]*\b)");
    auto it = re.globalMatch(text);
    while (it.hasNext())
        out[it.next().captured(0)]++;
    // Kısa ve sayısal ağırlıklıları ele
    for (auto k = out.begin(); k != out.end();) {
        if (k.key().size() < minLen) k = out.erase(k);
        else ++k;
    }
    return out;
}

QList<WordCand> WordComplete::suggest(const QString& prefix,
                                      const QList<QPair<QString, int>>& docs,
                                      const QString& suffix, int maxOut) {
    if (prefix.size() < 2) return {};
    const QSet<QString> kw = keywords(suffix);
    QMap<QString, WordCand> agg;
    for (const auto& doc : docs) {
        const QMap<QString, int> words = collect(doc.first, 4);
        for (auto it = words.begin(); it != words.end(); ++it) {
            if (!it.key().startsWith(prefix, Qt::CaseSensitive)) continue;
            if (it.key() == prefix) continue;
            if (kw.contains(it.key())) continue;
            WordCand& c = agg[it.key()];
            c.word = it.key();
            c.freq += it.value();
            c.dist = (c.dist == 0) ? doc.second : qMin(c.dist, doc.second);
        }
    }
    QList<WordCand> out = agg.values();
    std::sort(out.begin(), out.end(), [](const WordCand& a, const WordCand& b) {
        if (a.freq != b.freq) return a.freq > b.freq;
        if (a.word.size() != b.word.size()) return a.word.size() > b.word.size();
        return a.dist < b.dist;
    });
    if (out.size() > maxOut) out = out.mid(0, maxOut);
    return out;
}
