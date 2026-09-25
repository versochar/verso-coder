#include "RemoteExplorer.h"
#include "../core/SshSession.h"
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QToolBar>
#include <QVBoxLayout>

RemoteExplorer::RemoteExplorer(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    auto* bar = new QToolBar(this);
    bar->addAction("Yenile", this, &RemoteExplorer::refresh);
    bar->addAction("Üst", this, &RemoteExplorer::goUp);
    bar->addAction("Klasör+", this, &RemoteExplorer::newFolder);
    bar->addAction("Dosya+", this, &RemoteExplorer::newFile);
    lay->addWidget(bar);
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabels({"Ad", "Boyut", "Değişme"});
    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    lay->addWidget(m_tree, 1);
    connect(m_tree, &QTreeWidget::itemActivated, this, &RemoteExplorer::onItemActivated);
    connect(m_tree, &QTreeWidget::customContextMenuRequested, this,
            &RemoteExplorer::onContextMenu);
}

void RemoteExplorer::setSession(SshSession* s, const ConnectionProfile& p) {
    m_session = s;
    m_profile = p;
    m_cur = p.remoteRoot.trimmed().isEmpty() ? "/" : p.remoteRoot.trimmed();
    refresh();
}

void RemoteExplorer::refresh() {
    m_tree->clear();
    if (!m_session) {
        auto* it = new QTreeWidgetItem(m_tree, {"(bağlı değil — Uzak → Bağlan)"});
        it->setDisabled(true);
        return;
    }
    auto r = m_session->exec(RemoteFileSystem::lsCommand(m_cur));
    if (r.exit != 0) {
        emit statusMessage("Uzak listeleme hatası: " + r.err.left(120));
        return;
    }
    for (const RemoteEntry& e : RemoteFileSystem::parseLs(m_cur, r.out)) {
        auto* it = new QTreeWidgetItem(m_tree);
        it->setText(0, e.isDir ? ("📁 " + e.name) : e.name);
        it->setText(1, e.isDir ? "" : QString::number(e.size));
        it->setText(2, e.mtime.isValid() ? e.mtime.toString("dd.MM HH:mm") : "");
        it->setData(0, Qt::UserRole, e.path);
        it->setData(0, Qt::UserRole + 1, e.isDir);
    }
    emit statusMessage("Uzak: " + m_cur);
}

void RemoteExplorer::goUp() {
    if (!m_session) return;
    m_cur = RemoteFileSystem::parent(m_cur);
    refresh();
}

void RemoteExplorer::listDir(const QString& dir, QTreeWidgetItem* parent) {
    Q_UNUSED(parent);
    m_cur = dir;
    refresh();
}

QString RemoteExplorer::selectedPath() const {
    auto* it = m_tree->currentItem();
    return it ? it->data(0, Qt::UserRole).toString() : QString();
}

void RemoteExplorer::onItemActivated(QTreeWidgetItem* it, int col) {
    Q_UNUSED(col);
    if (!it || !m_session) return;
    const QString path = it->data(0, Qt::UserRole).toString();
    if (it->data(0, Qt::UserRole + 1).toBool()) { listDir(path, nullptr); return; }
    emit fileOpenRequested(path);
}

void RemoteExplorer::onContextMenu(const QPoint& pos) {
    auto* it = m_tree->itemAt(pos);
    QMenu m(this);
    if (it && it->data(0, Qt::UserRole + 1).toBool())
        m.addAction("Aç", [this, it]() {
            listDir(it->data(0, Qt::UserRole).toString(), nullptr);
        });
    else if (it)
        m.addAction("Editörde aç", [this]() { emit fileOpenRequested(selectedPath()); });
    m.addAction("İndir...", this, &RemoteExplorer::downloadSelected);
    m.addAction("Buraya yükle...", this, &RemoteExplorer::uploadHere);
    m.addAction("Sil", this, &RemoteExplorer::removeSelected);
    m.exec(m_tree->viewport()->mapToGlobal(pos));
}

void RemoteExplorer::newFolder() {
    if (!m_session) return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Yeni Klasör", "Ad:", QLineEdit::Normal,
                                              QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    auto r = m_session->exec(RemoteFileSystem::mkdirCommand(
        RemoteFileSystem::join(m_cur, name.trimmed())));
    if (r.exit != 0) emit statusMessage("mkdir hatası: " + r.err.left(120));
    refresh();
}

void RemoteExplorer::newFile() {
    if (!m_session) return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Yeni Dosya", "Ad:", QLineEdit::Normal,
                                              QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    const QString path = RemoteFileSystem::join(m_cur, name.trimmed());
    auto r = m_session->writeFile(path, "", 15000);
    if (r.exit != 0) emit statusMessage("Dosya hatası: " + r.err.left(120));
    else emit fileOpenRequested(path);
    refresh();
}

void RemoteExplorer::removeSelected() {
    const QString path = selectedPath();
    if (path.isEmpty() || !m_session) return;
    auto r = QMessageBox::question(this, "Sil", path + "\nuzaktan silinsin mi?");
    if (r != QMessageBox::Yes) return;
    auto e = m_session->exec("rm -rf " + RemoteFileSystem::quote(path));
    if (e.exit != 0) emit statusMessage("Silme hatası: " + e.err.left(120));
    refresh();
}

void RemoteExplorer::uploadHere() {
    if (!m_session) return;
    const QString local = QFileDialog::getOpenFileName(this, "Yükle");
    if (local.isEmpty()) return;
    const QString remote = RemoteFileSystem::join(
        m_cur, RemoteFileSystem::fileName(local));
    emit statusMessage("Yükleniyor: " + local);
    if (!m_session->upload(local, remote)) emit statusMessage("Yükleme başarısız.");
    else emit statusMessage("Yüklendi: " + remote);
    refresh();
}

void RemoteExplorer::downloadSelected() {
    const QString path = selectedPath();
    if (path.isEmpty() || !m_session) return;
    const QString local = QFileDialog::getSaveFileName(
        this, "İndir", RemoteFileSystem::fileName(path));
    if (local.isEmpty()) return;
    if (!m_session->download(path, local)) emit statusMessage("İndirme başarısız.");
    else emit statusMessage("İndirildi: " + local);
}
