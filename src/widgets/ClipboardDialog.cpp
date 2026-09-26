#include "ClipboardDialog.h"
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

ClipboardDialog::ClipboardDialog(ClipboardRing* ring, QWidget* parent)
    : QDialog(parent), m_ring(ring) {
    setWindowTitle("Pano Geçmişi");
    resize(520, 380);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);
    m_list = new QListWidget(this);
    lay->addWidget(m_list, 1);
    auto* row = new QHBoxLayout();
    auto* bPaste = new QPushButton("Yapıştır", this);
    auto* bPin = new QPushButton("Sabitle/Bırak", this);
    auto* bClear = new QPushButton("Temizle", this);
    row->addStretch(1);
    row->addWidget(bPaste);
    row->addWidget(bPin);
    row->addWidget(bClear);
    lay->addLayout(row);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(box, &QDialogButtonBox::rejected, this, &ClipboardDialog::reject);
    lay->addWidget(box);
    connect(bPaste, &QPushButton::clicked, this, [this]() {
        auto* it = m_list->currentItem();
        if (it) emit pasteRequested(it->data(Qt::UserRole).toString());
    });
    connect(m_list, &QListWidget::itemDoubleClicked, bPaste, &QPushButton::click);
    connect(bPin, &QPushButton::clicked, this, &ClipboardDialog::togglePin);
    connect(bClear, &QPushButton::clicked, this, &ClipboardDialog::clearRing);
    refresh();
}

void ClipboardDialog::refresh() {
    m_list->clear();
    if (!m_ring) return;
    for (const QString& t : m_ring->items()) {
        const bool pin = m_ring->pinned().contains(t);
        auto* it = new QListWidgetItem(
            (pin ? "📌 " : "") + t.left(100).replace('\n', ' '), m_list);
        it->setData(Qt::UserRole, t);
    }
}

void ClipboardDialog::togglePin() {
    auto* it = m_list->currentItem();
    if (!it || !m_ring) return;
    const QString t = it->data(Qt::UserRole).toString();
    if (m_ring->pinned().contains(t)) m_ring->unpin(t);
    else m_ring->pin(t);
    refresh();
}

void ClipboardDialog::clearRing() {
    if (!m_ring) return;
    m_ring->clear();
    refresh();
}
