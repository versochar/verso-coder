#include "AutoTheme.h"

bool AutoTheme::isDayHour(int hour) {
    return hour >= 7 && hour < 19;
}

QString AutoTheme::pick(const QString& mode, bool systemDark, int hour,
                        const QString& dayTheme, const QString& nightTheme,
                        const QString& current) {
    if (mode == "system")
        return systemDark ? nightTheme : dayTheme;
    if (mode == "schedule")
        return isDayHour(hour) ? dayTheme : nightTheme;
    return current;
}

int AutoTheme::secsUntilSwitch(int hour, int minute) {
    const int nowMin = hour * 60 + minute;
    const int dayStart = 7 * 60;
    const int nightStart = 19 * 60;
    int target;
    if (nowMin < dayStart) target = dayStart;
    else if (nowMin < nightStart) target = nightStart;
    else target = dayStart + 24 * 60;
    return (target - nowMin) * 60;
}
