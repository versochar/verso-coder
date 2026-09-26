#include "Embedding.h"
#include <cstring>
#include <cmath>

namespace Embedding {

QByteArray serialize(const QList<float>& v) {
    QByteArray out;
    out.resize(v.size() * int(sizeof(float)));
    for (int i = 0; i < v.size(); ++i)
        std::memcpy(out.data() + i * sizeof(float), &v[i], sizeof(float));
    return out;
}

QList<float> deserialize(const QByteArray& b) {
    QList<float> out;
    if (b.size() % int(sizeof(float)) != 0) return out;
    out.resize(b.size() / int(sizeof(float)));
    for (int i = 0; i < out.size(); ++i)
        std::memcpy(&out[i], b.constData() + i * sizeof(float), sizeof(float));
    return out;
}

double cosine(const QList<float>& a, const QList<float>& b) {
    if (a.size() != b.size() || a.isEmpty()) return 0.0;
    double dot = 0, na = 0, nb = 0;
    for (int i = 0; i < a.size(); ++i) {
        dot += double(a[i]) * b[i];
        na += double(a[i]) * a[i];
        nb += double(b[i]) * b[i];
    }
    if (na <= 0.0 || nb <= 0.0) return 0.0;
    return dot / (std::sqrt(na) * std::sqrt(nb));
}

QList<float> normalize(const QList<float>& v) {
    double n = 0;
    for (float f : v) n += double(f) * f;
    n = std::sqrt(n);
    if (n <= 0.0) return v;
    QList<float> out;
    out.reserve(v.size());
    for (float f : v) out << float(double(f) / n);
    return out;
}

QList<float> average(const QList<QList<float>>& vs) {
    if (vs.isEmpty() || !sameDim(vs)) return {};
    QList<float> out(vs.first().size(), 0.0f);
    for (const QList<float>& v : vs)
        for (int i = 0; i < v.size(); ++i) out[i] += v[i];
    const float n = float(vs.size());
    for (float& f : out) f /= n;
    return out;
}

bool sameDim(const QList<QList<float>>& vs) {
    if (vs.isEmpty()) return true;
    const int d = vs.first().size();
    for (const QList<float>& v : vs)
        if (v.size() != d) return false;
    return true;
}

} // namespace Embedding
