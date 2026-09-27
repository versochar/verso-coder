#pragma once
#include <QByteArray>
#include <QVariant>
#include <functional>

class QWidget;
class QRect;
class QGraphicsOpacityEffect;

// Stage 10: animasyon yardımcıları. "Azaltılmış hareket" tercihine saygı duyar:
// etkinse tüm animasyonlar anında (sıçramasız) uygulanır.
class Animator {
public:
    static bool reducedMotion();
    static void setReducedMotion(bool on);

    // Opaklık tabanlı yumuşak görünüm/gizleme (QGraphicsOpacityEffect)
    static void fadeIn(QWidget* w, int ms = 160);
    static void fadeOut(QWidget* w, int ms = 140, std::function<void()> finished = nullptr);

    // Alt kenardan kayarak giriş / üste kayarak çıkış (toast, dock)
    static void slideDownIn(QWidget* w, int ms = 180);
    static void slideUpOut(QWidget* w, int ms = 160, std::function<void()> finished = nullptr);

    // Geometri değişimini yumuşat (panel aç/kapa)
    static void animateGeometry(QWidget* w, const QRect& target, int ms = 200);

    // show/hide + yumuşak geçiş tek çağrıda
    static void setVisibleAnimated(QWidget* w, bool visible, int ms = 160);

private:
    static void setOpacity(QWidget* w, qreal value);
    // Stage 38: opaklık efekti (animasyonun gerçek hedefi)
    static QGraphicsOpacityEffect* opacityEffect(QWidget* w);
    static void runProperty(QWidget* w, const QByteArray& prop, const QVariant& from,
                            const QVariant& to, int ms, std::function<void()> finished = nullptr);
};
