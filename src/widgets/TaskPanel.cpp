#include "TaskPanel.h"
#include <QColor>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QUrl>
#include <QVBoxLayout>

TaskPanel::TaskPanel(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);

    auto* row = new QHBoxLayout();
    m_run = new QPushButton("▶ Çalıştır", this);
    m_stop = new QPushButton("■ Durdur", this);
    m_stop->setEnabled(false);
    m_cfg = new QPushButton("tasks.json", this);
    m_cfg->setToolTip("Çalışma alanında tasks.json oluştur veya aç");
    auto* bReload = new QPushButton("⟳", this);
    bReload->setToolTip("Görevleri yenile");
    bReload->setFixedWidth(32);
    row->addWidget(m_run);
    row->addWidget(m_stop);
    row->addWidget(m_cfg);
    row->addWidget(bReload);
    row->addStretch(1);
    lay->addLayout(row);

    auto* body = new QHBoxLayout();
    m_list = new QListWidget(this);
    m_list->setMaximumWidth(240);
    m_out = new QPlainTextEdit(this);
    m_out->setReadOnly(true);
    m_out->setFont(QFont("Consolas, monospace", 10));
    m_out->setPlaceholderText("Görev çıktısı burada görünür.");
    body->addWidget(m_list);
    body->addWidget(m_out, 1);
    lay->addLayout(body, 1);

    m_runner = new TaskRunner(this);
    connect(m_run, &QPushButton::clicked, this, &TaskPanel::runSelected);
    connect(m_stop, &QPushButton::clicked, this, [this]() { m_runner->kill(); });
    connect(m_cfg, &QPushButton::clicked, this, &TaskPanel::createOrOpenConfig);
    connect(bReload, &QPushButton::clicked, this, &TaskPanel::reload);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &TaskPanel::runSelected);

    connect(m_runner, &TaskRunner::started, this, [this](const QString& label) {
        m_run->setEnabled(false);
        m_stop->setEnabled(true);
        appendOut(QString("\n=== %1 ===\n").arg(label));
    });
    connect(m_runner, &TaskRunner::outputChunk, this, &TaskPanel::appendOut);
    connect(m_runner, &TaskRunner::finished, this, [this](const QString& label, int code) {
        m_run->setEnabled(true);
        m_stop->setEnabled(false);
        appendOut(QString("\n[%1 bitti — çıkış %2]\n").arg(label).arg(code));
    });
}

void TaskPanel::appendOut(const QString& text) {
    m_out->moveCursor(QTextCursor::End);
    m_out->insertPlainText(text);
    m_out->moveCursor(QTextCursor::End);
}

void TaskPanel::setRoot(const QString& root) {
    m_root = root;
    reload();
}

void TaskPanel::reload() {
    m_list->clear();
    m_usingDefault = false;
    if (m_root.isEmpty()) return;
    if (m_runner->loadForRoot(m_root)) {
        // yüklendi
    } else {
        QString err;
        m_runner->parseTasksJson(TaskRunner::defaultTasksJson(), &err);
        // varsayılanları listeye koy (runner'da değil, sadece göster)
        m_usingDefault = true;
    }
    if (m_usingDefault) {
        QString err;
        for (const TaskDef& t : TaskRunner::parseTasksJson(TaskRunner::defaultTasksJson(), &err))
            m_list->addItem("(varsayılan) " + t.label);
        auto* hint = new QListWidgetItem("— tasks.json oluştur —");
        hint->setForeground(QColor("#858585"));
        m_list->addItem(hint);
    } else {
        for (const TaskDef& t : m_runner->tasks())
            m_list->addItem(t.label + (t.group.isEmpty() ? "" : "  [" + t.group + "]"));
        if (m_list->count() == 0) m_list->addItem("(görev yok)");
    }
}

void TaskPanel::runSelected() {
    const int idx = m_list->currentRow();
    if (idx < 0) return;
    const QString label = m_list->item(idx)->text();
    if (label.startsWith("—")) { createOrOpenConfig(); return; }
    runTask(label);
}

void TaskPanel::runTask(const QString& label) {
    if (m_usingDefault) {
        QString err;
        const QList<TaskDef> defs = TaskRunner::parseTasksJson(TaskRunner::defaultTasksJson(), &err);
        for (const TaskDef& t : defs) {
            if (label.contains(t.label)) {
                m_out->clear();
                appendOut(QString("(varsayılan görev) %1\n$ %2\n").arg(t.label, t.command));
                // runner'a geçici yükleme olmadan çalıştırmak için doğrudan komut
                m_run->setEnabled(false);
                m_stop->setEnabled(true);
                QProcess* p = new QProcess(this);
                p->setWorkingDirectory(m_root);
                connect(p, &QProcess::readyReadStandardOutput, this,
                        [this, p]() { appendOut(QString::fromUtf8(p->readAllStandardOutput())); });
                connect(p, &QProcess::readyReadStandardError, this,
                        [this, p]() { appendOut(QString::fromUtf8(p->readAllStandardError())); });
                connect(p, &QProcess::finished, this, [this, p, t](int code, QProcess::ExitStatus) {
                    appendOut(QString("\n[%1 bitti — çıkış %2]\n").arg(t.label).arg(code));
                    m_run->setEnabled(true);
                    m_stop->setEnabled(false);
                    p->deleteLater();
                });
                p->start("bash", {"-lc", t.command});
                return;
            }
        }
        return;
    }
    m_runner->runLabel(label.split("  [").first().trimmed(), m_root);
}

void TaskPanel::createOrOpenConfig() {
    if (m_root.isEmpty()) return;
    const QString path = TaskRunner::configPathForRoot(m_root);
    if (!QFile::exists(path)) {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            f.write(TaskRunner::defaultTasksJson().toUtf8());
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    reload();
}
