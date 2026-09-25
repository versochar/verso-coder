#pragma once
#include <QString>

// Stage 15: Conventional Commits — doğrulama + ayrıştırma + istem.
struct CommitParts {
    QString type;    // feat|fix|docs|refactor|test|chore|...
    QString scope;   // ":alan" (boş olabilir)
    QString subject; // en fazla 72 karakter
    bool valid = false;
};

class CommitMsg {
public:
    static CommitParts parse(const QString& msg);
    static bool isValid(const QString& msg);
    static QString format(const QString& type, const QString& scope,
                          const QString& subject);
    static QString buildPrompt(const QString& diff);
    static QStringList types();
};
