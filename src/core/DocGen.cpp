#include "DocGen.h"
#include <QRegularExpression>

int DocGen::findFuncStart(const QString& text, int line0) {
    const QStringList lines = text.split('\n');
    // İmleç satırından yukarı: "ad(...)" + "{" içeren ilk satır
    static QRegularExpression sig(
        R"(^\s*(?:[\w:\<\>\*&]+\s+)+(\w+)\s*\([^;]*\)\s*(?:const\s*)?(?:\{|:)?\s*$)");
    for (int i = qBound(0, line0, lines.size() - 1); i >= qMax(0, line0 - 40); --i) {
        const QString t = lines[i].trimmed();
        if (t.isEmpty() || t.startsWith("//") || t.startsWith("#")
            || t.startsWith("*") || t.startsWith("/*"))
            continue;
        if (t.endsWith(';')) continue; // bildirim, tanım değil
        if (sig.match(lines[i]).hasMatch()) return i;
        // Python: "def ad(...):"
        static QRegularExpression py(R"(^\s*def\s+\w+\s*\(.*\)\s*:\s*$)");
        if (py.match(lines[i]).hasMatch()) return i;
    }
    return -1;
}

QString DocGen::commentPrefix(const QString& suffix) {
    const QString s = suffix.toLower();
    if (s == "py" || s == "sh" || s == "rb") return "#";
    return "//";
}

QString DocGen::buildPrompt(const QString& code, const QString& lang) {
    return QString(
               "Şu %1 fonksiyonu için kısa Türkçe Doxygen/JSDoc tarzı belge yorumu yaz. "
               "SADECE yorum bloğunu döndür, kod yazma:\n```%1\n%2\n```")
        .arg(lang, code.left(3000));
}
