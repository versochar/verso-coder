#pragma once
#include <QString>
#include <QStringList>

// Stage 12: arayüz yoğunluğu — Kompakt / Rahat / Geniş.
// UiMetrics ölçeği + UI font farkını tek yerden üretir (saf mantık).
class Density {
public:
    enum class Level { Compact, Comfortable, Spacious };

    static Level fromName(const QString& name); // bilinmeyen → Comfortable
    static QString name(Level l);
    static QStringList names();
    static QString title(const QString& name); // "Kompakt" / ...

    static double metricsScale(Level l); // 0.9 / 1.0 / 1.15
    static int fontDelta(Level l);       // -1 / 0 / +1 (uiSize'a eklenir)
};
