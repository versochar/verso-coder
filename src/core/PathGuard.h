#pragma once
#include <QString>
#include <QStringList>

// Stage 38: yol güvenliği.
// QDir::cleanPath sembolik bağlantıları ÇÖZMEZ; proje içindeki bir bağlantı
// ("link -> /etc") ile ajan kök dışındaki dosyalara erişebiliyordu. PathGuard
// her yol bileşenini kademeli olarak kanonikleştirir ve kökten çıkan ilk
// segmenti bulup reddeder.
class PathGuard {
public:
    explicit PathGuard(const QString& root);

    QString root() const { return m_root; }
    // Sembolik bağlantılar çözülmüş gerçek kök
    QString canonicalRoot() const { return m_canonRoot; }

    // Yol kök içinde mi? Değilse `why` nedenini Türkçe açıklar.
    bool isInside(const QString& path, QString* why = nullptr) const;

    // Girdi (mutlak veya göreli) → doğrulanmış mutlak yol. Başarısızsa false.
    bool resolve(const QString& input, QString& outAbs, QString* why = nullptr) const;

    // Yolun kök dışına kaçan ilk segmenti (sembolik bağlantı ya da "..")
    QString escapeSegment(const QString& path) const;
    bool hasSymlinkEscape(const QString& path) const;

    // --- saf yardımcılar (test edilebilir) ---
    // Ham girdi kuşkullu mu? (boş, null bayt, aşırı uzun, gömülü kontrol)
    static bool isSuspiciousInput(const QString& raw, QString* why = nullptr);
    // Var olmayan dosyalar için: var olan en yakın atayı kanonikleştirip
    // yaprağı geri ekler ("yol/sıfır/var/olmayan.txt" → güvenli mutlak yol)
    static QString canonicalizeBestEffort(const QString& absPath);
    // Kanonik kök karşılaştırması (sonda eğik çizgi farkını yutar)
    static bool isSameOrInside(const QString& canonicalRoot, const QString& canonicalPath);
    // Bir yolun tüm bileşenlerini listeler (kademeli kanonikleştirme için)
    static QStringList segmentsOf(const QString& cleanAbsPath);
    // Kademeli kanonikleştirme: her önek için canonicalFilePath
    static QString progressiveCanonical(const QString& cleanAbsPath);

private:
    QString m_root;
    QString m_canonRoot;
};
