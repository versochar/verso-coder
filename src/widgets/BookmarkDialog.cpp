#include "BookmarkDialog.h"
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QHeaderView>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

BookmarkDialog::BookmarkDialog(BookmarkStore* store, QWidget* parent)
    : QDialog(parent), m_store(store) {
    setWindowTitle("Yer İmleri");
    resize(520, 400);
    auto* lay = new QVBoxLayout(this);
    m_list = new QTreeWidget(this);
    m_list->setHeaderLabels({"Dosya", "Satır"});
    m_list->setRootIsDecorated(false);
    m_list->header()->setStretchLastSection(false);
    m_list->setColumnWidth(0, 380);
    lay->addWidget(m_list, 1);
    auto* row = new QHBoxLayout();
    auto* bGo = new QPushButton("Git", this);
    auto* bDel = new QPushButton("Sil", this);
    row->addStretch(1);
    row->addWidget(bGo);
    row->addWidget(bDel);
    lay->addLayout(row);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(box, &QDialogButtonBox::rejected, this, &BookmarkDialog::reject);
    lay->addWidget(box);
    auto jump = [this]() {
        auto* it = m_list->currentItem();
        if (!it) return;
        emit jumpRequested(it->data(0, Qt::UserRole).toString(),
                           it->data(0, Qt::UserRole + 1).toInt());
    };
    connect(bGo, &QPushButton::clicked, this, jump);
    connect(m_list, &QTreeWidget::itemDoubleClicked, this, jump);
    connect(bDel, &QPushButton::clicked, this, [this]() {
        auto* it = m_list->currentItem();
        if (!it || !m_store) return;
        m_store->remove(it->data(0, Qt::UserRole).toString(),
                        it->data(0, Qt::UserRole + 1).toInt());
        m_store->save();
        refresh();
    });
    refresh();
}

void BookmarkDialog::refresh() {
    m_list->clear();
    if (!m_store) return;
    for (const QString& f : m_store->files()) {
        for (int ln : m_store->lines(f)) {
            auto* it = new QTreeWidgetItem(
                m_list, {QFileInfo(f).fileName(), QString::number(ln)});
            it->setToolTip(0, f);
            it->setData(0, Qt::UserRole, f);
            it->setData(0, Qt::UserRole + 1, ln);
        }
    }
}
