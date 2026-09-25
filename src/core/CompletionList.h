#pragma once
#include <QJsonObject>
#include <QList>
#include <QString>

// Stage 13: tamamlama öğesi — LSP CompletionItem parse + prefix filtre + sıralama.
struct CompletionItem {
    QString label;
    QString detail;
    QString insertText;
    int kind = 1; // 1 Text ... 3 Function, 6 Variable, 7 Class, 14 Keyword
    QString sortText;
    QString filterText;
};

class CompletionList {
public:
    static QList<CompletionItem> parse(const QJsonObject& res);
    static QList<CompletionItem> filter(const QList<CompletionItem>& items,
                                       const QString& prefix, int limit = 100);
    static QString kindTitle(int kind);
    static QString kindIcon(int kind); // tek harfli gösterim: ƒ, C, v, m...
};
