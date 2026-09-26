#pragma once
#include <QDialog>

// Stage 28: dosya geçmişi (`git log --follow`) + sürüm farkı.
class GitHistoryDialog : public QDialog {
    Q_OBJECT
public:
    explicit GitHistoryDialog(const QString& workdir, const QString& file,
                              QWidget* parent = nullptr);

private slots:
    void refreshList();
    void showDiff();

private:
    QString m_dir;
    QString m_file;
    class QTreeWidget* m_list = nullptr;
    class QTextEdit* m_diff = nullptr;
};
