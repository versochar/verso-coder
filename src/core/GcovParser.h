#pragma once
#include <QList>
#include <QString>

// Stage 14: gcov .gcov metin çıktısı çözümleyici (bağımlılıksız).
struct GcovLine {
    int line = 0;   // kaynak satırı (1-based)
    long hits = -1; // -1: çalıştırılamaz satır, 0: kapsanmıyor, >0: çalıştı
};

struct GcovFile {
    QString source; // "Source:..." satırından
    QList<GcovLine> lines;
    int covered() const;
    int coverable() const;
    double percent() const;
};

class GcovParser {
public:
    static GcovFile parse(const QString& gcovText);
    // "9:12:kod" | "#####:12:kod" | "-:" | "=====" biçimleri
    static bool parseLine(const QString& raw, GcovLine& out);
};
