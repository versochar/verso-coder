#pragma once
#include <QList>
#include <QRegularExpression>
#include <QString>

// .gitignore eşleştirici (kök .gitignore; *, ?, **, {}, !, dizin/ destekli).
class GitIgnore {
public:
    void load(const QString& root); // root/.gitignore okur (yoksa boş)
    bool isIgnored(const QString& absPath, bool isDir) const;
    bool hasRules() const { return !m_rules.isEmpty(); }

    struct Rule {
        QRegularExpression rx; // göreli yola karşı
        bool neg = false;
        bool dirOnly = false;
    };
    static QList<Rule> parseFile(const QString& filePath);
    static bool matchRules(const QList<Rule>& rules, const QString& relPath, bool isDir);

private:
    QList<Rule> m_rules;
    QString m_root;
};
