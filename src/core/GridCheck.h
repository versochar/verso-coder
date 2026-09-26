#pragma once
#include <QFont>
#include <QString>

// Stage 23: piksel-hizalı ızgara garantisi — editör fontu gerçek monospace mi?
// Kriter: basılabilir ASCII'nin (32..126) her harfinin advance'ı eşit olmalı
// (örn. alt alta "iere"/"teat" yazıldığında her harf aynı pikselde başlar).
// Saf mantık (test edilebilir).
class GridCheck {
public:
    static bool isMonospace(const QFont& f);
    static int cellWidth(const QFont& f);      // -1: monospace değil
    static int columnX(const QFont& f, int col); // sütun başlangıç pikseli
    static QString systemMonospace(); // "DejaVu Sans Mono" vb. ilk bulunan
};
