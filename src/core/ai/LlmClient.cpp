#include "LlmClient.h"
#include "SecretStore.h"
#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>
#include <QTimer>

LlmClient::LlmClient(QObject* parent) : QObject(parent) {
    m_spec = ProviderRegistry::defaultProvider();
}

LlmClient::~LlmClient() { cancel(); }

void LlmClient::setProvider(const ProviderSpec& spec) {
    if (m_spec.id == spec.id && m_spec.baseUrl == spec.baseUrl) return;
    cancel();
    m_spec = spec;
    if (m_apiKey.isEmpty()) loadKeyForProvider();
}

QString LlmClient::apiKey() const {
    if (!m_apiKey.isEmpty()) return m_apiKey;
    if (m_secrets) return m_secrets->effectiveKey(m_spec.id);
    return {};
}

void LlmClient::loadKeyForProvider() {
    if (!m_secrets) return;
    m_apiKey = m_secrets->effectiveKey(m_spec.id);
}

QJsonObject LlmClient::requestHeaders(const ProviderSpec& spec, const QString& apiKey) {
    QJsonObject h = spec.allHeaders(apiKey);
    h["Content-Type"] = "application/json";
    h["Accept"] = "application/json";
    return h;
}

bool LlmClient::isRetryable(int httpStatus) {
    return httpStatus == 429 || httpStatus == 408 || (httpStatus >= 500 && httpStatus < 600);
}

int LlmClient::retryDelayMs(int attempt, int httpStatus, const QByteArray& retryAfter) {
    if (isRetryable(httpStatus) && !retryAfter.trimmed().isEmpty()) {
        bool ok = false;
        const int secs = retryAfter.trimmed().toInt(&ok);
        if (ok && secs >= 0) return qBound(200, secs * 1000, 60000);
    }
    const int base = 800;                       // 0.8s
    const int d = base * (1 << qBound(0, attempt, 5)); // üstel
    const int jitter = (attempt * 137) % 400;   // küçük kaydırma
    return qBound(200, d + jitter, 30000);
}

QNetworkReply* LlmClient::post(const QJsonObject& body, const QString& url) {
    QNetworkRequest r{QUrl(url)};
    const QJsonObject h = requestHeaders(m_spec, apiKey());
    for (auto it = h.begin(); it != h.end(); ++it)
        r.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
    return m_net.post(r, QJsonDocument(body).toJson(QJsonDocument::Compact));
}

void LlmClient::failWith(const QString& msg, int status) {
    m_busy = false;
    emit error(msg);
    Q_UNUSED(status);
}

// --- model listeleme ---

void LlmClient::fetchModels() {
    const QString url = ProviderCodec::modelsUrl(m_spec);
    if (url.isEmpty()) {
        // Katalog uç noktası yok: örnek modelleri göster
        emit modelsReady(ProviderRegistry::sampleModels(m_spec.id));
        return;
    }
    QNetworkRequest r{QUrl(url)};
    const QJsonObject h = requestHeaders(m_spec, apiKey());
    for (auto it = h.begin(); it != h.end(); ++it)
        r.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
    QNetworkReply* rep = m_net.get(r);
    m_reply = rep;
    m_busy = true;
    connect(rep, &QNetworkReply::finished, this, [this, rep]() {
        rep->deleteLater();
        m_reply = nullptr;
        m_busy = false;
        const QByteArray body = rep->readAll();
        if (rep->error() != QNetworkReply::NoError) {
            emit error(ProviderRegistry::mapError(m_spec, 0, body));
            emit modelsReady(ProviderRegistry::sampleModels(m_spec.id));
            return;
        }
        if (rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() >= 400) {
            emit error(ProviderRegistry::mapError(
                m_spec, rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), body));
            emit modelsReady(ProviderRegistry::sampleModels(m_spec.id));
            return;
        }
        QStringList models = ProviderCodec::parseModels(m_spec, body);
        if (models.isEmpty()) models = ProviderRegistry::sampleModels(m_spec.id);
        emit modelsReady(models);
    });
}

// --- sohbet ---

void LlmClient::chat(const AiChatRequest& request) {
    AiChatRequest r = request;
    r.stream = false;
    sendChat(r, false);
}

void LlmClient::chatStream(const AiChatRequest& request) {
    sendChat(request, true);
}

