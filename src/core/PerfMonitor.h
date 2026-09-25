#pragma once
#include <QList>
#include <QPair>
#include <QString>

// Basit performans ölçümü: adlandırılmış işaretler (başlangıçtan ms) + RSS.
class PerfMonitor {
public:
    static PerfMonitor& instance();

    void reset();                       // başlangıç zamanını şimdi yap
    void mark(const QString& name);     // işaret koy
    qint64 elapsedMs(const QString& name) const;
    qint64 sinceStartMs() const;
    QList<QPair<QString, qint64>> marks() const;
    static qint64 currentRssKb();       // /proc/self/statm (Linux); yoksa -1
    QString summary() const;

private:
    PerfMonitor();
    qint64 m_start = 0;
    QList<QPair<QString, qint64>> m_marks;
};
