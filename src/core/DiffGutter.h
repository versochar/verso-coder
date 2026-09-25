#pragma once
#include <QMap>
#include <QString>

// Stage 11: zengin gutter için git diff satır durumları (saf çözümleyici).
// `git diff -U0` çıktısından yeni-dosya satır numaralarını üretir:
// 'a' eklenen, 'm' değişen, 'd' silinen (işaret, silinen bloğun üst satırındadır).
class DiffGutter {
public:
    static QMap<int, char> changedLines(const QString& diffText); // 1-based satır → durum
};
