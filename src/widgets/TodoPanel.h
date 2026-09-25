#pragma once
#include <QFutureWatcher>
#include <QStringList>
#include <QWidget>

class QLineEdit;
class QTreeWidget;
class QLabel;

struct TodoHit {
    QString file;
    int line = 0;
    QString tag;
    QString text;
};

// TODO/FIXME/HACK tarayıcı (arka plan taramalı).
class TodoPanel : public QWidget {
    Q_OBJECT
public:
    explicit TodoPanel(QWidget* parent = nullptr);
    void setRoot(const QString& root);

    static QList<TodoHit> scanSync(const QString& root, const QString& pattern,
                                   int maxHits = 2000);

signals:
    void fileOpened(const QString& path, int line);

private slots:
    void runScan();
    void onScanDone();

private:
    QString m_root;
    QLineEdit* m_pattern;
    QTreeWidget* m_list;
    QLabel* m_status;
    QFutureWatcher<QList<TodoHit>> m_watcher;
};
