#pragma once
#include "ThemeTokens.h"
#include <QStringList>

// Stage 9: tema deposu — gömülü JSON paletleri (resources/themes/*.json)
// + kullanıcı tarafından içe aktarılan temalar (AppData/themes).
class ThemeStore {
public:
    static ThemeStore& instance();

    QStringList builtinNames() const;
    QStringList customNames() const;
    QStringList themeNames() const;                 // gömülü + özel
    bool hasTheme(const QString& name) const;
    ThemeTokens theme(const QString& name) const;   // yoksa dark varsayılanı
    bool isValidTheme(const QString& name) const;   // JSON gerçekten ayrıştırılabiliyor mu

    bool importTheme(const QString& srcJsonPath, QString* importedName = nullptr);
    bool exportTheme(const QString& name, const QString& dstPath) const;

    static QString builtinThemesDir();   // binary yanındaki resources/themes
    static QString customThemesDir();    // AppData/themes

private:
    ThemeStore() = default;
    static QString findBuiltinFile(const QString& name);
    static QString customFile(const QString& name);
};
