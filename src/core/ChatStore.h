#pragma once
#include <QDateTime>
#include <QList>
#include <QString>

struct ChatMessage {
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

private:
    QString filePath(const QString& id) const;
    QString m_dir;
};
