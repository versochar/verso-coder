#include "UiMetrics.h"
#include <QtGlobal>

static double g_scale = 1.0;

double UiMetrics::scale() { return g_scale; }
void UiMetrics::setScale(double s) { g_scale = qBound(0.75, s, 2.0); }

int UiMetrics::spacing(int unit) {
    static const int base[] = {4, 8, 12, 16};
    const int i = qBound(0, unit - 1, 3);
    return int(base[i] * g_scale + 0.5);
}

int UiMetrics::radiusSm()      { return int(4 * g_scale + 0.5); }
int UiMetrics::radius()        { return int(6 * g_scale + 0.5); }
int UiMetrics::radiusLg()      { return int(10 * g_scale + 0.5); }
int UiMetrics::focusRingWidth(){ return 2; }
int UiMetrics::tabBarUnderline(){ return 2; }
int UiMetrics::iconSize()      { return int(18 * g_scale + 0.5); }
int UiMetrics::activityButton(){ return int(40 * g_scale + 0.5); }

void UiMetrics::applyTo(QCoreApplication&) {}
