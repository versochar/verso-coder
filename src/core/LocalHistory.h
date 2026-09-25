#pragma once
#include <QDateTime>
#include <QList>
#include <QString>

// Stage 17: yerel geçmiş — kaydetmede zaman damgalı anlık görüntüler.
// Depo: <yedekDizini>/history/<hash>/<zaman>.bak (içerik + meta).
struct HistorySnap {
    QString id; // "yyyyMMdd-HHmmss" (+ çakışmada -n)
    QDateTime when;
    qint64 size = 0;
    QString path; // .bak tam yolu
};

class LocalHistory {
public:
    explicit LocalHistory(const QString& storeDir);

    // Anlık görüntü al (içerik aynıysa atlanır → false)
    bool snapshot(const QString& filePath, const QString& content, int keepMax = 50);
    QList<HistorySnap> list(const QString& filePath) const;
    QString read(const HistorySnap& s) const;
    // Geri yükle: önce günü kurtar (mevcut içeriği de anlık görüntüle)
    bool restore(const QString& filePath, const HistorySnap& s);
    bool clear(const QString& filePath);
    // Eski ranlıklar budanır (keepMax üstü silinir), silinen sayısı döner
    int prune(const QString& filePath, int keepMax = 50);

    static QString keyFor(const QString& filePath); // dizin adı (hash)
    static QString stamp();

private:
    QString dirFor(const QString& filePath) const;
    QString m_store;
};
