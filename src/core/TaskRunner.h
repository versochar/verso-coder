#pragma once
#include "TaskChain.h"
#include <QJsonArray>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

struct TaskDef {
    QString label;
    QString command;
    QString cwd;    // boşsa proje kökü
    QString group;  // "build" | "test" | "" ...
    int scheduleMin = 0; // Stage 25: >0 ise dakikada bir arka planda koşar
    bool watch = false;   // Stage 28: dosya değişiminde yeniden koş
};

// tasks.json çalıştırıcı (VS Code benzeri minimal biçim).
// Arama sırası: <root>/.verso/tasks.json, <root>/.mitsune/tasks.json, <root>/tasks.json
class TaskRunner : public QObject {
    Q_OBJECT
public:
    explicit TaskRunner(QObject* parent = nullptr);

    static QList<TaskDef> parseTasksJson(const QString& json, QString* error = nullptr);
    static QString defaultTasksJson();
    static QString configPathForRoot(const QString& root); // ilk bulunan veya <root>/.verso/tasks.json

    bool loadForRoot(const QString& root);
    QList<TaskDef> tasks() const { return m_tasks; }
    bool isRunning() const { return m_proc.state() != QProcess::NotRunning; }

    void run(int index, const QString& root);
    void runLabel(const QString& label, const QString& root);
    // Stage 28: ${input:} değerleriyle çalıştır
    void runLabelExpanded(const QString& label, const QString& root,
                          const QMap<QString, QString>& inputs);
    QList<TaskChain::TaskInput> taskInputs() const;
    QJsonArray tasksJsonArray() const; // Stage 30: dependsOn zinciri için
    void kill();

signals:
    void started(const QString& label);
    void outputChunk(const QString& text);
    void finished(const QString& label, int exitCode);

private:
    void startProcess(const TaskDef& t, const QString& root);

    QList<TaskDef> m_tasks;
    QString m_lastJson; // Stage 28: inputs bölümü için ham metin
    QProcess m_proc;
    QString m_current;
};
