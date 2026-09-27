#include "CollabSession.h"
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>
#include <QJsonObject>

CollabSession::CollabSession(QObject* parent) : QObject(parent) {}

CollabSession::~CollabSession() { leave(); }

int CollabSession::host(const QString& user, quint16 port) {
    leave();
    m_server = new QWebSocketServer("verso-collab", QWebSocketServer::NonSecureMode,
                                    this);
    if (!m_server->listen(QHostAddress::Any, port)) {
        emit sessionError("Dinlenemedi: " + m_server->errorString());
        m_server->deleteLater();
        m_server = nullptr;
        return -1;
    }
    m_user = user.trimmed().isEmpty() ? "evsahibi" : user.trimmed();
    m_role = "host";
    m_active = true;
    m_rev = 0;
    connect(m_server, &QWebSocketServer::newConnection, this,
            &CollabSession::onNewConnection);
    emit activeChanged(true);
    return m_server->serverPort();
}

bool CollabSession::join(const QString& url, const QString& user) {
    leave();
    m_pendingUser = user.trimmed().isEmpty() ? "misafir" : user.trimmed();
    m_client = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    connect(m_client, &QWebSocket::connected, this, &CollabSession::onConnected);
    connect(m_client, &QWebSocket::textMessageReceived, this,
            &CollabSession::onTextMessage);
    connect(m_client, &QWebSocket::disconnected, this, &CollabSession::onClosed);
    connect(m_client, &QWebSocket::errorOccurred, this, &CollabSession::onWsError);
    m_client->open(QUrl(url));
    return true;
}

void CollabSession::onConnected() {
    if (!m_client || m_active) return;
    m_user = m_pendingUser;
    m_role = "peer";
    m_active = true;
    m_rev = 0;
    m_client->sendTextMessage(
        QString::fromUtf8(QJsonDocument(CollabMerge::makeHello(m_user, 0)).toJson()));
    emit activeChanged(true);
}

void CollabSession::onWsError(QAbstractSocket::SocketError err) {
    Q_UNUSED(err);
    if (m_active || !m_client) return;
    const QString e = m_client->errorString();
    m_client->deleteLater();
    m_client = nullptr;
    m_pendingUser.clear();
    emit sessionError("Bağlanılamadı: " + e);
}

void CollabSession::leave() {
    if (m_client) {
        m_client->close();
        m_client->deleteLater();
        m_client = nullptr;
    }
    for (QWebSocket* s : m_socks) {
        s->close();
        s->deleteLater();
    }
    m_socks.clear();
    m_sockUser.clear();
    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
    const bool was = m_active;
    m_active = false;
    m_role.clear();
    m_pendingUser.clear();
    m_cursors.clear();
    if (was) emit activeChanged(false);
}

void CollabSession::setBaseText(const QString& text) {
    m_base = text;
    m_rev = 0;
}

void CollabSession::publishText(const QString& newText) {
    if (!m_active || newText == m_base) return;
    const auto ops = CollabMerge::diff(m_base, newText);
    if (ops.isEmpty()) return;
    m_base = newText;
    ++m_rev;
    broadcast(CollabMerge::makeEdit(m_user, m_rev - 1, ops));
}

void CollabSession::publishCursor(int line, int col) {
    if (!m_active) return;
    broadcast(CollabMerge::makeCursor(m_user, line, col));
}

void CollabSession::publishTerm(const QString& chunk) {
    if (!m_active || m_role != "host" || chunk.isEmpty()) return;
    broadcast({{"t", "term"}, {"user", m_user}, {"chunk", chunk.left(4000)}});
}

void CollabSession::broadcast(const QJsonObject& msg, QWebSocket* except) {
    const QString text = QString::fromUtf8(QJsonDocument(msg).toJson());
    if (m_role == "host") {
        for (QWebSocket* s : m_socks)
            if (s != except && s->isValid()) s->sendTextMessage(text);
    } else if (m_client && m_client != except && m_client->isValid()) {
        m_client->sendTextMessage(text);
    }
}

