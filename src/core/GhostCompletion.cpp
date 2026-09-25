#include "GhostCompletion.h"

QString GhostCompletion::buildPrompt(const QString& prefix, const QString& suffix,
                                     const QString& lang) {
    // Son 40 satır önek + ilk 15 satır sonek (bütçe dostu FIM)
    QStringList pl = prefix.split('\n');
    if (pl.size() > 40) pl = pl.mid(pl.size() - 40);
    QStringList sl = suffix.split('\n');
    if (sl.size() > 15) sl = sl.mid(0, 15);
    return QString(
               "Sen bir %1 kod tamamlama motorusun. SADECE imleç konumuna gelecek "
               "devam kodunu yaz, açıklama yok, kod bloğu işareti yok.\n"
               "ÖNEK:\n%2\n<imleç>\nSONEK:\n%3\nDEVAM:")
        .arg(lang, pl.join('\n'), sl.join('\n'));
}

QString GhostCompletion::clean(const QString& reply, const QString& prefixEnd) {
    QString t = reply.trimmed();
    // Fence temizle
    if (t.startsWith("```")) {
        int nl = t.indexOf('\n');
        t = (nl > 0) ? t.mid(nl + 1) : t.mid(3);
        int end = t.lastIndexOf("```");
        if (end >= 0) t = t.left(end);
        t = t.trimmed();
    }
    // Önek tekrarıysa at
    if (!prefixEnd.isEmpty() && t.startsWith(prefixEnd))
        t = t.mid(prefixEnd.size());
    t = t.trimmed();
    // En fazla 3 satır
    QStringList lines = t.split('\n');
    if (lines.size() > 3) lines = lines.mid(0, 3);
    t = lines.join('\n').trimmed();
    if (t.size() > 300) t = t.left(300);
    return t;
}

bool GhostCompletion::shouldTrigger(QChar before, QChar before2) {
    Q_UNUSED(before2);
    if (before.isNull() || before == '\n') return false;
    if (before.isLetterOrNumber() || before == '_') return true;
    if (before == '.' || before == '(' || before == ',' || before == ' '
        || before == '=' || before == ':')
        return true;
    return false;
}

bool GhostCompletion::stillValid(const QString& currentPrefix,
                                 const QString& ghostPrefix) {
    if (ghostPrefix.isEmpty()) return false;
    // Kullanıcı yazmaya devam ettiyse önek, istek anındaki öneki kapsar
    return currentPrefix.startsWith(ghostPrefix);
}
