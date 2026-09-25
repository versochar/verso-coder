#pragma once
#include <QString>
#include <QStringList>

// Stage 12: çoklu görünüm profili deposu.
// UiProfile JSON'larını isimli olarak saklar (QSettings "uiProfiles" grubu).
class ProfileStore {
public:
    static QStringList names();
    static bool save(const QString& name, const QString& json);
    static QString load(const QString& name);
    static bool remove(const QString& name);
    static bool exists(const QString& name);
    static QString sanitize(const QString& raw); // dosya/anahtar güvenli ad
};
