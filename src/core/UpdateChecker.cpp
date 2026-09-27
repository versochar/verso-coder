#include "UpdateChecker.h"
#include <QDateTime>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>

UpdateChecker::UpdateChecker(QObject* parent) : QObject(parent) {}

int UpdateChecker::compareVersions(const QString& a, const QString& b) {
    // "2.9.0" kesimleri sayısal karşılaştır; öneksiz "v" yutulur
    auto parts = [](QString v) {
        v.remove(QRegularExpression("^[vV]"));
        QList<int> out;
        for (const QString& p : v.split('.')) {
            bool ok = false;
            const int n = p.split('-').first().toInt(&ok);
            out << (ok ? n : 0);
        }
        while (out.size() < 3) out << 0;
        return out;
    };
    const QList<int> x = parts(a), y = parts(b);
    for (int i = 0; i < 3; ++i) {
        if (x[i] != y[i]) return x[i] < y[i] ? -1 : 1;
    }
    return 0;
}

UpdateChecker::Result UpdateChecker::parseFeed(const QByteArray& body,
                                               const QString& current) {
    Result r;
    r.checked = true;
    const QJsonObject o = QJsonDocument::fromJson(body).object();
    if (o.isEmpty()) {
        r.error = "yanıt çözümlenemedi";
        return r;
    }
    r.latest = o.value("tag_name").toString().trimmed();
    r.url = o.value("html_url").toString();
    if (r.latest.isEmpty()) {
        r.error = "sürüm alanı yok";
        return r;
    }
    r.newer = compareVersions(current, r.latest) < 0;
    return r;
}

bool UpdateChecker::shouldCheck() {
    QSettings st("Verso", "VersoCoder");
    if (!st.value("update/check", true).toBool()) return false;
    const QDateTime last =
        st.value("update/lastCheck").toDateTime();
    if (!last.isValid()) return true;
    return last.daysTo(QDateTime::currentDateTime()) >= 7;
}

void UpdateChecker::markChecked() {
    QSettings("Verso", "VersoCoder")
        .setValue("update/lastCheck", QDateTime::currentDateTime());
}

UpdateChecker::Result UpdateChecker::checkSync() {
    Result r;
    QNetworkAccessManager nam;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QNetworkReply* reply = nam.get(QNetworkRequest(QUrl(m_feed)));
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(m_timeoutMs);
    loop.exec();
    if (!reply->isFinished() || reply->error() != QNetworkReply::NoError) {
        r.error = reply->isFinished() ? reply->errorString() : "zaman aşımı";
        reply->deleteLater();
        return r;
    }
    r = parseFeed(reply->readAll(), m_current);
    reply->deleteLater();
    return r;
}

void UpdateChecker::check() {
    // Arka planda engelleyici yoklama (kısa); sonuç sinyalle gelir
    QTimer::singleShot(0, this, [this]() {
        const Result r = checkSync();
        if (r.checked) markChecked();
        emit ready(r);
    });
}
