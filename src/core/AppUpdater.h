#pragma once
#include <QObject>
#include <QString>

// Sürüm dosyası indirici: eşzamansız GET → dosyaya yaz (http(s) ve file://).
// İptal edilebilir; yarım dosya silinir.
class AppUpdater : public QObject {
    Q_OBJECT
public:
    explicit AppUpdater(QObject* parent = nullptr);

    void setSourceUrl(const QString& u) { m_url = u; }
    void setDestDir(const QString& d) { m_dir = d; }
    void setFileName(const QString& n) { m_name = n; }
    void setTimeoutMs(int ms) { m_timeoutMs = qMax(2000, ms); }
    void start();
    void cancel();

signals:
    void progress(qint64 received, qint64 total);
    void finished(const QString& path);
    void failed(const QString& error);

private:
    QString m_url;
    QString m_dir;
    QString m_name;
    int m_timeoutMs = 120000;
    bool m_running = false;
};
