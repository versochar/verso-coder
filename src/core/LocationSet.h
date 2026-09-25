#pragma once
#include <QJsonObject>
#include <QList>
#include <QString>

// Stage 13: konum kümesi — references/definition yanıtlarını parse + grupla.
struct LspLocation {
    QString path;
    int line = 0; // 0-based
    int col = 0;
    int endLine = 0;
    int endCol = 0;
};

class LocationSet {
public:
    // Tek Location | Location[] | LocationLink[] hepsini kabul eder
    static QList<LspLocation> parse(const QJsonObject& res);
    static QList<LspLocation> parseArray(const class QJsonArray& arr);
    // Dosyaya göre grupla (sıralı dosya listesi döner)
    static QStringList files(const QList<LspLocation>& locs);
    static QList<LspLocation> forFile(const QList<LspLocation>& locs, const QString& path);
};
