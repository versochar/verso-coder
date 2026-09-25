#include "Density.h"

Density::Level Density::fromName(const QString& name) {
    if (name == "compact") return Level::Compact;
    if (name == "spacious") return Level::Spacious;
    return Level::Comfortable;
}

QString Density::name(Level l) {
    if (l == Level::Compact) return "compact";
    if (l == Level::Spacious) return "spacious";
    return "comfortable";
}

QStringList Density::names() {
    return {"compact", "comfortable", "spacious"};
}

QString Density::title(const QString& name) {
    if (name == "compact") return "Kompakt";
    if (name == "spacious") return "Geniş";
    return "Rahat";
}

double Density::metricsScale(Level l) {
    if (l == Level::Compact) return 0.9;
    if (l == Level::Spacious) return 1.15;
    return 1.0;
}

int Density::fontDelta(Level l) {
    if (l == Level::Compact) return -1;
    if (l == Level::Spacious) return +1;
    return 0;
}
