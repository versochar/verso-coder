#include "OllamaClient.h"
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QTimer>

OllamaClient::OllamaClient(QObject* parent) : QObject(parent) {}

void OllamaClient::setHost(const QString& host) {
    m_host = host.trimmed();
    while (m_host.endsWith('/')) m_host.chop(1);
}

void OllamaClient::fetchModels() {
    QNetworkRequest req(QUrl(m_host + "/api/tags"));
    QNetworkReply* r = m_net.get(req);
    connect(r, &QNetworkReply::finished, this, [this, r]() {
        r->deleteLater();
        if (r->error() != QNetworkReply::NoError) {
            emit error(r->errorString());
            return;
        }
        QStringList out;
        QJsonDocument d = QJsonDocument::fromJson(r->readAll());
        for (const QJsonValue& v : d.object().value("models").toArray())
            out << v.toObject().value("name").toString();
        emit modelsReady(out);
    });
}

static QByteArray chatPayload(const QString& model, const QString& systemPrompt,
                              const QString& userText, const QJsonObject& options, bool stream) {
    QJsonObject root;
    root["model"] = model;
    root["stream"] = stream;
    root["options"] = options;
    QJsonArray msgs;
    if (!systemPrompt.isEmpty())
        msgs.append(QJsonObject{{"role", "system"}, {"content", systemPrompt}});
    msgs.append(QJsonObject{{"role", "user"}, {"content", userText}});
    root["messages"] = msgs;
    return QJsonDocument(root).toJson();
}

void OllamaClient::chat(const QString& model, const QString& systemPrompt,
                        const QString& userText, const QJsonObject& options) {
    QNetworkRequest req(QUrl(m_host + "/api/chat"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* r = m_net.post(req, chatPayload(model, systemPrompt, userText, options, false));
    connect(r, &QNetworkReply::finished, this, [this, r]() {
        r->deleteLater();
        if (r->error() != QNetworkReply::NoError) {
            emit error(r->errorString());
            return;
        }
        QJsonDocument d = QJsonDocument::fromJson(r->readAll());
        QJsonObject o = d.object();
        emitTokenCounts(o);
        QString txt = o.value("message").toObject().value("content").toString();
        emit chatReply(txt.isEmpty() ? "(boş yanıt)" : txt);
    });
}

QString OllamaClient::chatSync(const QString& model, const QString& systemPrompt,
                               const QString& userText, const QJsonObject& options,
                               QString& error, int timeoutMs) {
    error.clear();
    QNetworkRequest req(QUrl(m_host + "/api/chat"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* r = m_net.post(req, chatPayload(model, systemPrompt, userText, options, false));
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
        error = "zaman aşımı";
        return {};
    }
    if (r->error() != QNetworkReply::NoError) {
        error = r->errorString();
        r->deleteLater();
        return {};
    }
    QJsonDocument d = QJsonDocument::fromJson(r->readAll());
    QJsonObject o = d.object();
    emitTokenCounts(o);
    if (o.contains("error")) error = o.value("error").toString();
    r->deleteLater();
    return o.value("message").toObject().value("content").toString();
}

void OllamaClient::emitTokenCounts(const QJsonObject& obj) {
    int p = obj.value("prompt_eval_count").toInt(0);
    int e = obj.value("eval_count").toInt(0);
    if (p > 0 || e > 0) emit tokensUsed(p, e);
}

void OllamaClient::chatStream(const QString& model, const QString& systemPrompt,
                              const QString& userText, const QJsonObject& options) {
    cancelStream();
    QNetworkRequest req(QUrl(m_host + "/api/chat"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_pending.clear();
    m_full.clear();
    m_stream = m_net.post(req, chatPayload(model, systemPrompt, userText, options, true));
    connect(m_stream, &QNetworkReply::readyRead, this, [this]() {
        if (m_stream) parseStreamChunk(m_stream->readAll());
    });
    connect(m_stream, &QNetworkReply::finished, this, [this]() {
        if (!m_stream) return;
        // Kalan baytları da işle
        parseStreamChunk(m_stream->readAll());
        bool ok = (m_stream->error() == QNetworkReply::NoError ||
                   m_stream->error() == QNetworkReply::OperationCanceledError);
        QString full = m_full;
        m_stream->deleteLater();
        m_stream = nullptr;
        if (!ok) { emit error(m_stream ? m_stream->errorString() : "ağ hatası"); return; }
        if (full.isEmpty()) emit error("(boş yanıt)");
        else emit chatFinished(full);
    });
}

void OllamaClient::cancelStream() {
    if (m_stream) {
        // finished() sinyalindeki error dalını susturmak için önce bağlantıyı kopar
        m_stream->disconnect(this);
        m_stream->abort();
        m_stream->deleteLater();
        m_stream = nullptr;
    }
}

void OllamaClient::parseStreamChunk(const QByteArray& data) {
    m_pending += data;
    int from = 0;
    while (true) {
        int nl = m_pending.indexOf('\n', from);
        if (nl < 0) break;
        QByteArray line = m_pending.mid(from, nl - from).trimmed();
        from = nl + 1;
        if (line.isEmpty()) continue;
        QJsonDocument d = QJsonDocument::fromJson(line);
        if (!d.isObject()) continue;
        QString piece = d.object().value("message").toObject().value("content").toString();
        if (!piece.isEmpty()) {
            m_full += piece;
            emit chatToken(piece);
        }
        if (d.object().contains("error")) {
            emit error(d.object().value("error").toString());
            return;
        }
        if (d.object().value("done").toBool(false)) emitTokenCounts(d.object());
    }
    m_pending = m_pending.mid(from);
}

// ---------- Stage 15: model indirme ----------

void OllamaClient::pull(const QString& model) {
    cancelPull();
    QNetworkRequest req(QUrl(m_host + "/api/pull"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_pull = m_net.post(req, QJsonDocument(QJsonObject{{"model", model}}).toJson());
    connect(m_pull, &QNetworkReply::readyRead, this, [this, model]() {
        if (!m_pull) return;
        QByteArray data = m_pull->readAll();
        for (const QByteArray& raw : data.split('\n')) {
            QByteArray line = raw.trimmed();
            if (line.isEmpty()) continue;
            QJsonDocument d = QJsonDocument::fromJson(line);
            if (!d.isObject()) continue;
            QJsonObject o = d.object();
            if (o.contains("error")) { emit error(o.value("error").toString()); return; }
            emit pullProgress(model, (qint64)o.value("completed").toDouble(),
                              (qint64)o.value("total").toDouble(),
                              o.value("status").toString());
        }
    });
    connect(m_pull, &QNetworkReply::finished, this, [this, model]() {
        if (!m_pull) return;
        const bool ok = (m_pull->error() == QNetworkReply::NoError);
        const QString err = m_pull->errorString();
        m_pull->deleteLater();
        m_pull = nullptr;
        if (!ok) { emit error(err); return; }
        emit pullFinished(model);
    });
}

void OllamaClient::cancelPull() {
    if (m_pull) {
        m_pull->disconnect(this);
        m_pull->abort();
        m_pull->deleteLater();
        m_pull = nullptr;
    }
}
