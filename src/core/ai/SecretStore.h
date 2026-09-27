#pragma once
#include <QJsonObject>
#include <QString>
#include <QList>\n#include <QStringList>

// Stage 35: API anahtarı kasası.
// Öncelik sırası: OS anahtar deposu (libsecret/secret-tool) → 0600 dosya →
// ortam değişkeni (VERSO_AI_KEY_<PROVIDER_ID>).
// Anahtarlar hiçbir zaman QSettings/log/kaza raporuna yazılmaz.
class SecretStore {
public:
    explicit SecretStore(const QString& filePath = QString());

    QString filePath() const { return m_file; }
    bool usesKeyring() const { return m_useKeyring; }
    void setUseKeyring(bool on);
    static bool keyringAvailable();

    // API
    QString get(const QString& providerId) const;
    bool set(const QString& providerId, const QString& key);
    bool remove(const QString& providerId);
    bool has(const QString& providerId) const;
    QStringList providers() const;  // anahtarı olan sağlayıcılar
    void clearAll();
    int count() const { return m_keys.size(); }

    // Ortam değişkeni yedeği
    static QString envKey(const QString& providerId);
    QString effectiveKey(const QString& providerId) const; // kasada yoksa env

    // Gösterim
    static QString maskKey(const QString& key);   // "sk-ab…yz9"
    static QString maskProviderId(const QString& id);

    // Kasa dosyasını oku/yaz (anahtar deposu kullanılmıyorsa)
    bool loadFile();
    bool saveFile() const;

    // --- Stage 40: tutarlılık denetimi ---
    // Tek-yazıcı kuralı: set() etkin arka uca yazar ve diğerindeki eski
    // kopyayı siler. doctor() kalan tutarsızlıkları listeler.
    struct Issue {
        QString providerId;
        QString issue;   // kısa başlık
        QString detail;  // açıklama + öneri
        bool fixable = true;
    };
    QList<Issue> doctor() const;
    // doctor() bulgularını tek hamlede düzelt (etkin arka uç kazanır)
    int repair();

private:
    QString filePathFor(const QString& providerId) const;
    bool keyringLookup(const QString& providerId, QString& out) const;
    bool keyringStore(const QString& providerId, const QString& key);
    bool keyringErase(const QString& providerId);

    QString m_file;
    bool m_useKeyring = true;
    QJsonObject m_keys; // id -> anahtar (yalnız dosya kasası kullanılırken)
};
