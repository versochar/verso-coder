#pragma once
#include <QCryptographicHash>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

// Stage 24: yinelenen kod tespiti — normalize satır bloklarının hash'i.
// Saf mantık (test edilebilir).
struct DupGroup {
    QString hash;
    QStringList files; // "dosya:satır" kayıtları
    int lines = 0;
};

class DuplicateFinder {
public:
    // pathToText: dosya → içerik; minLines: en küçük blok (varsayılan 6)
    static QList<DupGroup> find(const QMap<QString, QString>& pathToText,
                                int minLines = 6);
};
