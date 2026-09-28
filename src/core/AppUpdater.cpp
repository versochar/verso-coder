#include "AppUpdater.h"
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QTimer>

AppUpdater::AppUpdater(QObject* parent) : QObject(parent) {}

void AppUpdater::start() {
    if (m_running) return;
    QString dir = m_dir;
    if (dir.isEmpty())
        dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QString name = m_name;
    if (name.isEmpty()) {
        name = QUrl(m_url).fileName();
        if (name.isEmpty()) name = "verso-guncelleme.bin";
    }
    if (dir.isEmpty() || !QDir().mkpath(dir)) {
        emit failed("indirme dizini açılamadı");
        return;
    }
    const QString path = QDir(dir).filePath(name);
    auto* out = new QFile(path, this);
    if (!out->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        out->deleteLater();
        emit failed("dosya yazılamadı");
        return;
    }
    auto* nam = new QNetworkAccessManager(this);
    QNetworkReply* reply = nam->get(QNetworkRequest(QUrl(m_url)));
    auto* timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::downloadProgress, this,
            [this](qint64 r, qint64 t) { emit progress(r, t); });
    connect(reply, &QNetworkReply::readyRead, out,
            [reply, out]() { out->write(reply->readAll()); });
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, nam, timer, out, path]() {
                timer->stop();
                const bool ok = reply->error() == QNetworkReply::NoError;
                const QString err = ok ? QString()
                    : reply->error() == QNetworkReply::OperationCanceledError
                        ? "iptal edildi"
                        : reply->errorString().left(200);
                out->close();
                if (!ok) {
                    out->remove(); // yarım dosya bırakma
                    emit failed(err.isEmpty() ? "indirilemedi" : err);
                } else if (out->size() == 0) {
                    out->remove();
                    emit failed("boş yanıt");
                } else {
                    emit finished(path);
                }
                m_running = false;
                reply->deleteLater();
                nam->deleteLater();
                timer->deleteLater();
                out->deleteLater();
            });
    m_running = true;
    timer->start(m_timeoutMs);
}

void AppUpdater::cancel() {
    // Aktif yanıt varsa finished yolundan "iptal edildi" üretir
    for (QNetworkReply* r : findChildren<QNetworkReply*>()) r->abort();
}
