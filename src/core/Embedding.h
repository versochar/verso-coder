#pragma once
#include <QByteArray>
#include <QList>

// Stage 33: gömme (embedding) vektör matematiği — saf, test edilebilir.
namespace Embedding {

// float listesi → little-endian bayt dizisi (kalıcı önbellek için).
QByteArray serialize(const QList<float>& v);
QList<float> deserialize(const QByteArray& b);

// Kosinüs benzerliği (-1..1); boyut uyuşmazsa veya sıfır vektörde 0.
double cosine(const QList<float>& a, const QList<float>& b);

// L2 normalizasyonu.
QList<float> normalize(const QList<float>& v);

// Birkaç vektörün ortalaması (parça merkezleri).
QList<float> average(const QList<QList<float>>& vs);

// Hepsi aynı boyutta mı?
bool sameDim(const QList<QList<float>>& vs);

} // namespace Embedding
