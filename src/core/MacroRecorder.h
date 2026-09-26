#pragma once
#include <QSettings>
#include <QString>
#include <QStringList>

// Stage 25: klavye makrosu — KOMUT dizisi kaydeder/oynatır (tuş değil,
// komut-id'leri; çökmeye dayanıklı). Saf liste mantığı (test edilebilir).
class MacroRecorder {
public:
    void start() { m_rec.clear(); m_on = true; }
    void stop() { m_on = false; }
    bool recording() const { return m_on; }
    void push(const QString& cmdId); // kayıt açıksa ve makro-komutu değilse ekler
    QStringList commands() const { return m_rec; }
    void clear() { m_rec.clear(); }
    void save() const; // QSettings "macro/last"
    void load();

private:
    static bool isMacroCmd(const QString& id);
    QStringList m_rec;
    bool m_on = false;
};
