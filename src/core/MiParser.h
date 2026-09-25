#pragma once
#include <QList>
#include <QString>
#include <QVariant>

// Stage 14: GDB/MI çıktı çözümleyici (saf mantık).
// Kayıtlar: token^done,... | *stopped,... | +... | =...,notify | ~".." | @".." | &".."
struct MiRecord {
    QString kind;   // "result" | "exec" | "status" | "notify" | "console" | "target" | "log" | "prompt" | "unknown"
    QString token;  // sonuç kayıtlarındaki istek kimliği ("": yok)
    QString cls;    // done | running | stopped | thread-created | ... (akışlarda boş)
    QVariantMap fields;
    QString stream; // console/target/log metni (çözülmüş)
};

class MiParser {
public:
    // Tek satırı çözümle (sondaki \r\n kırpılır)
    static MiRecord parseLine(const QString& line);
    // Değer ayrıştırma: "..." | {...} | [...] → QVariant
    static QVariant parseValue(const QString& s, int& pos);
    static QString parseCString(const QString& s, int& pos);

    // Yardımcı çıkarıcılar
    static QString frameFile(const QVariantMap& frame); // fullname || file
    static int frameLine(const QVariantMap& frame);
};
