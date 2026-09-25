#pragma once
#include <QJsonObject>
#include <QList>
#include <QString>

// Stage 13: kod eylemleri — CodeAction yanıtı parse + kind filtreleme.
struct CodeActionItem {
    QString title;
    QString kind; // quickfix, refactor, source...
    bool hasEdit = false;
    bool hasCommand = false;
    QJsonObject raw;
};

class CodeActionList {
public:
    // Yanıt: (Command|CodeAction)[] — Command ise {title, command}
    static QList<CodeActionItem> parse(const QJsonObject& res);
    static QList<CodeActionItem> parseArray(const class QJsonArray& arr);
    static QList<CodeActionItem> filterByKind(const QList<CodeActionItem>& items,
                                             const QString& kindPrefix);
};
