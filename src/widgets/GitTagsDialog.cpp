#include "GitTagsDialog.h"
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QVBoxLayout>

GitTagsDialog::GitTagsDialog(const QString& workdir, QWidget* parent)
    : QDialog(parent), m_dir(workdir) {
    setWindowTitle("Etiketler");
    resize(420, 380);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);
    m_list = new QListWidget(this);
    lay->addWidget(m_list, 1);
    auto* row = new QHBoxLayout();
    auto* bNew = new QPushButton("Yeni", this);
    auto* bDel = new QPushButton("Sil", this);
    row->addStretch(1);
    row->addWidget(bNew);
    row->addWidget(bDel);
    lay->addLayout(row);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(box, &QDialogButtonBox::rejected, this, &GitTagsDialog::reject);
    lay->addWidget(box);
    connect(bNew, &QPushButton::clicked, this, &GitTagsDialog::createTag);
    connect(bDel, &QPushButton::clicked, this, &GitTagsDialog::deleteTag);
    refreshList();
}

QString GitTagsDialog::runGit(const QStringList& args) const {
    QProcess p;
    p.setWorkingDirectory(m_dir);
    p.start("git", args);
    if (!p.waitForFinished(10000)) return {};
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}

void GitTagsDialog::refreshList() {
    m_list->clear();
    m_list->addItems(runGit({"tag", "-l", "--sort=-creatordate"})
                         .split('\n', Qt::SkipEmptyParts));
}

void GitTagsDialog::createTag() {
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Yeni Etiket", "Ad (örn. v1.2.0):",
                                               QLineEdit::Normal, "v", &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    const QString msg = QInputDialog::getText(this, "Yeni Etiket",
                                              "Açıklama (boşsa hafif etiket):");
    QString err;
    if (msg.trimmed().isEmpty()) {
        QProcess p;
        p.setWorkingDirectory(m_dir);
        p.start("git", {"tag", name.trimmed()});
        p.waitForFinished(10000);
        err = QString::fromUtf8(p.readAllStandardError());
    } else {
        QProcess p;
        p.setWorkingDirectory(m_dir);
        p.start("git", {"tag", "-a", name.trimmed(), "-m", msg.trimmed()});
        p.waitForFinished(10000);
        err = QString::fromUtf8(p.readAllStandardError());
    }
    if (!err.isEmpty()) QMessageBox::warning(this, "Etiket", err.left(300));
    refreshList();
}

void GitTagsDialog::deleteTag() {
    auto* it = m_list->currentItem();
    if (!it) return;
    auto r = QMessageBox::question(this, "Etiket Sil",
                                   it->text() + " silinsin mi? (yerel)",
                                   QMessageBox::Yes | QMessageBox::Cancel);
    if (r != QMessageBox::Yes) return;
    runGit({"tag", "-d", it->text()});
    refreshList();
}
