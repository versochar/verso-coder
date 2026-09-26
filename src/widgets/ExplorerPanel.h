#pragma once
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QWidget>

class GitIgnore;
class TrashManager;

// .gitignore kurallarını dosya ağacına uygulayan proxy model.
class GitIgnoreFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit GitIgnoreFilterModel(QObject* parent = nullptr);
    void setIgnore(const GitIgnore* ig) { m_ig = ig; invalidateFilter(); }
    void refresh() { invalidateFilter(); } // korumalı invalidateFilter sarmalayıcı
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    const GitIgnore* m_ig = nullptr;
};

class ExplorerPanel : public QWidget {
    Q_OBJECT
public:
    explicit ExplorerPanel(QWidget* parent = nullptr);
    void setRoot(const QString& path);
    QString root() const { return m_root; }
    void setGitIgnore(GitIgnore* ig);
    void setTrash(TrashManager* trash) { m_trash = trash; }

signals:
    void fileOpened(const QString& path);
    void filePreviewRequested(const QString& path); // Stage 24: tek tık önizleme
    void compareRequested(const QString& path);  // seçili dosya: git ile karşılaştır
    void pathRenamed(const QString& oldPath, const QString& newPath); // taşı/yeniden adlandır

private slots:
    void onContextMenu(const QPoint& pos);

protected:
    bool eventFilter(QObject* o, QEvent* e) override; // Stage 21: F2/Del/F5

private:
    QString targetDir(const QModelIndex& proxyIdx) const;
    void newFile(const QString& base);
    void newDir(const QString& base);
    void removePath(const QString& path);
    bool renamePath(const QString& oldPath, const QString& newName);

    QFileSystemModel* m_model;
    GitIgnoreFilterModel* m_proxy;
    QTreeView* m_tree;
    QString m_root;
    GitIgnore* m_ig = nullptr;
    TrashManager* m_trash = nullptr;
};
