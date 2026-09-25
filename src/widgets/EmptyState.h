#pragma once
#include <QWidget>

class QLabel;
class QPushButton;

// Stage 10: yeniden kullanılabilir boş durum görünümü — ikon + başlık +
// açıklama + isteğe bağlı eylem butonu. Panellerin "içerik yok" halini güzelleştirir.
class EmptyState : public QWidget {
    Q_OBJECT
public:
    EmptyState(const QString& iconName, const QString& title, const QString& subtitle,
               const QString& actionLabel = QString(), QWidget* parent = nullptr);

    void setActionLabel(const QString& label);
    void setActionVisible(bool on);
    QString title() const { return m_title; }

signals:
    void actionClicked();

private:
    QString m_title;
    QLabel* m_icon = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_sub = nullptr;
    QPushButton* m_action = nullptr;
};
