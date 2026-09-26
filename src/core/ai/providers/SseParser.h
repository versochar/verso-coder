#pragma once
#include <QByteArray>
#include <QString>
#include <QStringList>

// Stage 35: OpenAI/Gemini/UnoRouter tarzı SSE akış ayrıştırıcı.
// Parça sınırları bölünmüş olabilir; çok satırlı `data:` alanları ve
// [DONE] sonlayıcısı desteklenir. Ağdan bağımsız, saf, test edilebilir.
class SseParser {
public:
    SseParser() = default;
    // Ham parçayı besle; tamamlanan "olay" gövdeleri events() listesine eklenir.
    void feed(const QByteArray& raw);
    // Dizi sabitleri için (QByteArray/QString belirsizliğini önler)
    void feed(const char* raw) { feed(QByteArray(raw)); }
    void feed(const QString& raw) { feed(raw.toUtf8()); }
    // Bekleyen son olay varsa onu da verir (bağlantı kapandığında).
    void flush();
    QStringList events() const { return m_events; }
    bool isDone() const { return m_done; }
    int eventCount() const { return m_events.size(); }
    void clear();

private:
    void pushEvent(const QString& payload);

    QString m_buffer;
    QStringList m_events;
    bool m_done = false;
};
