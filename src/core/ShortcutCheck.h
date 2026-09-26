#pragma once
#include <QMap>
#include <QString>
#include <QStringList>

// Stage 21: kısayol çakışma denetimi — saf mantık (test edilebilir).
// Girdi: komut-id → "Ctrl+N" (boş = tanımsız). Çıktı: "Ctrl+N: A, B" satırları.
class ShortcutCheck {
public:
    static QStringList findConflicts(const QMap<QString, QString>& idToKeys) {
        QMap<QString, QStringList> byKeys;
        for (auto it = idToKeys.constBegin(); it != idToKeys.constEnd(); ++it) {
            const QString k = it.value().trimmed();
            if (!k.isEmpty()) byKeys[k] << it.key();
        }
        QStringList out;
        for (auto it = byKeys.constBegin(); it != byKeys.constEnd(); ++it)
            if (it.value().size() > 1) {
                QStringList v = it.value();
                v.sort();
                out << QString("%1: %2").arg(it.key(), v.join(", "));
            }
        out.sort();
        return out;
    }
};
