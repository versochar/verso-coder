#include "Stability.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

QString Stability::shaFile(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QCryptographicHash h(QCryptographicHash::Sha256);
    while (!f.atEnd()) h.addData(f.read(1 << 20));
    return QString::fromLatin1(h.result().toHex());
}

QString Stability::shaData(const QByteArray& data) {
    return QString::fromLatin1(
        QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

bool Stability::writeChecked(const QString& path, const QByteArray& data,
                             QString* error) {
    QSaveFile f(path); // atomik: tmp + committe rename
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) *error = f.errorString();
        return false;
    }
    if (f.write(data) != data.size()) {
        if (error) *error = "yazım yarım kaldı";
        f.cancelWriting();
        return false;
    }
    if (!f.commit()) {
        if (error) *error = f.errorString();
        return false;
    }
    QFile sf(path + ".sha256");
    if (sf.open(QIODevice::WriteOnly | QIODevice::Truncate))
        sf.write(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
    if (error) error->clear();
    return true;
}

bool Stability::readChecked(const QString& path, QByteArray& data) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    data = f.readAll();
    QFile sf(path + ".sha256");
    if (!sf.open(QIODevice::ReadOnly)) return true; // eski dosya: mühürsüz kabul
    const QByteArray expect = sf.readAll().trimmed();
    const QByteArray actual =
        QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();
    return actual == expect;
}

int Stability::Backoff::delayFor(int attempt) const {
    if (attempt < 0) return baseMs;
    qint64 d = qint64(baseMs) << qMin(attempt, 20);
    if (d > maxMs) d = maxMs;
    return int(d);
}

bool LogRotate::rotate(const QString& path, int keep, qint64 maxBytes) {
    QFileInfo fi(path);
    if (!fi.exists() || fi.size() <= maxBytes) return true;
    for (int i = keep - 1; i >= 1; --i) {
        const QString src = (i == 1) ? path : QString("%1.%2").arg(path).arg(i - 1);
        const QString dst = QString("%1.%2").arg(path).arg(i);
        if (QFile::exists(src)) {
            QFile::remove(dst);
            QFile::rename(src, dst);
        }
    }
    return true;
}
