#include "InlineEdit.h"

QString InlineEdit::buildPrompt(const QString& instruction, const QString& code,
                                const QString& lang) {
    return QString(
               "Sen bir %1 kod düzenleyicisisin. Aşağıdaki TALİMAT'a göre KOD'u "
               "yeniden yaz. SADECE yeni kodu ```%1 bloğunda döndür, açıklama yazma.\n"
               "TALİMAT: %2\nKOD:\n```%1\n%3\n```")
        .arg(lang, instruction, code.left(6000));
}

QString InlineEdit::extractCode(const QString& reply) {
    // Son ``` bloğu: sondaki kapatma çitinden geriye açılışı bul
    const int end = reply.lastIndexOf("```");
    if (end < 0) return reply.trimmed();
    const int start = reply.lastIndexOf("```", end - 1);
    if (start < 0) return reply.mid(end + 3).trimmed(); // tek çit: sonrasını al
    QString inner = reply.mid(start + 3, end - start - 3).trimmed();
    // Dil etiketi satırını at (```cpp\n...)
    const int nl = inner.indexOf('\n');
    if (nl >= 0 && !inner.left(nl).contains(' ') && !inner.left(nl).contains('\n')
        && inner.left(nl).size() <= 12)
        inner = inner.mid(nl + 1).trimmed();
    return inner.isEmpty() ? reply.trimmed() : inner;
}
