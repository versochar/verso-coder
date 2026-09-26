#pragma once
#include <QDialog>
#include <QMap>
#include <QString>

// Stage 24: Proje Radarı — metrikler, yinelenen kod, kullanılmayan
// fonksiyonlar, dosya bağımlılıkları. Sekmeli salt-okunur diyalog.
class MetricsDialog : public QDialog {
    Q_OBJECT
public:
    // root: proje kökü; currentFile: etkin dosya (metrik + kullanılmayan için)
    explicit MetricsDialog(const QString& root, const QString& currentFile,
                           QWidget* parent = nullptr);

signals:
    void fileJumpRequested(const QString& path, int line);

private:
    // Kökteki kaynak dosyalar (uzantı filtreli, en çok 1500)
    static QStringList collectSources(const QString& root);
    static QString readCapped(const QString& path);

    QString m_root;
    QString m_current;
};
