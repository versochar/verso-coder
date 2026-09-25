#pragma once
#include <QColor>
#include <QStringList>

// Stage 9: accent (vurgu) rengi ve türevleri. Tek taban renkten
// hover/pressed/soft durum renkleri otomatik hesaplanır.
class AccentColor {
public:
    static QStringList presetNames();
    static QColor preset(const QString& name);   // bilinmeyen → mavi
    static QColor hover(const QColor& c, bool dark);   // koyu temada açılır
    static QColor pressed(const QColor& c, bool dark); // hover'dan bir tık daha
    static QColor soft(const QColor& c, const QColor& bg, double t = 0.18); // arka planla karışım
};
