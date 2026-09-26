#pragma once
#include "../core/TaskRunner.h"
#include <QWidget>

class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QFileSystemWatcher;
class QTimer;

// tasks.json görevlerini listeler, çalıştırır ve çıktıyı gösterir.
class TaskPanel : public QWidget {
    Q_OBJECT
public:
    explicit TaskPanel(QWidget* parent = nullptr);

    void setRoot(const QString& root);
    void reload();
    QString root() const { return m_root; }
    TaskRunner* runner() const { return m_runner; } // Stage 30: zincir için

signals:
    void taskDone(const QString& label, int code); // Stage 30: zincir devamı

public slots:
    void runTask(const QString& label);
    void runSelected();
    void createOrOpenConfig();

private:
    void appendOut(const QString& text);
    // Stage 28: ${input:} çözümleme + izleme görevleri
    QMap<QString, QString> resolveInputs();
    void setupWatch();

    TaskRunner* m_runner;
    QListWidget* m_list;
    QPlainTextEdit* m_out;
    QPushButton* m_run;
    QPushButton* m_stop;
    QPushButton* m_cfg;
    QString m_root;
    bool m_usingDefault = false;
    QFileSystemWatcher* m_watchFiles = nullptr;
    QTimer* m_watchTimer = nullptr;
    QStringList m_watchLabels; // bitince izlenen görevler
    qint64 m_watchLastMs = 0;  // Stage 31: taşma sayacı
    int m_watchBursts = 0;
};
