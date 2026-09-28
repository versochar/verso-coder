#include "CollabCrypt.h"
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QStringList>
#include <QtEndian>

namespace {
// RFC 8439 §2.1: çeyrek tur
inline void qr(quint32& a, quint32& b, quint32& c, quint32& d) {
    a += b; d ^= a; d = (d << 16) | (d >> 16);
    c += d; b ^= c; b = (b << 12) | (b >> 20);
    a += b; d ^= a; d = (d << 8) | (d >> 24);
    c += d; b ^= c; b = (b << 7) | (b >> 25);
}

inline quint32 le32(const uchar* p) {
    return quint32(p[0]) | (quint32(p[1]) << 8) | (quint32(p[2]) << 16) |
           (quint32(p[3]) << 24);
}

inline void storeLe32(uchar* p, quint32 v) {
    p[0] = uchar(v);
    p[1] = uchar(v >> 8);
    p[2] = uchar(v >> 16);
    p[3] = uchar(v >> 24);
}

// RFC 8439 §2.3: bir blok (64 bayt) anahtar akışı üret
void block(const quint32 in[16], uchar out[64]) {
    quint32 x[16];
    for (int i = 0; i < 16; ++i) x[i] = in[i];
    for (int i = 0; i < 10; ++i) {
        qr(x[0], x[4], x[8], x[12]); // sütun turu
        qr(x[1], x[5], x[9], x[13]);
        qr(x[2], x[6], x[10], x[14]);
        qr(x[3], x[7], x[11], x[15]);
        qr(x[0], x[5], x[10], x[15]); // köşegen turu
        qr(x[1], x[6], x[11], x[12]);
        qr(x[2], x[7], x[8], x[13]);
        qr(x[3], x[4], x[9], x[14]);
    }
    for (int i = 0; i < 16; ++i) storeLe32(out + 4 * i, x[i] + in[i]);
}
} // namespace

QByteArray CollabCrypt::chacha20(const QByteArray& key, const QByteArray& nonce,
                                 quint32 counter, const QByteArray& data) {
    if (key.size() != 32 || nonce.size() != 12) return {};
    quint32 st[16] = {0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
                      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uchar* k = reinterpret_cast<const uchar*>(key.constData());
    const uchar* n = reinterpret_cast<const uchar*>(nonce.constData());
    for (int i = 0; i < 8; ++i) st[4 + i] = le32(k + 4 * i);
    st[12] = counter;
    for (int i = 0; i < 3; ++i) st[13 + i] = le32(n + 4 * i);
    QByteArray out;
    out.resize(data.size());
    uchar ks[64];
    const uchar* in = reinterpret_cast<const uchar*>(data.constData());
    uchar* dst = reinterpret_cast<uchar*>(out.data());
    int pos = 0;
    quint32 ctr = counter;
    while (pos < data.size()) {
        st[12] = ctr++;
        block(st, ks);
        const int n = qMin(64, data.size() - pos);
        for (int i = 0; i < n; ++i) dst[pos + i] = in[pos + i] ^ ks[i];
        pos += n;
    }
    return out;
}

QByteArray CollabCrypt::deriveKey(const QString& password) {
    QByteArray h = QCryptographicHash::hash("verso-collab-v1:" + password.toUtf8(),
                                            QCryptographicHash::Sha256);
    for (int i = 0; i < 999; ++i)
        h = QCryptographicHash::hash(h + password.toUtf8(), QCryptographicHash::Sha256);
    return h;
}

static QByteArray macKey(const QByteArray& key) {
    return QCryptographicHash::hash(key + "mac", QCryptographicHash::Sha256);
}

QString CollabCrypt::seal(const QByteArray& key, const QString& plain,
                          QString* error) {
    if (key.size() != 32) {
        if (error) *error = "anahtar 32 bayt olmalı";
        return {};
    }
    QByteArray iv(12, 0);
    QRandomGenerator::global()->fillRange(reinterpret_cast<quint32*>(iv.data()), 3);
    const QByteArray ct =
        chacha20(key, iv, 1, plain.toUtf8()); // sayaç 1 (0 ayrık)
    const QByteArray body = iv + ct;
    const QByteArray mac = QMessageAuthenticationCode::hash(
        body, macKey(key), QCryptographicHash::Sha256);
    return "v1." + QString::fromLatin1(body.toBase64()) + "." +
           QString::fromLatin1(mac.toBase64());
}

QString CollabCrypt::open(const QByteArray& key, const QString& sealed,
                          QString* error) {
    auto fail = [&](const char* m) {
        if (error) *error = m;
        return QString();
    };
    if (key.size() != 32) return fail("anahtar 32 bayt olmalı");
    if (!looksSealed(sealed)) return fail("zarf değil");
    const QStringList p = sealed.split('.');
    if (p.size() != 3) return fail("zarf bozuk");
    const QByteArray body = QByteArray::fromBase64(p[1].toLatin1());
    const QByteArray mac = QByteArray::fromBase64(p[2].toLatin1());
    if (body.size() < 12 || mac.size() != 32) return fail("zarf bozuk");
    const QByteArray beklenen = QMessageAuthenticationCode::hash(
        body, macKey(key), QCryptographicHash::Sha256);
    if (beklenen != mac) return fail("MAC uymadı (yanlış anahtar ya da oynama)");
    const QByteArray iv = body.left(12);
    const QByteArray ct = body.mid(12);
    return QString::fromUtf8(chacha20(key, iv, 1, ct));
}

bool CollabCrypt::looksSealed(const QString& text) {
    return text.startsWith("v1.");
}
