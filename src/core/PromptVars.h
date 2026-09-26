#pragma once
#include <QDate>
#include <QMap>
#include <QString>

// Stage 25: istem şablon değişkenleri — saf genişletme (test edilebilir).
// Desteklenenler: {{selection}} {{file}} {{filename}} {{date}} {{lang}}
class PromptVars {
public:
    static QString expand(const QString& tpl, const QMap<QString, QString>& vars);
    static QString expand(const QString& tpl, const QString& selection,
                          const QString& filePath);
};
