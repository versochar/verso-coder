#pragma once
#include <QDialog>

// Stage 22: kısayol hile sayfası — tüm komutlar + etkin tuşlar (salt okunur).
class ShortcutDialog : public QDialog {
    Q_OBJECT
public:
    explicit ShortcutDialog(QWidget* parent = nullptr);
};
