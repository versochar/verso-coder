#pragma once
#include <QProcess>
#include <QTextEdit>
#include <QTreeWidget>
#include <QWidget>

class QLineEdit;
class QListWidget;
class QComboBox;

// Geçmiş grafiği + cherry-pick/revert, stash, uzak depo sekmeleri.
class HistoryTab : public QWidget {
    Q_OBJECT
public:
    explicit HistoryTab(QWidget* parent = nullptr);
    void setWorkdir(const QString& dir);

signals:
    void fileOpenRequested(const QString& path);

private slots:
    void refresh();
    void showSelected();

private:
    QString runGit(const QStringList& args);
    QString m_dir;
    QTreeWidget* m_log;
    QTextEdit* m_detail;
    QLineEdit* m_filter;
};

class StashTab : public QWidget {
    Q_OBJECT
public:
    explicit StashTab(QWidget* parent = nullptr);
    void setWorkdir(const QString& dir);

private slots:
    void refresh();
    void push();
    void apply(int pop);

private:
    QString runGit(const QStringList& args);
    QString m_dir;
    QListWidget* m_list;
    QLineEdit* m_msg;
};

class RemoteTab : public QWidget {
    Q_OBJECT
public:
    explicit RemoteTab(QWidget* parent = nullptr);
    void setWorkdir(const QString& dir);

private slots:
    void refresh();

private:
    QString runGit(const QStringList& args);
    QString m_dir;
    QTextEdit* m_out;
    QLineEdit* m_name;
    QLineEdit* m_url;
};
