#pragma once
#include <QDialog>

class QListWidget;

// Git çalışma ağaçları: listele / ekle / kaldır / buda.
class WorktreeDialog : public QDialog {
    Q_OBJECT
public:
    explicit WorktreeDialog(const QString& root, QWidget* parent = nullptr);

private slots:
    void refreshList();
    void addWorktree();
    void removeSelected();

private:
    QString m_root;
    QListWidget* m_list = nullptr;
};
