#pragma once
#include <QList>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

// Stage 17: değiştirme motoru derinleştirme (saf) — hariç tutma,
// dosya başına önizleme, büyük/küçük harf koruma.
struct ReplaceEdit {
    QString file;
    int line1 = 1;
    QString before; // eşleşen satır (kırpılmış)
    QString after;  // değiştirilmiş satır (kırpılmış)
};

class ReplaceEngine {
public:
    // Dahil (*.cpp) + hariç (!test*, *build*) glob listesi
    static bool fileAllowed(const QString& fileName, const QString& includeFilter,
                            const QString& excludeFilter);
    static bool globListMatch(const QString& str, const QString& spaceSeparated);
    // Dosya içeriğine uygula → (yeni metin, düzenlenen satırlar)
    static QPair<QString, QList<ReplaceEdit>> applyFile(
        const QString& file, const QString& text, const QString& pattern,
        const QString& replacement, bool useRegex, bool caseSens);
    // Değiştirmede büyük/küçük harf koru ("Foo" → "Bar" ise "foo" → "bar")
    static QString preserveCase(const QString& matched, const QString& replacement);
};
