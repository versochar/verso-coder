#include "ToastManager.h"
#include "Animator.h"
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QTimer>
#include <QVBoxLayout>

ToastManager& ToastManager::instance() {
    static ToastManager m;
    return m;
}

ToastManager::ToastManager(QObject* parent) : QObject(parent) {}

QString ToastManager::typeString(ToastType t) {
    switch (t) {
    case ToastType::Success: return "success";
    case ToastType::Warning: return "warning";
    case ToastType::Error:   return "error";
    default:                 return "info";
    }
}

void ToastManager::attach(QWidget* hostWindow) {
    if (!hostWindow) return;
    if (m_host == hostWindow) return;
    if (m_host) m_host->removeEventFilter(this);
    m_host = hostWindow;
    m_host->installEventFilter(this);

    m_overlay = new QWidget(m_host);
    m_overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_overlay->setAttribute(Qt::WA_NoSystemBackground);
    m_lay = new QVBoxLayout(m_overlay);
    m_lay->setContentsMargins(0, 0, 14, 14);
    m_lay->setSpacing(8);
    m_lay->addStretch(1); // alttaki toast'lar sağ-alta yaslanır
    onHostResized();
}

void ToastManager::onHostResized() {
    if (!m_host || !m_overlay) return;
    m_overlay->setGeometry(m_host->rect());
}

int ToastManager::visibleCount() const {
    return m_overlay ? int(m_overlay->findChildren<QFrame*>(QString(), Qt::FindDirectChildrenOnly).size())
                     : 0;
}

void ToastManager::show(ToastType type, const QString& message, int ms) {
    if (!m_host || !m_overlay || message.trimmed().isEmpty()) return;

    // Aynı mesaj görünüyorsa: yenisini ekleme, süresini tazele
    const auto toasts = m_overlay->findChildren<QFrame*>(QString(), Qt::FindDirectChildrenOnly);
    for (QFrame* t : toasts) {
        if (t->property("toastText").toString() == message) {
            QTimer* life = t->findChild<QTimer*>();
            if (life) life->start(ms);
            return;
        }
    }
    // En fazla 4 toast: en eskisini at
    while (toasts.size() >= 4 && toasts.size() > 0) {
        QFrame* oldest = toasts.first();
        oldest->hide();
        oldest->deleteLater();
        break;
    }

    auto* toast = new QFrame(m_overlay);
    toast->setObjectName("toast");
    toast->setProperty("severity", typeString(type));
    toast->setProperty("toastText", message);
    auto* row = new QHBoxLayout(toast);
    row->setContentsMargins(12, 8, 14, 8);
    row->setSpacing(8);
    auto* dot = new QLabel("●", toast);
    dot->setObjectName("toastDot");
    auto* text = new QLabel(message, toast);
    text->setObjectName("toastText");
    text->setWordWrap(true);
    text->setMaximumWidth(340);
    row->addWidget(dot);
    row->addWidget(text, 1);

    auto* life = new QTimer(toast);
    life->setSingleShot(true);
    connect(life, &QTimer::timeout, this, [this, toast]() {
        QPointer<QFrame> guard(toast);
        Animator::fadeOut(toast, 140, [this, guard]() {
            if (!guard) return;
            m_lay->removeWidget(guard);
            guard->deleteLater();
        });
    });
    m_lay->addWidget(toast);
    toast->adjustSize();
    Animator::slideDownIn(toast, 180);
    life->start(ms);
}

void ToastManager::dismissAll() {
    if (!m_overlay) return;
    const auto toasts = m_overlay->findChildren<QFrame*>(QString(), Qt::FindDirectChildrenOnly);
    for (QFrame* t : toasts) {
        m_lay->removeWidget(t);
        t->deleteLater();
    }
}

bool ToastManager::eventFilter(QObject* o, QEvent* e) {
    if (o == m_host && (e->type() == QEvent::Resize || e->type() == QEvent::Show))
        onHostResized();
    return QObject::eventFilter(o, e);
}
