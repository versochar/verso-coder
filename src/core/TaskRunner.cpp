#include "TaskRunner.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

TaskRunner::TaskRunner(QObject* parent) : QObject(parent) {
    connect(&m_proc, &QProcess::readyReadStandardOutput, this, [this]() {
        emit outputChunk(QString::fromUtf8(m_proc.readAllStandardOutput()));
    });
    connect(&m_proc, &QProcess::readyReadStandardError, this, [this]() {
        emit outputChunk(QString::fromUtf8(m_proc.readAllStandardError()));
    });
    connect(&m_proc, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        emit finished(m_current, code);
        m_current.clear();
    });
    connect(&m_proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        emit outputChunk("\n[hata] " + m_proc.errorString() + "\n");
        if (!m_current.isEmpty()) {
            emit finished(m_current, -1);
            m_current.clear();
        }
    });
}

QList<TaskDef> TaskRunner::parseTasksJson(const QString& json, QString* error) {
    QList<TaskDef> out;
    if (error) error->clear();
    QJsonDocument d = QJsonDocument::fromJson(json.toUtf8());
    if (!d.isObject() && !d.isArray()) {
        if (error) *error = "geçersiz JSON";
        return out;
    }
    QJsonArray arr = d.isArray() ? d.array() : d.object().value("tasks").toArray();
    for (const QJsonValue& v : arr) {
        QJsonObject o = v.toObject();
        TaskDef t;
        t.label = o.value("label").toString();
        t.command = o.value("command").toString();
        t.cwd = o.value("cwd").toString();
        // group "build"/"test" ya da {kind:"build"} olabilir
        if (o.value("group").isObject())
            t.group = o.value("group").toObject().value("kind").toString();
        else if (o.value("group").isString())
            t.group = o.value("group").toString();
        if (!t.label.isEmpty() && !t.command.isEmpty()) out << t;
    }
    if (out.isEmpty() && error && !arr.isEmpty()) *error = "geçerli görev yok";
    return out;
}

QString TaskRunner::defaultTasksJson() {
    return QString::fromUtf8(QJsonDocument(QJsonObject{
        {"tasks", QJsonArray{
            QJsonObject{{"label", "Build"}, {"command", "cmake --build build"}, {"group", "build"}},
            QJsonObject{{"label", "Test"}, {"command", "ctest --test-dir build --output-on-failure"}, {"group", "test"}},
            QJsonObject{{"label", "Configure"}, {"command", "cmake -S . -B build -DCMAKE_BUILD_TYPE=Release"}},
        }}}).toJson(QJsonDocument::Indented));
}

QString TaskRunner::configPathForRoot(const QString& root) {
    const QString v = QDir(root).absoluteFilePath(".verso/tasks.json");
    if (QFile::exists(v)) return v;
    const QString a = QDir(root).absoluteFilePath(".mitsune/tasks.json");
    if (QFile::exists(a)) return a;
    const QString b = QDir(root).absoluteFilePath("tasks.json");
    if (QFile::exists(b)) return b;
    return v; // varsayılan (yoksa oluşturma hedefi)
}

bool TaskRunner::loadForRoot(const QString& root) {
    m_tasks.clear();
    QFile f(configPathForRoot(root));
    if (!f.open(QIODevice::ReadOnly)) return false;
    QString err;
    m_tasks = parseTasksJson(QString::fromUtf8(f.readAll()), &err);
    return !m_tasks.isEmpty();
}

void TaskRunner::startProcess(const TaskDef& t, const QString& root) {
    m_current = t.label;
    emit started(t.label);
    const QString dir = t.cwd.isEmpty() ? root : QDir(root).absoluteFilePath(t.cwd);
    m_proc.setWorkingDirectory(dir.isEmpty() ? QDir::currentPath() : dir);
    m_proc.start("bash", {"-lc", t.command});
}

void TaskRunner::run(int index, const QString& root) {
    if (index < 0 || index >= m_tasks.size()) return;
    if (isRunning()) kill();
    startProcess(m_tasks[index], root);
}

void TaskRunner::runLabel(const QString& label, const QString& root) {
    for (int i = 0; i < m_tasks.size(); ++i)
        if (m_tasks[i].label.compare(label, Qt::CaseInsensitive) == 0) {
            run(i, root);
            return;
        }
}

void TaskRunner::kill() {
    if (m_proc.state() != QProcess::NotRunning) {
        m_proc.kill();
        m_proc.waitForFinished(3000);
    }
}
