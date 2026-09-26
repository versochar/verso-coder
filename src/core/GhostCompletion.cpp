#include "GhostCompletion.h"
#include "ai/LlmProvider.h"

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

GhostCompletion::Mode GhostCompletion::modeFor(const QString& providerId, bool cloudEnabled) {
    const ProviderSpec spec = ProviderRegistry::byId(providerId);
    const bool local = spec.id.isEmpty() || spec.kind == ProviderKind::Ollama;
    if (local) return Mode::Fim;
    return cloudEnabled ? Mode::Prefix : Mode::Off;
}

QString GhostCompletion::modeLabel(Mode m) {
    switch (m) {
    case Mode::Fim: return QStringLiteral("FIM (yerel)");
    case Mode::Prefix: return QStringLiteral("önek (bulut)");
    case Mode::Off: return QStringLiteral("kapalı");
    }
    return QStringLiteral("?");
}

bool GhostCompletion::wantsSuffix(const QString& providerId, bool cloudEnabled) {
    return modeFor(providerId, cloudEnabled) == Mode::Fim;
}

QString GhostCompletion::buildPromptPrefix(const QString& prefix, const QString& lang) {
    // Son ~40 satır bağlam; yalnız devam kodunu iste
    const QStringList lines = prefix.split(QLatin1Char('\n'));
    const int take = qMin(lines.size(), 40);
    const QStringList tail = lines.mid(lines.size() - take);
    return QStringLiteral("Aşağıdaki %1 kodunun devamını yaz. Yalnız kod, açıklama yok.\n\n"
                          "```%2\n%3")
        .arg(lang)
        .arg(lang.isEmpty() ? QString() : lang,
             tail.join(QLatin1Char('\n')));
}

QString GhostCompletion::buildPrompt(Mode mode, const QString& prefix, const QString& suffix,
                                     const QString& lang) {
    if (mode == Mode::Fim) return buildPrompt(prefix, suffix, lang);
    return buildPromptPrefix(prefix, lang);
}
