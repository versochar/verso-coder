#pragma once
#include "LspClient.h" // LspDiag
#include <QList>
#include <QString>

// Stage 14: problem eşleştiriciler — derleme/test çıktısı → Sorunlar paneli.
// gcc/clang, python traceback, cmake, genel dosya:satır.
class ProblemMatcher {
public:
    // Tüm eşleştiricileri uygular, yinelenenleri tekilleştirir
    static QList<LspDiag> match(const QString& output, const QString& source = "derleme");
    static QList<LspDiag> matchGcc(const QString& output, const QString& source);
    static QList<LspDiag> matchPython(const QString& output);
    static QList<LspDiag> matchGeneric(const QString& output, const QString& source);
};
