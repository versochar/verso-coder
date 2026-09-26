#pragma once
#include <QList>
#include <QString>

// Stage 31: kararlılık yardımcıları — saf mantık (test edilebilir).
class Stability {
public:
    // Dosya sha256 (boşsa "")
    static QString shaFile(const QString& path);
    // Bellek-içi sha256 hex
    static QString shaData(const QByteArray& data);
    // Atomik yazım (tmp + rename) + .sha256 eş dosyası
    static bool writeChecked(const QString& path, const QByteArray& data,
                             QString* error = nullptr);
    // Doğrulamalı okuma (eşleşmezse false)
    static bool readChecked(const QString& path, QByteArray& data);

    // Üstel geri çekilme: denemeNo 0,1,2… → ms (tavanlı + titremesiz)
    struct Backoff {
        int baseMs = 1000;
        int maxMs = 60000;
        int delayFor(int attempt) const;
    };
};

// Stage 31: log rotasyonu — boyut aşımında .1/.2 kaydırma.
class LogRotate {
public:
    // path > maxBytes ise döndür, en çok keep yedek tutar
    static bool rotate(const QString& path, int keep = 5,
                       qint64 maxBytes = 1024 * 1024);
};
