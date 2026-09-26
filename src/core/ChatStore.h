#pragma once
#include <QDateTime>
#include <QList>
#include <QString>

struct ChatMessage {
    QString id;       // Stage 33: oturum içi benzersiz kimlik (dallanma için)
    QString parentId; // Stage 33: "" = kök, aksi mesaj kimliği
    QString role; // "user" | "ai" | "system"
    QString text;
    qint64 whenMs = 0;
};

struct ChatSession {
    QString id;
    QString title;
    QList<ChatMessage> messages;
    qint64 updatedMs = 0;
};

// Çoklu sohbet oturumu deposu: her oturum <dir>/<id>.json dosyasında saklanır.
class ChatStore {
public:
    explicit ChatStore(const QString& dir);

    QString createSession(const QString& title = QString());
    QStringList sessionIds() const;          // güncelleme zamanına göre (yeni→eski)
    QList<ChatSession> sessions() const;      // meta (mesajlar dahil)
    ChatSession load(const QString& id) const;
    bool save(const ChatSession& s);
    bool remove(const QString& id);
    bool rename(const QString& id, const QString& title);
    void append(const QString& id, const ChatMessage& m);
    QString titleFor(const QString& id) const;
    QString dir() const { return m_dir; }

    // --- Stage 33: sohbet dallanma (ağaç) ---
    static QString newMessageId();
    // parentId'ye bağlı yeni mesaj ekler; atanan mesaj kimliğini döner.
    QString appendMsg(const QString& sessionId, const QString& parentId, const ChatMessage& m);
    // leafId'den köke kadar yol (kronolojik sıra).
    static QList<ChatMessage> pathTo(const ChatSession& s, const QString& leafId);
    // parentId'nin doğrudan çocukları ("" = kökler).
    static QList<ChatMessage> childrenOf(const ChatSession& s, const QString& parentId);
    // Ağaç yaprakları (dallanma uçları).
    static QStringList leafIds(const ChatSession& s);

private:
    QString filePath(const QString& id) const;
    QString m_dir;
};
