#include "ChatStore.h"
#include <QDir>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <algorithm>

ChatStore::ChatStore(const QString& dir) : m_dir(dir) {
    QDir().mkpath(m_dir);
}

QString ChatStore::filePath(const QString& id) const {
    return m_dir + "/" + id + ".json";
}

QString ChatStore::createSession(const QString& title) {
    QString id = QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss-zzz");
    QString base = id;
    int n = 1;
    while (QFile::exists(filePath(id))) id = base + "-" + QString::number(++n); // ms çakışmasını önle
    ChatSession s;
    s.id = id;
    s.title = title.isEmpty() ? QString("Sohbet %1").arg(
        QDateTime::currentDateTime().toString("dd.MM HH:mm")) : title;
    s.updatedMs = QDateTime::currentMSecsSinceEpoch();
    save(s);
    return id;
}

bool ChatStore::save(const ChatSession& s) {
    QJsonObject o;
    o["id"] = s.id;
    o["title"] = s.title;
    o["updated"] = s.updatedMs;
    QJsonArray arr;
    for (const ChatMessage& m : s.messages)
        arr.append(QJsonObject{{"id", m.id},
                               {"parent", m.parentId},
                               {"role", m.role},
                               {"text", m.text},
                               {"when", m.whenMs}});
    o["messages"] = arr;
    QFile f(filePath(s.id));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    return true;
}

ChatSession ChatStore::load(const QString& id) const {
    ChatSession s;
    QFile f(filePath(id));
    if (!f.open(QIODevice::ReadOnly)) return s;
    QJsonDocument d = QJsonDocument::fromJson(f.readAll());
    QJsonObject o = d.object();
    s.id = o.value("id").toString(id);
    s.title = o.value("title").toString();
    s.updatedMs = qint64(o.value("updated").toDouble());
    for (const QJsonValue& v : o.value("messages").toArray()) {
        QJsonObject mo = v.toObject();
        ChatMessage m;
        m.id = mo.value("id").toString();
        m.parentId = mo.value("parent").toString();
        m.role = mo.value("role").toString();
        m.text = mo.value("text").toString();
        m.whenMs = qint64(mo.value("when").toDouble());
        if (m.id.isEmpty()) m.id = QString("m%1").arg(s.messages.size()); // eski kayıt
        s.messages << m;
    }
    return s;
}

bool ChatStore::remove(const QString& id) {
    return QFile::remove(filePath(id));
}

bool ChatStore::rename(const QString& id, const QString& title) {
    ChatSession s = load(id);
    if (s.id.isEmpty() && !QFile::exists(filePath(id))) return false;
    s.title = title;
    s.updatedMs = QDateTime::currentMSecsSinceEpoch();
    return save(s);
}

void ChatStore::append(const QString& id, const ChatMessage& m) {
    ChatSession s = load(id);
    s.id = id;
    if (s.title.isEmpty()) s.title = m.text.left(40);
    ChatMessage msg = m;
    if (msg.id.isEmpty()) msg.id = newMessageId();
    // Stage 33: doğrusal eklemede üst = son mesaj
    if (msg.parentId.isEmpty() && !s.messages.isEmpty())
        msg.parentId = s.messages.last().id;
    s.messages << msg;
    s.updatedMs = QDateTime::currentMSecsSinceEpoch();
    save(s);
}

// --- Stage 33: dallanma yardımcıları ---

static int g_msgSeq = 0;

QString ChatStore::newMessageId() {
    return QString("m%1-%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(++g_msgSeq);
}

QString ChatStore::appendMsg(const QString& sessionId, const QString& parentId,
                             const ChatMessage& m) {
    ChatSession s = load(sessionId);
    s.id = sessionId;
    if (s.title.isEmpty()) s.title = m.text.left(40);
    ChatMessage msg = m;
    msg.parentId = parentId;
    if (msg.id.isEmpty()) msg.id = newMessageId();
    s.messages << msg;
    s.updatedMs = QDateTime::currentMSecsSinceEpoch();
    save(s);
    return msg.id;
}

QList<ChatMessage> ChatStore::pathTo(const ChatSession& s, const QString& leafId) {
    QHash<QString, ChatMessage> byId;
    for (const ChatMessage& m : s.messages) byId.insert(m.id, m);
    QList<ChatMessage> rev;
    QString cur = leafId;
    int guard = 0;
    while (!cur.isEmpty() && byId.contains(cur) && guard++ < 10000) {
        const ChatMessage m = byId.value(cur);
        rev << m;
        cur = m.parentId;
    }
    std::reverse(rev.begin(), rev.end());
    return rev;
}

QList<ChatMessage> ChatStore::childrenOf(const ChatSession& s, const QString& parentId) {
    QList<ChatMessage> out;
    for (const ChatMessage& m : s.messages)
        if (m.parentId == parentId) out << m;
    return out;
}

QStringList ChatStore::leafIds(const ChatSession& s) {
    QSet<QString> parents;
    for (const ChatMessage& m : s.messages)
        if (!m.parentId.isEmpty()) parents.insert(m.parentId);
    QStringList out;
    for (const ChatMessage& m : s.messages)
        if (!parents.contains(m.id)) out << m.id;
    return out;
}

QString ChatStore::titleFor(const QString& id) const {
    return load(id).title;
}

QStringList ChatStore::sessionIds() const {
    QStringList ids;
    QDir d(m_dir);
    for (const QString& f : d.entryList({"*.json"}, QDir::Files))
        ids << f.left(f.size() - 5);
    // updated'e göre sırala
    std::sort(ids.begin(), ids.end(), [this](const QString& a, const QString& b) {
        const qint64 ua = load(a).updatedMs, ub = load(b).updatedMs;
        if (ua != ub) return ua > ub;
        return a > b; // eşit zaman damgasında kimliğe göre azalan (deterministik)
    });
    return ids;
}

QList<ChatSession> ChatStore::sessions() const {
    QList<ChatSession> out;
    for (const QString& id : sessionIds()) out << load(id);
    return out;
}
