#pragma once
#include <QList>
#include <QString>
#include <QStringList>

// Stage 38: kesintili RAG indeksleme.
// Canlı ölçüm: ücretsiz sağlayıcılarda gömme 1 istek / 30 dakika. Binlerce
// parçayı tek oturumda gömmek imkânsız; ilerleme diske yazılır ve bir sonraki
// oturumda kaldığı yerden sürülür. "Beklemede" durumu kullanıcıya gösterilir.
struct RagProgress {
    QString root;         // proje kökü
    QStringList pending;  // henüz gömülmemiş dosyalar
    int indexed = 0;      // gömülen parça sayısı
    int total = 0;        // toplam parça (tahmin)
    bool waiting = false; // hız sınırı nedeniyle bekliyor
    qint64 waitUntilMs = 0; // bekleme bitiş zamanı (epoch ms)

    bool valid() const { return !root.isEmpty() && !pending.isEmpty(); }
    int doneCount() const { return int(pending.size()); }
    // Yüzde (0-100); toplam bilinmiyorsa 0
    int percent() const;
    QString describe() const;
};

class RagProgressStore {
public:
    explicit RagProgressStore(const QString& file = QString());

    QString filePath() const { return m_file; }
    bool load();
    bool save() const;
    void clear();

    RagProgress progress() const { return m_progress; }
    void setProgress(const RagProgress& p);
    // Bekleme durumu
    void markWaiting(qint64 untilEpochMs);
    void clearWaiting();
    bool isWaiting(qint64 nowEpochMs = 0) const;
    int waitLeftSec(qint64 nowEpochMs = 0) const;
    // Oturum sırasında ilerleme (her çağrıda diske yazar)
    void advance(const QString& fileDone);
    // Farklı projeyse eski ilerlemeyi tutma
    bool matchesRoot(const QString& root) const;

private:
    QString m_file;
    RagProgress m_progress;
};
