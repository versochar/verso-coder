#include "ExplorerPanel.h"
#include "../core/FileIcons.h"
#include "../core/GitIgnore.h"
#include "../core/TrashManager.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

// ---------- GitIgnoreFilterModel ----------
GitIgnoreFilterModel::GitIgnoreFilterModel(QObject* parent) : QSortFilterProxyModel(parent) {
    setDynamicSortFilter(true);
}

bool GitIgnoreFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    if (!m_ig || !m_ig->hasRules()) return true;
    auto* src = qobject_cast<QFileSystemModel*>(sourceModel());
    if (!src) return true;
    QModelIndex idx = src->index(sourceRow, 0, sourceParent);
    QString path = src->filePath(idx);
    if (path.isEmpty()) return true;
    return !m_ig->isIgnored(path, src->isDir(idx));
}

// ---------- ExplorerPanel ----------
ExplorerPanel::ExplorerPanel(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    auto* openBtn = new QPushButton(tr("Klasör Aç"), this);
    connect(openBtn, &QPushButton::clicked, this, [this]() {
        QString d = QFileDialog::getExistingDirectory(this, tr("Klasör Aç"));
        if (!d.isEmpty()) setRoot(d);
    });

    m_model = new QFileSystemModel(this);
    m_model->setReadOnly(false);
    m_model->setResolveSymlinks(false); // Stage 32: ek stat çağrısı yok
    m_model->setOption(QFileSystemModel::DontUseCustomDirectoryIcons, true); // Stage 32: hızlı ikon
    m_model->setIconProvider(new FileIconProvider()); // model sahiplenir (Qt6)
    m_model->setFilter(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    m_proxy = new GitIgnoreFilterModel(this);
    m_proxy->setSourceModel(m_model);

    m_tree = new QTreeView(this);
    m_tree->setModel(m_proxy);
    m_tree->setUniformRowHeights(true); // Stage 32: satır ölçümü tek kez → hızlı
    m_tree->setAnimated(false);
    for (int i = 1; i < m_model->columnCount(); ++i) m_tree->hideColumn(i);
    m_tree->setHeaderHidden(true);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tree->setDragDropMode(QAbstractItemView::InternalMove);
    m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers); // yeniden adlandırma diyalogla

    lay->addWidget(openBtn);
    lay->addWidget(m_tree, 1);

    // Stage 24: tek tık önizleme (çift tık tam açar)
    connect(m_tree, &QTreeView::clicked, this, [this](const QModelIndex& proxyIdx) {
        QModelIndex idx = m_proxy->mapToSource(proxyIdx);
        if (!idx.isValid()) return;
        QString p = m_model->filePath(idx);
        if (!m_model->isDir(idx)) emit filePreviewRequested(p);
    });
    connect(m_tree, &QTreeView::doubleClicked, this, [this](const QModelIndex& proxyIdx) {
        QModelIndex idx = m_proxy->mapToSource(proxyIdx);
        if (!idx.isValid()) return;
        QString p = m_model->filePath(idx);
        if (!m_model->isDir(idx)) emit fileOpened(p);
    });
    connect(m_tree, &QTreeView::customContextMenuRequested, this, &ExplorerPanel::onContextMenu);
    // Stage 21: klavye kısayolları — F2 yeniden adlandır, Del çöp kutusu, F5 yenile
    m_tree->installEventFilter(this);

    // Taşıma / yeniden adlandırmada açık sekmeleri güncelle + filtre tazele
    auto renamed = [this](const QString& a, const QString& b) {
        // Var olan yol "yeni", var olmayan "eski"dir (sinyal sırası garanti değil)
        QString newPath = QFileInfo::exists(a) ? a : b;
        QString oldPath = (newPath == a) ? b : a;
        m_proxy->refresh();
        emit pathRenamed(oldPath, newPath);
    };
    connect(m_model, &QFileSystemModel::fileRenamed, this, renamed);
}

void ExplorerPanel::setGitIgnore(GitIgnore* ig) {
    m_ig = ig;
    m_proxy->setIgnore(ig); // setIgnore invalidateRowsFilter çağırır
}
void ExplorerPanel::setRoot(const QString& path) {
    m_root = path;
    QModelIndex srcIdx = m_model->setRootPath(path);
    m_tree->setRootIndex(m_proxy->mapFromSource(srcIdx));
    if (m_tree->rootIndex().isValid())
        m_tree->expand(m_tree->rootIndex());
}

