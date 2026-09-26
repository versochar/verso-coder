#include "EmbeddingClient.h"
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

EmbeddingClient::EmbeddingClient(QObject* parent) : m_net(parent) {}

void EmbeddingClient::setHost(const QString& host) {
    m_host = host.trimmed();
    while (m_host.endsWith('/')) m_host.chop(1);
}

QByteArray EmbeddingClient::payload(const QString& model, const QStringList& inputs) {
    QJsonObject root;
    root["model"] = model;
    QJsonArray arr;
    for (const QString& s : inputs) arr.append(s);
    root["input"] = arr;
    return QJsonDocument(root).toJson();
}

QList<QList<float>> EmbeddingClient::parseResponse(const QJsonObject& o) {
    QList<QList<float>> out;
    const QJsonArray embs = o.value("embeddings").toArray();
    for (const QJsonValue& ev : embs) {
        QList<float> v;
        for (const QJsonValue& fv : ev.toArray()) v << float(fv.toDouble());
        out << v;
    }
    // Eski /api/embeddings biçimi: tek "embedding"
    if (out.isEmpty() && o.contains("embedding")) {
        QList<float> v;
        for (const QJsonValue& fv : o.value("embedding").toArray()) v << float(fv.toDouble());
        if (!v.isEmpty()) out << v;
    }
    return out;
}

QList<QList<float>> EmbeddingClient::embedSync(const QString& model, const QStringList& inputs,
                                               QString& error, int timeoutMs) {
    error.clear();
    if (inputs.isEmpty()) return {};
    QNetworkRequest req(QUrl(m_host + "/api/embed"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* r = m_net.post(req, payload(model, inputs));
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(r, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(qMax(1000, timeoutMs));
    loop.exec();
    if (r->isRunning()) {
        r->abort();
        r->deleteLater();
        error = "gömme zaman aşımı";
        return {};
    }
    if (r->error() != QNetworkReply::NoError) {
        error = r->errorString();
        r->deleteLater();
        return {};
    }
    const QJsonObject o = QJsonDocument::fromJson(r->readAll()).object();
    if (o.contains("error")) error = o.value("error").toString();
    r->deleteLater();
    return parseResponse(o);
}
