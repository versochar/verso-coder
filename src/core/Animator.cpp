#include "Animator.h"
#include <QGraphicsOpacityEffect>
#include <QPointer>
#include <QPropertyAnimation>
#include <QRect>
#include <QWidget>

static bool g_reducedMotion = false;

bool Animator::reducedMotion() { return g_reducedMotion; }
void Animator::setReducedMotion(bool on) { g_reducedMotion = on; }

// Widget üzerindeki opaklık efektini döndürür (yoksa oluşturur).
QGraphicsOpacityEffect* Animator::opacityEffect(QWidget* w) {
    if (!w) return nullptr;
    auto* eff = qobject_cast<QGraphicsOpacityEffect*>(w->graphicsEffect());
    if (eff) return eff;
    // Başka bir efekt varsa (drop shadow vb.) dokunma: üstüne yeni efekt
    // eklemek görsel olarak bozar.
    if (w->graphicsEffect()) return nullptr;
    eff = new QGraphicsOpacityEffect(w);
    w->setGraphicsEffect(eff);
    return eff;
}

void Animator::setOpacity(QWidget* w, qreal value) {
    auto* eff = opacityEffect(w);
    if (eff) eff->setOpacity(value);
}

void Animator::runProperty(QWidget* w, const QByteArray& prop, const QVariant& from,
                           const QVariant& to, int ms, std::function<void()> finished) {
    if (!w) {
        if (finished) finished();
        return;
    }
    // "opacity" widget'in değil GÖRSEL EFEKTİN özelliğidir; widget üzerinde
    // hedeflenirse Qt "non-existing property" uyarısı verir ve animasyon
    // hiç çalışmaz.
    QObject* target = w;
    if (prop == "opacity") {
        target = opacityEffect(w);
        if (!target) {
            if (finished) finished();
            return;
        }
    } else if (w->metaObject()->indexOfProperty(prop.constData()) < 0) {
        // Var olmayan özellik: sessizce atla, Qt uyarısı üretme
        if (finished) finished();
        return;
    }
    auto* anim = new QPropertyAnimation(target, prop, w);
    anim->setDuration(Animator::reducedMotion() ? 0 : qMax(0, ms));
    anim->setStartValue(from);
    anim->setEndValue(to);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    if (finished)
        QObject::connect(anim, &QPropertyAnimation::finished, w, [finished]() { finished(); });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void Animator::fadeIn(QWidget* w, int ms) {
    if (!w) return;
    w->show();
    setOpacity(w, 0.0);
    runProperty(w, "opacity", 0.0, 1.0, ms);
}

void Animator::fadeOut(QWidget* w, int ms, std::function<void()> finished) {
    if (!w) return;
    if (reducedMotion()) {
        w->hide();
        if (finished) finished();
        return;
    }
    QPointer<QWidget> guard(w);
    setOpacity(w, 1.0);
    runProperty(w, "opacity", 1.0, 0.0, ms, [guard, finished]() {
        if (guard) guard->hide();
        if (finished) finished();
    });
}

void Animator::slideDownIn(QWidget* w, int ms) {
    if (!w) return;
    w->show();
    if (reducedMotion()) return;
    setOpacity(w, 0.0);
    runProperty(w, "opacity", 0.0, 1.0, ms);
    // kısa yukarıdan-aşağı kayma: maksimum yüksekliği küçük tut
    QRect g = w->geometry();
    QRect from = g.translated(0, qMin(24, g.height() / 2));
    w->setGeometry(from);
    runProperty(w, "geometry", from, g, ms);
}

void Animator::slideUpOut(QWidget* w, int ms, std::function<void()> finished) {
    fadeOut(w, ms, finished);
}

void Animator::animateGeometry(QWidget* w, const QRect& target, int ms) {
    if (!w) return;
    if (reducedMotion()) {
        w->setGeometry(target);
        return;
    }
    runProperty(w, "geometry", w->geometry(), target, ms);
}

void Animator::setVisibleAnimated(QWidget* w, bool visible, int ms) {
    if (!w) return;
    if (visible) fadeIn(w, ms);
    else fadeOut(w, ms);
}
