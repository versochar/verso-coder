#pragma once
#include <QByteArray>
#include <QString>

// İşbirliği trafiği şifreleme: ChaCha20 (RFC 8439) + HMAC-SHA256
// (Encrypt-then-MAC). Qt'de simetrik şifre yok; akış şifresi burada,
// MAC için QMessageAuthenticationCode kullanılır.
// Biçim: "v1." + base64(iv12 + şifrelimetin) + "." + base64(mac32)
class CollabCrypt {
public:
    // 32 baytlık anahtar türet (paroladan; deterministik esnetme)
    static QByteArray deriveKey(const QString& password);
    // Ham ChaCha20 (test + iç kullanım): key32 + nonce12 + sayaç
    static QByteArray chacha20(const QByteArray& key, const QByteArray& nonce,
                               quint32 counter, const QByteArray& data);
    static QString seal(const QByteArray& key, const QString& plain,
                        QString* error = nullptr);
    static QString open(const QByteArray& key, const QString& sealed,
                        QString* error = nullptr);
    static bool looksSealed(const QString& text);
};
