#pragma once
#include <QWidget>

class QLabel;
class QPushButton;

// Stage 12: özel pencere başlık çubuğu (frameless mod).
// Sürükle-taşı, çift tıkla büyüt, min/maks/kapat düğmeleri.
// Yerel pencere yöneticisinde sorun çıkarsa ayarlardan kapatılabilir.
class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget* window, QWidget* parent = nullptr);
    void setTitle(const QString& t);

signals:
    void minimizeRequested();
    void maximizeRequested();
    void closeRequested();

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;

private:
    QWidget* m_window = nullptr;
    QLabel* m_title = nullptr;
    QPushButton* m_max = nullptr;
    QPoint m_dragPos;
    bool m_dragging = false;
};
