#pragma once
#include <QCoreApplication>
#include <QString>

// Stage 9: tutarlı arayüz ölçüleri — boşluklar, köşe yarıçapları, odak halkası.
// Tek yerden ölçeklenir (UI zoom Stage 10'da genişletilecek).
class UiMetrics {
public:
    static double scale();             // 1.0 = normal
    static void setScale(double s);

    static int spacing(int unit = 2);  // 1..4 → 4, 8, 12, 16 px (ölçekli)
    static int radiusSm();             // 4
    static int radius();               // 6  (buton/input/kart)
    static int radiusLg();             // 10 (dialog/menü)
    static int focusRingWidth();       // 2
    static int tabBarUnderline();      // 2 (aktif sekme alt çizgisi)
    static int iconSize();             // 18 (aktivite çubuğu vb.)
    static int activityButton();       // 40

    static void applyTo(QCoreApplication&); // (uzatma noktası; şimdilik no-op)
};
