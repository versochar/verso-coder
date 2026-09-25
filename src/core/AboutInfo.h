#pragma once
#include <QString>

// Sürüm/hakkında bilgisi + güncelleme manifesti çözümleme.
class AboutInfo {
public:
    static QString version();     // "1.0.0"
    static QString appName();
    static QString buildInfo();   // Qt sürümü, derleyici, tarih
    static QString changelog();
    static QString aboutHtml();

    struct Update {
        QString version;
        QString url;
        QString notes;
        bool valid = false;
    };
    // {"version":"1.1.0","url":"...","notes":"..."}
    static Update parseManifest(const QString& json);
    static bool isNewer(const QString& candidate, const QString& current); // semver karşılaştırma
    static int compareVersions(const QString& a, const QString& b);
};
