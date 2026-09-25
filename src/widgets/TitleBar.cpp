#include "TitleBar.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>

TitleBar::TitleBar(QWidget* window, QWidget* parent)
    : QWidget(parent), m_window(window) {
    setObjectName("titleBar");
    setFixedHeight(32);
    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(12, 0, 4, 0);
    lay->setSpacing(4);

    m_title = new QLabel("Verso Coder", this);
    m_title->setObjectName("titleLabel");
    lay->addWidget(m_title, 1);

    auto mkBtn = [this](const QString& glyph, const QString& tip) {
        auto* b = new QPushButton(glyph, this);
        b->setObjectName("titleBtn");
        b->setToolTip(tip);
        b->setFixedSize(40, 26);
        b->setFocusPolicy(Qt::NoFocus);
        return b;
    };
    auto* bMin = mkBtn("─", "Simge durumuna küçült");
    m_max = mkBtn("▢", "Büyüt / geri yükle");
    auto* bClose = mkBtn("✕", "Kapat");
    bClose->setObjectName("titleCloseBtn");
    lay->addWidget(bMin);
    lay->addWidget(m_max);
    lay->addWidget(bClose);

    connect(bMin, &QPushButton::clicked, this, &TitleBar::minimizeRequested);
    connect(m_max, &QPushButton::clicked, this, &TitleBar::maximizeRequested);
    connect(bClose, &QPushButton::clicked, this, &TitleBar::closeRequested);
}

void TitleBar::setTitle(const QString& t) {
    m_title->setText(t.isEmpty() ? "Verso Coder" : t);
}

void TitleBar::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && m_window && !m_window->isMaximized()) {
        m_dragging = true;
        m_dragPos = e->globalPosition().toPoint() - m_window->frameGeometry().topLeft();
        e->accept();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent* e) {
    if (m_dragging && m_window && (e->buttons() & Qt::LeftButton)) {
        m_window->move(e->globalPosition().toPoint() - m_dragPos);
        e->accept();
    }
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        m_dragging = false;
        emit maximizeRequested();
    }
}
