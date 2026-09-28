#include "WorktreeDialog.h"
#include "../core/GitRunner.h"
#include "../core/GitWorktree.h"
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

WorktreeDialog::WorktreeDialog(const QString& root, QWidget* parent)
    : QDialog(parent), m_root(root) {
    setWindowTitle("Çalışma Ağaçları — " + root);
    resize(560, 380);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);
    m_list = new QListWidget(this);
    m_list->setFont(QFont("monospace"));
    lay->addWidget(m_list, 1);
    auto* row = new QHBoxLayout();
    auto* bAdd = new QPushButton("Ekle...", this);
    auto* bDel = new QPushButton("Kaldır", this);
    auto* bPrune = new QPushButton("Buda", this);
    row->addWidget(bAdd);
    row->addWidget(bDel);
    row->addWidget(bPrune);
    lay->addLayout(row);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(box, &QDialogButtonBox::rejected, this, &WorktreeDialog::reject);
    lay->addWidget(box);
    connect(bAdd, &QPushButton::clicked, this, &WorktreeDialog::addWorktree);
    connect(bDel, &QPushButton::clicked, this, &WorktreeDialog::removeSelected);
    connect(bPrune, &QPushButton::clicked, this, [this]() {
        LocalGitRunner g(m_root);
        QString err;
        if (!GitWorktree::prune(g, &err))
            QMessageBox::warning(this, "Buda", err.isEmpty() ? "Budanamadı" : err);
        refreshList();
    });
    refreshList();
}

void WorktreeDialog::refreshList() {
    m_list->clear();
    LocalGitRunner g(m_root);
    QString err;
    const auto list = GitWorktree::list(g, &err);
    if (!err.isEmpty()) {
        m_list->addItem("Hata: " + err);
        return;
    }
    for (const auto& w : list) {
        QString t = w.path + "  [" + w.branch + "]  " + w.commit;
        if (w.bare) t += "  (çıplak)";
        if (w.locked) t += "  (kilitli)";
        if (w.prunable) t += "  (budanabilir)";
        auto* it = new QListWidgetItem(t, m_list);
        it->setData(Qt::UserRole, w.path);
    }
    if (m_list->count() == 0) m_list->addItem("(ağaç yok)");
}

void WorktreeDialog::addWorktree() {
    bool ok = false;
    const QString sug = QDir(m_root).absoluteFilePath("../" + QDir(m_root).dirName() + "-w1");
    const QString path = QInputDialog::getText(this, "Ekle", "Dizin:", QLineEdit::Normal,
                                               sug, &ok);
    if (!ok || path.trimmed().isEmpty()) return;
    const QString rev = QInputDialog::getText(this, "Ekle", "Rev (boş=HEAD):",
                                              QLineEdit::Normal, QString(), &ok);
    if (!ok) return;
    const QString dal =
        QInputDialog::getText(this, "Ekle", "Yeni dal (boş=mevcut):", QLineEdit::Normal,
                              QString(), &ok);
    if (!ok) return;
    LocalGitRunner g(m_root);
    QString err;
    if (!GitWorktree::add(g, path.trimmed(), rev.trimmed(), dal.trimmed(), &err)) {
        QMessageBox::warning(this, "Ekle", err.isEmpty() ? "Eklenemedi" : err);
        return;
    }
    refreshList();
}

void WorktreeDialog::removeSelected() {
    auto* it = m_list->currentItem();
    const QString path = it ? it->data(Qt::UserRole).toString() : QString();
    if (path.isEmpty()) return;
    auto r = QMessageBox::question(this, "Kaldır",
                                   path + "\nKaldırılsın mı? (dizin silinir)",
                                   QMessageBox::Yes | QMessageBox::Cancel);
    if (r != QMessageBox::Yes) return;
    LocalGitRunner g(m_root);
    QString err;
    if (!GitWorktree::remove(g, path, false, &err)) {
        auto f = QMessageBox::question(this, "Kaldır",
                                       "Temiz değil:\n" + err + "\nZorla kaldırılsın mı?",
                                       QMessageBox::Yes | QMessageBox::Cancel);
        if (f != QMessageBox::Yes) return;
        if (!GitWorktree::remove(g, path, true, &err)) {
            QMessageBox::warning(this, "Kaldır", err.isEmpty() ? "Kaldırılamadı" : err);
            return;
        }
    }
    refreshList();
}
