#pragma once
#include <QList>
#include <QString>

// Stage 14: kesme noktaları deposu (QSettings kalıcı, dosya başına satırlar).
struct Breakpoint {
    QString file;
    int line = 1; // 1-based
    bool enabled = true;
    QString condition;
    QString log; // log noktası mesajı (boşsa normal bp)
    int hitCount = 0; // Stage 26: N vuruşta bir dur (0 = her sefer)
    bool isLogPoint() const { return !log.isEmpty(); }
};

class BreakpointStore {
public:
    QList<Breakpoint> all() const;
    QList<Breakpoint> forFile(const QString& file) const;
    bool has(const QString& file, int line) const;
    // Varsa kaldır (true), yoksa ekle (false) — arayüz tıklaması
    bool toggle(const QString& file, int line);
    void setEnabled(const QString& file, int line, bool on);
    void setCondition(const QString& file, int line, const QString& cond);
    void setHitCount(const QString& file, int line, int n); // Stage 26
    void remove(const QString& file, int line);
    void clearFile(const QString& file);
    void clearAll();

private:
    void load(QList<Breakpoint>& out) const;
    void save(const QList<Breakpoint>& bps) const;
};
