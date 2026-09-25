#include "ReplaceEngine.h"
#include <QRegularExpression>

bool ReplaceEngine::globListMatch(const QString& str, const QString& spaceSeparated) {
    if (spaceSeparated.trimmed().isEmpty()) return false;
    for (const QString& pat : spaceSeparated.split(' ', Qt::SkipEmptyParts)) {
        QRegularExpression rx(QRegularExpression::wildcardToRegularExpression(pat.trimmed()),
                              QRegularExpression::CaseInsensitiveOption);
        if (rx.match(str).hasMatch()) return true;
    }
    return false;
}

bool ReplaceEngine::fileAllowed(const QString& fileName, const QString& includeFilter,
                                const QString& excludeFilter) {
    const QString base =
        fileName.contains('/') ? fileName.mid(fileName.lastIndexOf('/') + 1) : fileName;
    if (!includeFilter.trimmed().isEmpty()
        && !globListMatch(base, includeFilter)
        && !globListMatch(fileName, includeFilter))
        return false;
    if (globListMatch(base, excludeFilter) || globListMatch(fileName, excludeFilter))
        return false;
    return true;
}

QString ReplaceEngine::preserveCase(const QString& matched, const QString& replacement) {
    if (matched.isEmpty() || replacement.isEmpty()) return replacement;
    // TAMAMEN_BÜYÜK / tamamen küçük / İlkharf
    bool allUpper = true, allLower = true;
    for (QChar c : matched) {
        if (!c.isLetter()) continue;
        if (!c.isUpper()) allUpper = false;
        if (!c.isLower()) allLower = false;
    }
    if (allUpper && !allLower) return replacement.toUpper();
    if (allLower && !allUpper) return replacement.toLower();
    if (matched[0].isUpper())
        return replacement[0].toUpper() + replacement.mid(1);
    return replacement;
}

QPair<QString, QList<ReplaceEdit>> ReplaceEngine::applyFile(
    const QString& file, const QString& text, const QString& pattern,
    const QString& replacement, bool useRegex, bool caseSens) {
    QList<ReplaceEdit> edits;
    QRegularExpression rx(
        useRegex ? pattern : QRegularExpression::escape(pattern),
        caseSens ? QRegularExpression::NoPatternOption
                 : QRegularExpression::CaseInsensitiveOption);
    if (!rx.isValid()) return {text, edits};
    QStringList lines = text.split('\n');
    const bool keepCase = !useRegex && replacement == replacement.toLower()
        && !replacement.isEmpty();
    // VS Code uyumluluğu: regex modunda $1..$9 → Qt'nin \1..\9 biçimine çevir
    QString rep = replacement;
    if (useRegex) {
        static QRegularExpression dollar(R"(\$(\d))");
        // C++ "\\\\1" = Qt replace'te "\\" (tek \) + "1" → sonuç "\1" (yakalama)
        rep.replace(dollar, "\\\\1");
    }
    for (int i = 0; i < lines.size(); ++i) {
        const QString before = lines[i];
        QString after;
        if (keepCase) {
            // Eşleşme başına durum koru
            int pos = 0;
            QString out;
            auto it = rx.globalMatch(before);
            while (it.hasNext()) {
                auto m = it.next();
                out += before.mid(pos, m.capturedStart() - pos);
                out += preserveCase(m.captured(0), replacement);
                pos = m.capturedEnd();
            }
            out += before.mid(pos);
            after = out;
        } else {
            after = before;
            after.replace(rx, rep);
        }
        if (after != before) {
            lines[i] = after;
            edits << ReplaceEdit{file, i + 1, before.trimmed().left(160),
                                 after.trimmed().left(160)};
        }
    }
    return {lines.join('\n'), edits};
}
