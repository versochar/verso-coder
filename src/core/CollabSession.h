#pragma once
#include "CollabMerge.h"
#include <QAbstractSocket>
#include <QMap>
#include <QObject>

class QWebSocket;
class QWebSocketServer;

// Stage 18: canlı işbirliği oturumu — ev sahibi (server) ya da eş (client).
// Aktif belge metni + imleçler paylaşılır; roller: "host" | "peer" | "guest".
class CollabSession : public QObject {
    Q_OBJECT
public:
    explicit CollabSession(QObject* parent = nullptr);
    ~CollabSession() override;

    bool isActive() const { return m_active; }
    QString role() const { return m_role; }
    QString userName() const { return m_user; }
    QStringList peers() const { return m_cursors.keys(); }
    // Eş imleçleri: kullanıcı → (satır, sütun)
    QMap<QString, QPair<int, int>> cursors() const { return m_cursors; }
    int revision() const { return m_rev; }

    // Ev sahipliği: port dinle (0 = rastgele) → gerçek port döner, hata -1
    int host(const QString& user, quint16 port = 0);
    // Katıl: ws://host:port (zamanuyumsuz — bağlanınca activeChanged(true))
    bool join(const QString& url, const QString& user);
    void leave();

    // Giden: belge metni değişti (yerel diff → yayın)
    void publishText(const QString& newText);
    void setBaseText(const QString& text); // rev sıfırla (dosya açılışı)
    void publishCursor(int line, int col);
    void publishTerm(const QString& chunk); // terminal yayını (host)

signals:
    void activeChanged(bool on);
    void textMerged(const QString& text, const QString& fromUser);
    void cursorsChanged();
    void termChunk(const QString& user, const QString& chunk);
    void peerJoined(const QString& user);
    void peerLeft(const QString& user);
    void sessionError(const QString& msg);

private slots:
    void onNewConnection();
    void onConnected();
    void onTextMessage(const QString& text);
    void onClosed();
    void onWsError(QAbstractSocket::SocketError err);

private:
    void handleMessage(QWebSocket* from, const QJsonObject& msg);
    void broadcast(const QJsonObject& msg, QWebSocket* except = nullptr);
    QString m_user;
    QString m_role; // "", "host", "peer", "guest"
    bool m_active = false;
    int m_rev = 0;
    QString m_base; // son mutabık metin
    QMap<QString, QPair<int, int>> m_cursors;
    QWebSocketServer* m_server = nullptr;
    QList<QWebSocket*> m_socks;
    QWebSocket* m_client = nullptr; // peer/guest tarafı tek soket
    QString m_pendingUser;
};
