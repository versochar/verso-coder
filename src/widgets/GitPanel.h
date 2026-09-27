#pragma once
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QProcess>
#include <QTextEdit>
#include <QWidget>

// Dosya bazlı stage/unstage + diff + branch + log + hunk diff.
class GitPanel : public QWidget {
    Q_OBJECT
public:
    explicit GitPanel(QWidget* parent = nullptr);
    void setWorkdir(const QString& dir);

public slots:
    void refresh();
    void setCommitMessage(const QString& msg); // Stage 15: AI üretimi doldurur

signals:
    void fileOpenRequested(const QString& path);

private slots:
    void refreshBranches();
    void refreshLog();
    void stageSelected();
    void unstageSelected();
    void showDiff(bool cached);
    void showHunkDiff(bool cached); // DiffDialog: hunk stage/discard
    void commit();
    void openSelected();
    void stashSave();   // Stage 43
    void stashApply();
    void stashDrop();
    void refreshStash();

private:
    struct Cmd { int exit = -1; QString out; QString err; };
    Cmd runGit(const QStringList& args, int timeoutMs = 8000);
    void log(const QString& cmd, const QString& text);
    QString selectedFile() const;

    QString m_dir;
    QComboBox* m_branches;
    QComboBox* m_stash = nullptr; // Stage 43
    QLabel* m_ahead = nullptr;    // Stage 43: önde/geride
    QLineEdit* m_newBranch;
    QListWidget* m_files;
    QTextEdit* m_out;
    QLineEdit* m_msg;
    QComboBox* m_tpl = nullptr; // Stage 21: ileti şablonu
    QCheckBox* m_amend = nullptr; // Stage 21: commit --amend
};
