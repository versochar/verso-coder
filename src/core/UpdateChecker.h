#pragma once
#include <QObject>
#include <QString>

// Stage 48: güncelleme denetimi. GitHub releases yoklaması (haftada 1,
// kapatılabilir). Ağ yoksa sessizce vazgeçer; test file:// URL ile çalışır.
class UpdateChecker : public QObject {
    Q_OBJECT
public:
    struct Result {
        bool checked = false;   // yoklama tamamlandı mı
        bool newer = false;     // daha yeni sürüm var mı
        QString latest;         // "3.0.1"
        QString url;            // sürüm sayfası
        QString error;
    };

    explicit UpdateChecker(QObject* parent = nullptr);

    void setFeedUrl(const QString& url) { m_feed = url; }
    QString feedUrl() const { return m_feed; }
    void setCurrentVersion(const QString& v) { m_current = v; }
    void setTimeoutMs(int ms) { m_timeoutMs = qMax(1000, ms); }

    // Eşzamansız yoklama (sonuç ready sinyalinde)
    void check();
    // Engelleyici yoklama (test + CLI)
    Result checkSync();

    // Saf yardımcılar (test edilebilir)
    static int compareVersions(const QString& a, const QString& b); // -1/0/1
    static Result parseFeed(const QByteArray& body, const QString& current);
    static bool shouldCheck();   // haftada 1 + ayar açık mı
    static void markChecked();   // "denetlendi" damgası

signals:
    void ready(const Result& r);

private:
    QString m_feed = "https://api.github.com/repos/versochar/verso-coder/releases/latest";
    QString m_current;
    int m_timeoutMs = 8000;
};
