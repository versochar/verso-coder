#pragma once
#include "../core/CompletionList.h"
#include <QListWidget>

// Stage 13: imleç altında konumlanan tamamlama listesi.
class CompletionPopup : public QListWidget {
    Q_OBJECT
public:
    explicit CompletionPopup(QWidget* parent = nullptr);
    void showItems(const QList<CompletionItem>& items, const QPoint& globalPos);
    CompletionItem currentItem() const;
    void selectNext();
    void selectPrev();

signals:
    void chosen(const CompletionItem& item);
    void cancelled();

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;

private:
    QList<CompletionItem> m_items;
};