void LlmClient::sendChat(const AiChatRequest& req, bool stream) {
    if (m_busy) cancel();
    m_req = req;
    m_wantStream = stream;
    m_attempt = 0;
    m_sse.clear();
    m_streamCalls.clear();
    m_acc = AiReply();
    m_acc.model = m_spec.resolveModelId(req.model);

    if (m_spec.requiresKey() && apiKey().isEmpty()) {
        emit error(QString("%1 için API anahtarı gerekli (Ayarlar → Sağlayıcılar).")
                       .arg(m_spec.label));
        return;
    }

    QString model;
    const QJsonObject body = ProviderCodec::chatRequest(m_spec, req, &model);
    QJsonObject b = body;
    b["stream"] = stream;
    if (m_spec.kind == ProviderKind::Anthropic) b["stream"] = stream;
    const QString url = ProviderCodec::chatUrl(m_spec, model);

    QNetworkReply* rep = post(b, url);
    m_reply = rep;
    m_busy = true;
    emit statusChanged(stream ? "akış başlıyor" : "gönderiliyor");

    // Zaman aşımı (Stage 31 ağ katmanı)
    auto* t = new QTimer(rep);
    t->setSingleShot(true);
    connect(t, &QTimer::timeout, rep, [rep]() {
        if (rep->isRunning()) rep->abort();
    });
    t->start(stream ? 300000 : 240000);

    connect(rep, &QNetworkReply::readyRead, this, [this]() {
        if (!m_reply) return;
        if (!m_wantStream) return;
        m_sse.feed(m_reply->readAll());
        for (const QString& ev : m_sse.events()) {
            const AiChunk ch = ProviderCodec::parseStreamEvent(m_spec, ev, m_req.model);
            if (!ch.text.isEmpty() || !ch.reasoning.isEmpty() || !ch.toolCalls.isEmpty())
                emit chunkReady(ch);
            if (ch.usage.total() > 0) {
                m_acc.usage = ch.usage;
                emit tokensUsed(ch.usage.promptTokens, ch.usage.evalTokens,
                                ch.usage.reasoningTokens);
            }
        }
        m_sse.clear(); // işlenenler tüketildi
    });

    connect(rep, &QNetworkReply::finished, this, [this, rep]() {
        rep->deleteLater();
        m_reply = nullptr;
        m_busy = false;
        const int status = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = rep->readAll();

        if (isRetryable(status) && m_attempt < maxRetries()) {
            const int delay = retryDelayMs(m_attempt, status,
                                           rep->rawHeader("Retry-After"));
            ++m_attempt;
            emit statusChanged(QString("%1 → %2 sn sonra yeniden deneniyor")
                                   .arg(status)
                                   .arg(delay / 1000));
            QTimer::singleShot(delay, this, [this]() { sendChat(m_req, m_wantStream); });
            return;
        }

        if (m_wantStream) {
            m_sse.feed(body);
            m_sse.flush();
            for (const QString& ev : m_sse.events()) {
                const AiChunk ch = ProviderCodec::parseStreamEvent(m_spec, ev, m_req.model);
                m_acc.text += ch.text;
                m_acc.reasoning += ch.reasoning;
                m_acc.toolCalls += ch.toolCalls;
                if (ch.usage.total() > 0) m_acc.usage = ch.usage;
            }
            if (status >= 400 || rep->error() != QNetworkReply::NoError) {
                if (m_acc.text.isEmpty()) {
                    failWith(ProviderRegistry::mapError(m_spec, status, body), status);
                    return;
                }
            }
            m_acc.ok = !m_acc.text.isEmpty() || !m_acc.toolCalls.isEmpty();
            if (!m_acc.ok && m_acc.error.isEmpty())
                m_acc.error = "Boş yanıt";
            emit finished(m_acc);
            return;
        }

        if (status >= 400 || rep->error() != QNetworkReply::NoError) {
            failWith(ProviderRegistry::mapError(m_spec, status, body), status);
            return;
        }
        AiReply r = ProviderCodec::parseReply(m_spec, body, m_req.model);
        r.httpStatus = status;
        r.requestId = QString::fromLatin1(rep->rawHeader("x-request-id"));
        if (r.usage.total() > 0)
            emit tokensUsed(r.usage.promptTokens, r.usage.evalTokens, r.usage.reasoningTokens);
        if (r.ok) emit replyReady(r);
        else emit error(r.error);
        emit finished(r);
    });
}

void LlmClient::flushStream() {
    if (!m_reply || !m_wantStream) return;
    m_sse.feed(m_reply->readAll());
    for (const QString& ev : m_sse.events()) {
        const AiChunk ch = ProviderCodec::parseStreamEvent(m_spec, ev, m_req.model);
        if (!ch.text.isEmpty()) emit chunkReady(ch);
    }
    m_sse.clear();
}

