#include "CompletionPopup.h"
#include <QKeyEvent>
#include <QMouseEvent>

CompletionPopup::CompletionPopup(QWidget* parent) : QListWidget(parent) {
    setWindowFlags(Qt::Popup);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumWidth(320);
    setMaximumHeight(260);
    connect(this, &QListWidget::itemActivated, this, [this](QListWidgetItem* it) {
        int r = row(it);
        if (r >= 0 && r < m_items.size()) emit chosen(m_items[r]);
    });
}

void CompletionPopup::showItems(const QList<CompletionItem>& items, const QPoint& globalPos) {
    m_items = items;
    clear();
    for (const CompletionItem& it : items) {
        QString row = QString("%1  %2").arg(CompletionList::kindIcon(it.kind), it.label);
        if (!it.detail.isEmpty()) row += "  —  " + it.detail.left(60);
        auto* li = new QListWidgetItem(row, this);
        li->setToolTip(QString("%1 (%2)").arg(it.label, CompletionList::kindTitle(it.kind)));
    }
    if (!items.isEmpty()) setCurrentRow(0);
    move(globalPos);
    show();
    setFocus();
}

CompletionItem CompletionPopup::currentItem() const {
    int r = currentRow();
    if (r >= 0 && r < m_items.size()) return m_items[r];
    return CompletionItem();
}

void CompletionPopup::selectNext() {
    if (count() > 0) setCurrentRow((currentRow() + 1) % count());
}

void CompletionPopup::selectPrev() {
    if (count() > 0) setCurrentRow((currentRow() - 1 + count()) % count());
}

void CompletionPopup::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Escape) { emit cancelled(); hide(); return; }
    if (e->key() == Qt::Key_Up) { selectPrev(); return; }
    if (e->key() == Qt::Key_Down) { selectNext(); return; }
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter || e->key() == Qt::Key_Tab) {
        emit chosen(currentItem());
        hide();
        return;
    }
    QListWidget::keyPressEvent(e);
}

void CompletionPopup::mouseDoubleClickEvent(QMouseEvent* e) {
    QListWidget::mouseDoubleClickEvent(e);
    emit chosen(currentItem());
    hide();
}
