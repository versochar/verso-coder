#include "SseParser.h"

void SseParser::clear() {
    m_buffer.clear();
    m_events.clear();
    m_done = false;
}

void SseParser::pushEvent(const QString& payload) {
    const QString p = payload.trimmed();
    if (p.isEmpty()) return;
    if (p == "[DONE]") {
        m_done = true;
        return;
    }
    m_events << p;
}

void SseParser::feed(const QByteArray& raw) {
    m_buffer += QString::fromUtf8(raw);
    // Satır sonu normalizasyonu
    m_buffer.replace("\r\n", "\n");
    m_buffer.replace('\r', '\n');
    int idx;
    // Boş satır olay sınırıdır
    while ((idx = m_buffer.indexOf("\n\n")) >= 0) {
        const QString block = m_buffer.left(idx);
        m_buffer = m_buffer.mid(idx + 2);
        QStringList dataLines;
        for (const QString& line : block.split('\n')) {
            const QString l = line.trimmed();
            if (l.startsWith("data:")) dataLines << l.mid(5).trimmed();
        }
        if (!dataLines.isEmpty()) pushEvent(dataLines.join('\n'));
    }
}

void SseParser::flush() {
    if (m_buffer.trimmed().isEmpty()) {
        m_buffer.clear();
        return;
    }
    QStringList dataLines;
    for (const QString& line : m_buffer.split('\n')) {
        const QString l = line.trimmed();
        if (l.startsWith("data:")) dataLines << l.mid(5).trimmed();
    }
    m_buffer.clear();
    if (!dataLines.isEmpty()) pushEvent(dataLines.join('\n'));
}
