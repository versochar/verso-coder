#pragma once
#include <QString>

// Stage 12: otomatik tema kararı (saf mantık).
// "system": işletim sistemi açık/koyu tercihini izler.
// "schedule": gündüz (07:00–19:00) / gece temasını saate göre seçer.
class AutoTheme {
public:
    static bool isDayHour(int hour); // 7 <= h < 19
    // mode: "off" | "system" | "schedule". Kapalıysa current döner.
    static QString pick(const QString& mode, bool systemDark, int hour,
                        const QString& dayTheme, const QString& nightTheme,
                        const QString& current);
    // Bir sonraki geçişe kalan süre (sn) — zamanlayıcı için
    static int secsUntilSwitch(int hour, int minute);
};
