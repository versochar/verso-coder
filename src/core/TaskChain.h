#pragma once
#include <QJsonArray>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

// Stage 18: görev zinciri — dependsOn topolojik sıra + ${input:} + ${değişken}.
// TaskRunner::TaskDef alanlarıyla birlikte çalışır (saf çözümleyici burada).
class TaskChain {
public:
    // dependsOn'a göre çalıştırma sırası (etiket listesi); döngüde boş + error dolar
    static QList<QString> order(const QMap<QString, QStringList>& deps, QString& error);
    // ${input:ad} ve ${workspaceFolder} ${file} ${fileBasename} genişletme
    static QString expand(const QString& command, const QString& root,
                          const QString& file, const QMap<QString, QString>& inputs);
    // tasks.json "inputs" bölümü: [{id, default}] → id→değer (varsayılanlarla)
    static QMap<QString, QString> parseInputs(const QJsonArray& arr);
    // problemMatcher ("$gcc" | {pattern}) → dosya:satır sorunları
    struct ChainIssue { QString file; int line1 = 1; int col1 = 1; QString message; };
    static QList<ChainIssue> matchProblems(const QString& output, const QString& matcher,
                                           const QString& root);
    // dependsOn + inputs dahil görev tanımı genişletme (TaskDef ham JSON'dan)
    static QMap<QString, QStringList> depsFrom(const QJsonArray& tasks);
};