void LlmClient::handleChatFinished() { flushStream(); }

void LlmClient::scheduleRetry() { ++m_attempt; }

void LlmClient::cancel() {
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    m_busy = false;
    m_sse.clear();
}

// --- senkron çağrılar ---

AiReply LlmClient::chatSync(const AiChatRequest& request, int timeoutMs) {
    AiReply r;
    if (m_spec.requiresKey() && apiKey().isEmpty()) {
        r.error = QString("%1 için API anahtarı eksik.").arg(m_spec.label);
        return r;
    }
    QString model;
    QJsonObject b = ProviderCodec::chatRequest(m_spec, request, &model);
    b["stream"] = false;
    const QString url = ProviderCodec::chatUrl(m_spec, model);

    for (int attempt = 0; attempt <= maxRetries(); ++attempt) {
        QNetworkReply* rep = post(b, url);
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(rep, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        timer.start(qMax(1000, timeoutMs));
        loop.exec();
        if (rep->isRunning()) {
            rep->abort();
            rep->deleteLater();
            r.error = "Zaman aşımı";
            return r;
        }
        const int status = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = rep->readAll();
        const bool netErr = rep->error() != QNetworkReply::NoError;
        rep->deleteLater();
        if (netErr || status >= 400) {
            if (isRetryable(status) && attempt < maxRetries()) {
                QThread::msleep(static_cast<unsigned long>(
                    retryDelayMs(attempt, status)));
                continue;
            }
            r.httpStatus = status;
            r.error = ProviderRegistry::mapError(m_spec, status, body);
            return r;
        }
        r = ProviderCodec::parseReply(m_spec, body, request.model);
        r.httpStatus = status;
        return r;
    }
    r.error = "Yeniden deneme sınırı aşıldı";
    return r;
}

QList<QList<float>> LlmClient::embedSync(const QString& model, const QStringList& inputs,
                                        QString& error, int timeoutMs) {
    error.clear();
    if (!m_spec.supportsEmbed) {
        error = m_spec.label + " gömme desteklemiyor.";
        return {};
    }
    if (inputs.isEmpty()) return {};
    QString resolved;
    const QJsonObject b = ProviderCodec::embedRequest(m_spec, model, inputs, &resolved);
    const QString url = ProviderCodec::embedUrl(m_spec, model);
    if (url.isEmpty()) {
        error = "Gömme uç noktası yok.";
        return {};
    }
    QNetworkReply* rep = post(b, url);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(rep, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(qMax(1000, timeoutMs));
    loop.exec();
    if (rep->isRunning()) {
        rep->abort();
        rep->deleteLater();
        error = "Gömme zaman aşımı";
        return {};
    }
    const int status = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = rep->readAll();
    if (rep->error() != QNetworkReply::NoError || status >= 400) {
        rep->deleteLater();
        error = ProviderRegistry::mapError(m_spec, status, body);
        return {};
    }
    rep->deleteLater();
    return ProviderCodec::parseEmbed(m_spec, body);
}

bool LlmClient::testConnection(QString& error, int timeoutMs) {
    error.clear();
    if (m_spec.requiresKey() && apiKey().isEmpty()) {
        error = "API anahtarı girilmemiş.";
        return false;
    }
    // 1) Katalog dene
    const QString modelsUrl = ProviderCodec::modelsUrl(m_spec);
    if (!modelsUrl.isEmpty()) {
        QNetworkRequest r{QUrl(modelsUrl)};
        const QJsonObject h = requestHeaders(m_spec, apiKey());
        for (auto it = h.begin(); it != h.end(); ++it)
            r.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
        QNetworkReply* rep = m_net.get(r);
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(rep, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        timer.start(qMax(1000, timeoutMs));
        loop.exec();
        if (rep->isRunning()) {
            rep->abort();
            rep->deleteLater();
            error = "Bağlantı zaman aşımı";
            return false;
        }
        const int status = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = rep->readAll();
        const bool ok = rep->error() == QNetworkReply::NoError && status < 400;
        rep->deleteLater();
        if (ok) return true;
        error = ProviderRegistry::mapError(m_spec, status, body);
        return false;
    }
    // 2) Katalog yoksa küçük bir sohbet isteği dene
    AiChatRequest req;
    req.model = ProviderRegistry::sampleModels(m_spec.id).value(0);
    if (req.model.isEmpty()) req.model = "test";
    req.messages << AiMessage::user("ping");
    req.maxTokens = 8;
    const AiReply r = chatSync(req, timeoutMs);
    if (!r.ok) {
        error = r.error;
        return false;
    }
    return true;
}
