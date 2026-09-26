#pragma once
#include <QDialog>

// Stage 28: git etiket yönetimi — listele/oluştur/sil.
class GitTagsDialog : public QDialog {
    Q_OBJECT
public:
    explicit GitTagsDialog(const QString& workdir, QWidget* parent = nullptr);

private slots:
    void refreshList();
    void createTag();
    void deleteTag();

private:
    QString runGit(const QStringList& args) const;
    QString m_dir;
    class QListWidget* m_list = nullptr;
};