void CollabSession::onNewConnection() {
    QWebSocket* s = m_server ? m_server->nextPendingConnection() : nullptr;
    if (!s) return;
    connect(s, &QWebSocket::textMessageReceived, this,
            &CollabSession::onTextMessage);
    connect(s, &QWebSocket::disconnected, this, &CollabSession::onClosed);
    m_socks << s;
    // Yeni eşe güncel durumu gönder
    s->sendTextMessage(QString::fromUtf8(
        QJsonDocument(QJsonObject{{"t", "sync"},
                                  {"text", m_base.left(1000000)},
                                  {"rev", m_rev}})
            .toJson()));
}

void CollabSession::onTextMessage(const QString& text) {
    QWebSocket* from = qobject_cast<QWebSocket*>(sender());
    QJsonDocument d = QJsonDocument::fromJson(text.toUtf8());
    if (!d.isObject()) return;
    handleMessage(from, d.object());
}

void CollabSession::onClosed() {
    QWebSocket* s = qobject_cast<QWebSocket*>(sender());
    if (s == m_client) {
        m_client = nullptr;
        if (!m_active) {
            // Henüz bağlanmadan kapandı (hata zaten onWsError ile bildirildi)
            m_pendingUser.clear();
            return;
        }
        leave();
        emit sessionError("Oturum kapandı.");
        return;
    }
    // Stage 49: ani kopuş (bye gelmeden) hayalet bırakmasın
    if (s && m_socks.removeOne(s)) {
        const QString u = m_sockUser.take(s);
        if (!u.isEmpty()) {
            if (m_cursors.remove(u)) emit cursorsChanged();
            emit peerLeft(u);
        }
        s->deleteLater();
    }
}

void CollabSession::handleMessage(QWebSocket* from, const QJsonObject& msg) {
    const QString t = msg.value("t").toString();
    if (!CollabMerge::validType(t)) return;
    const QString user = msg.value("user").toString().left(40);
    if (t == "hello") {
        if (!user.isEmpty() && user != m_user) {
            if (from) m_sockUser[from] = user;
            emit peerJoined(user);
            // Host: diğer eşlere de duyur
            if (m_role == "host") broadcast(msg, from);
        }
        return;
    }
    if (t == "cursor") {
        if (!user.isEmpty()) {
            if (from) m_sockUser[from] = user;
            m_cursors[user] = qMakePair(msg.value("line").toInt(),
                                        msg.value("col").toInt());
            emit cursorsChanged();
            if (m_role == "host") broadcast(msg, from);
        }
        return;
    }
    if (t == "term") {
        emit termChunk(user, msg.value("chunk").toString());
        return;
    }
    if (t == "bye") {
        if (m_cursors.remove(user)) emit cursorsChanged();
        emit peerLeft(user);
        if (m_role == "host") broadcast(msg, from);
        return;
    }
    if (t == "sync") {
        // Eş tarafı: ev sahibinin metnini olduğu gibi al
        if (m_role != "host") {
            m_base = msg.value("text").toString();
            m_rev = msg.value("rev").toInt();
            emit textMerged(m_base, "evsahibi");
        }
        return;
    }
    if (t == "edit") {
        const int base = msg.value("base").toInt();
        auto ops = CollabMerge::parseOps(msg);
        if (ops.isEmpty()) return;
        if (base == m_rev) {
            int nr = m_rev;
            // Yerel işlem yok sayılır (host metni otoriter değil; son-yazan-kazanır)
            m_base = CollabMerge::apply(m_base, ops, {}, nr);
            m_rev = nr;
            emit textMerged(m_base, user);
        } else {
            // Rev uyuşmazlığı: tam eşitleme iste (host'a sor ya da yoksay)
            if (m_role == "host" && from)
                from->sendTextMessage(QString::fromUtf8(
                    QJsonDocument(QJsonObject{{"t", "sync"},
                                              {"text", m_base.left(1000000)},
                                              {"rev", m_rev}})
                        .toJson()));
        }
        if (m_role == "host") broadcast(msg, from);
        return;
    }
}
