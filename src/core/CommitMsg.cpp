#include "CommitMsg.h"
#include <QRegularExpression>

QStringList CommitMsg::types() {
    return {"feat", "fix", "docs", "refactor", "test", "chore", "perf", "build"};
}

CommitParts CommitMsg::parse(const QString& msg) {
    CommitParts p;
    static QRegularExpression re(R"(^(\w+)(?:\(([^)]*)\))?:\s*(.+)$)");
    auto m = re.match(msg.trimmed().split('\n').first().trimmed());
    if (!m.hasMatch()) return p;
    p.type = m.captured(1).toLower();
    p.scope = m.captured(2);
    p.subject = m.captured(3).trimmed();
    p.valid = types().contains(p.type) && !p.subject.isEmpty()
        && p.subject.size() <= 72 && !p.subject.endsWith('.');
    return p;
}

bool CommitMsg::isValid(const QString& msg) {
    return parse(msg).valid;
}

QString CommitMsg::format(const QString& type, const QString& scope,
                          const QString& subject) {
    QString s = subject.trimmed();
    if (s.endsWith('.')) s.chop(1);
    s = s.left(72);
    if (scope.trimmed().isEmpty()) return QString("%1: %2").arg(type, s);
    return QString("%1(%2): %3").arg(type, scope.trimmed(), s);
}

QString CommitMsg::buildPrompt(const QString& diff) {
    return "Aşağıdaki git diff için Conventional Commits tarzı TEK SATIRLIK Türkçe "
           "commit mesajı yaz (örn. \"feat(auth): token yenileme eklendi\"). "
           "SADECE mesajı döndür:\n```diff\n" + diff.left(6000) + "\n```";
}
