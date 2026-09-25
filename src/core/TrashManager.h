#pragma once
#include <QDateTime>
#include <QList>
#include <QString>

// Güvenli silme: dosyalar kalıcı silinmek yerine çöp kutusuna taşınır.
class TrashManager {
public:
    explicit TrashManager(const QString& trashDir = QString());
    static QString defaultDir();

    bool trash(const QString& absPath); // taşı; false ise silinmedi
    struct Entry {
        QString id;        // zaman damgalı klasör adı
        QString name;      // özgün ad
        QString origPath;  // özgün tam yol
        QDateTime when;
        bool isDir = false;
    };
    QList<Entry> list() const;
    bool restore(const QString& id); // özgün yola geri taşı (çakışırsa -geriN)
    bool clear();

private:
    QString metaPath(const QString& id) const;
    QString m_dir;
};
