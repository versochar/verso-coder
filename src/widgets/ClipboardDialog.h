#pragma once
#include <QDialog>

#include "../core/ClipboardRing.h"

// Stage 30: pano yöneticisi — halka + sabitler; seçileni yapıştırır.
class ClipboardDialog : public QDialog {
    Q_OBJECT
public:
    explicit ClipboardDialog(ClipboardRing* ring, QWidget* parent = nullptr);

signals:
    void pasteRequested(const QString& text);

private slots:
    void refresh();
    void togglePin();
    void clearRing();

private:
    ClipboardRing* m_ring;
    class QListWidget* m_list = nullptr;
};
