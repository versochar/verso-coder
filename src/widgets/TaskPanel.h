#pragma once
#include "../core/TaskRunner.h"
#include <QWidget>

class QListWidget;
class QPlainTextEdit;
class QPushButton;

// tasks.json görevlerini listeler, çalıştırır ve çıktıyı gösterir.
class TaskPanel : public QWidget {
    Q_OBJECT
public:
    explicit TaskPanel(QWidget* parent = nullptr);

    void setRoot(const QString& root);
    void reload();
    QString root() const { return m_root; }

public slots:
    void runTask(const QString& label);
    void runSelected();
    void createOrOpenConfig();

private:
    void appendOut(const QString& text);

    TaskRunner* m_runner;
    QListWidget* m_list;
    QPlainTextEdit* m_out;
    QPushButton* m_run;
    QPushButton* m_stop;
    QPushButton* m_cfg;
    QString m_root;
    bool m_usingDefault = false;
};
