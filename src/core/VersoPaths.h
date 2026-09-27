#pragma once
#include <QString>

// Kurulum yolu çözümleyici: geliştirme ağacı, binary yanı ve sistem
// kurulum dizini (cmake --install) sırasıyla denenir.
class VersoPaths {
public:
    // Var olan ilk kaynak kökü (mutlak yol ya da "resources/...")
    static QString resourceDir();
    // Alt dizin (örn. "themes", "icons", "plugins")
    static QString subDir(const QString& sub);
};
