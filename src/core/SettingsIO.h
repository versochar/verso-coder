#pragma once
#include <QJsonObject>
#include <QString>

class QSettings;

// Tüm ayarları JSON'a aktar / JSON'dan yükle (yedekleme + taşınabilirlik).
// QSettings& alınır; böylece test edilebilir (geçici INI dosyası ile).
class SettingsIO {
public:
    static QJsonObject dump(QSettings& q);
    static bool load(QSettings& q, const QJsonObject& obj, QString* error = nullptr);
    static bool writeFile(const QString& path, QSettings& q, QString* error = nullptr);
    static bool readFile(const QString& path, QSettings& q, QString* error = nullptr);
};
