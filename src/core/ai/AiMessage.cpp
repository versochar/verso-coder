#include "AiMessage.h"

QString AiImage::base64() const {
    return QString::fromLatin1(bytes.toBase64());
}

AiMessage AiMessage::system(const QString& t) {
    AiMessage m;
    m.role = AiRole::System;
    if (!t.trimmed().isEmpty()) m.texts << t;
    return m;
}

AiMessage AiMessage::user(const QString& t) {
    AiMessage m;
    m.role = AiRole::User;
    m.texts << t;
    return m;
}

AiMessage AiMessage::userText(const QString& t, const QList<AiImage>& imgs) {
    AiMessage m = user(t);
    m.images = imgs;
    return m;
}

AiMessage AiMessage::assistant(const QString& t) {
    AiMessage m;
    m.role = AiRole::Assistant;
    m.texts << t;
    return m;
}

AiMessage AiMessage::toolResultMsg(const QString& callId, const QString& n, const QString& out,
                                    bool ok) {
    AiMessage m;
    m.role = AiRole::Tool;
    m.callId = callId;
    m.name = n;
    m.toolOutput = out;
    m.ok = ok;
    return m;
}
