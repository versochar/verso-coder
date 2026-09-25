#include "EmptyState.h"
#include "../core/Animator.h"
#include "../core/IconTheme.h"
#include "../core/ThemeManager.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

EmptyState::EmptyState(const QString& iconName, const QString& title, const QString& subtitle,
                       const QString& actionLabel, QWidget* parent)
    : QWidget(parent), m_title(title) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(24, 32, 24, 32);
    lay->setSpacing(10);
    lay->addStretch(1);

    m_icon = new QLabel(this);
    m_icon->setObjectName("emptyIcon");
    m_icon->setAlignment(Qt::AlignHCenter);
    m_icon->setPixmap(IconTheme::pixmap(iconName, ThemeManager::instance().tokens().textDim, 44));
    lay->addWidget(m_icon);

    m_titleLabel = new QLabel(title, this);
    m_titleLabel->setObjectName("emptyTitle");
    m_titleLabel->setAlignment(Qt::AlignHCenter);
    lay->addWidget(m_titleLabel);

    m_sub = new QLabel(subtitle, this);
    m_sub->setObjectName("emptySub");
    m_sub->setAlignment(Qt::AlignHCenter);
    m_sub->setWordWrap(true);
    lay->addWidget(m_sub);

    if (!actionLabel.isEmpty()) {
        m_action = new QPushButton(actionLabel, this);
        m_action->setObjectName("emptyAction");
        m_action->setCursor(Qt::PointingHandCursor);
        connect(m_action, &QPushButton::clicked, this, [this]() {
            Animator::fadeIn(this); // eylem sonrası yumuşak geri dönüş hissi
            emit actionClicked();
        });
        lay->addSpacing(6);
        lay->addWidget(m_action, 0, Qt::AlignHCenter);
    }
    lay->addStretch(2);
}

void EmptyState::setActionLabel(const QString& label) {
    if (m_action) m_action->setText(label);
}

void EmptyState::setActionVisible(bool on) {
    if (m_action) m_action->setVisible(on);
}