// Stage 21: ağaç odaktayken F2 / Del / F5
bool ExplorerPanel::eventFilter(QObject* o, QEvent* e) {
    if (o == m_tree && e->type() == QEvent::KeyPress) {
        auto* k = static_cast<QKeyEvent*>(e);
        const QModelIndex proxyIdx = m_tree->currentIndex();
        const QModelIndex idx = m_proxy->mapToSource(proxyIdx);
        const QString sel = idx.isValid() ? m_model->filePath(idx) : QString();
        if (k->key() == Qt::Key_F2 && !sel.isEmpty()) {
            renamePath(sel, QString());
            return true;
        }
        if ((k->key() == Qt::Key_Delete || k->key() == Qt::Key_Backspace) && !sel.isEmpty()) {
            removePath(sel);
            return true;
        }
        if (k->key() == Qt::Key_F5) {
            m_proxy->refresh();
            return true;
        }
    }
    return QWidget::eventFilter(o, e);
}

QString ExplorerPanel::targetDir(const QModelIndex& proxyIdx) const {    QModelIndex idx = m_proxy->mapToSource(proxyIdx);
    if (!idx.isValid()) return m_root;
    QString p = m_model->filePath(idx);
    return m_model->isDir(idx) ? p : QFileInfo(p).absolutePath();
}

void ExplorerPanel::onContextMenu(const QPoint& pos) {
    QModelIndex proxyIdx = m_tree->indexAt(pos);
    QModelIndex idx = m_proxy->mapToSource(proxyIdx);
    QString sel = idx.isValid() ? m_model->filePath(idx) : QString();
    QString base = targetDir(proxyIdx);

    QMenu menu(this);
    QAction* nf = menu.addAction(tr("Yeni Dosya"));
    QAction* nd = menu.addAction(tr("Yeni Klasör"));
    menu.addSeparator();
    QAction* rn = sel.isEmpty() ? nullptr : menu.addAction(tr("Yeniden Adlandır"));
    QAction* cm = sel.isEmpty() ? nullptr : menu.addAction(tr("Karşılaştır (git diff)"));
    QAction* del = sel.isEmpty() ? nullptr : menu.addAction(
        m_trash ? tr("Sil (çöp kutusu)") : tr("Sil"));
    menu.addSeparator();
    QAction* refresh = menu.addAction(tr("Yenile"));

    QAction* chosen = menu.exec(m_tree->viewport()->mapToGlobal(pos));
    if (!chosen) return;
    if (chosen == nf) newFile(base);
    else if (chosen == nd) newDir(base);
    else if (chosen == rn && !sel.isEmpty()) renamePath(sel, QString());
    else if (chosen == cm && !sel.isEmpty()) emit compareRequested(sel);
    else if (chosen == del && !sel.isEmpty()) removePath(sel);
    else if (chosen == refresh) m_proxy->refresh();
}

void ExplorerPanel::newFile(const QString& base) {
    QString n = QInputDialog::getText(this, tr("Yeni Dosya"), tr("Ad:"), QLineEdit::Normal, QString(), nullptr);
    if (n.isEmpty()) return;
    QString p = QDir(base.isEmpty() ? m_root : base).absoluteFilePath(n);
    QDir().mkpath(QFileInfo(p).absolutePath());
    QFile f(p);
    if (f.open(QIODevice::Append)) f.close();
    emit fileOpened(p);
}

void ExplorerPanel::newDir(const QString& base) {
    QString n = QInputDialog::getText(this, tr("Yeni Klasör"), tr("Ad:"), QLineEdit::Normal, QString(), nullptr);
    if (n.isEmpty()) return;
    QDir().mkpath(QDir(base.isEmpty() ? m_root : base).absoluteFilePath(n));
}

void ExplorerPanel::removePath(const QString& path) {
    QFileInfo fi(path);
    if (!fi.exists()) return;
    if (m_trash && m_trash->trash(path)) return; // çöp kutusuna taşı
    auto r = QMessageBox::question(this, tr("Sil"),
        tr("\"%1\" kalıcı olarak silinsin mi?").arg(fi.fileName()),
        QMessageBox::Yes | QMessageBox::Cancel);
    if (r != QMessageBox::Yes) return;
    if (fi.isDir()) QDir(path).removeRecursively();
    else QFile::remove(path);
}

bool ExplorerPanel::renamePath(const QString& oldPath, const QString& newName) {
    QFileInfo fi(oldPath);
    QString name = newName.isEmpty()
        ? QInputDialog::getText(this, tr("Yeniden Adlandır"), tr("Yeni ad:"),
                                QLineEdit::Normal, fi.fileName(), nullptr)
        : newName;
    if (name.isEmpty() || name == fi.fileName()) return false;
    QString dst = fi.dir().absoluteFilePath(name);
    if (QFileInfo::exists(dst)) {
        QMessageBox::warning(this, tr("Yeniden Adlandır"), tr("Hedef zaten var."));
        return false;
    }
    return QFile::rename(oldPath, dst);
}
