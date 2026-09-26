#pragma once
#include <QDialog>

#include "../core/BookmarkStore.h"

class QTreeWidget;

// Stage 28: yer imleri — liste + atlama + silme.
class BookmarkDialog : public QDialog {
    Q_OBJECT
public:
    explicit BookmarkDialog(BookmarkStore* store, QWidget* parent = nullptr);
    void refresh();

signals:
    void jumpRequested(const QString& file, int line);

private:
    BookmarkStore* m_store;
    QTreeWidget* m_list = nullptr;
};
