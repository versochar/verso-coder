#include "PerfMonitor.h"
#include <QDateTime>
#include <QFile>

PerfMonitor& PerfMonitor::instance() {
    static PerfMonitor m;
    return m;
}

PerfMonitor::PerfMonitor() {
    m_start = QDateTime::currentMSecsSinceEpoch();
}

void PerfMonitor::reset() {
    m_start = QDateTime::currentMSecsSinceEpoch();
    m_marks.clear();
}

void PerfMonitor::mark(const QString& name) {
    m_marks << qMakePair(name, QDateTime::currentMSecsSinceEpoch());
}

qint64 PerfMonitor::elapsedMs(const QString& name) const {
    for (const auto& m : m_marks)
        if (m.first == name) return m.second - m_start;
    return -1;
}

qint64 PerfMonitor::sinceStartMs() const {
    return QDateTime::currentMSecsSinceEpoch() - m_start;
}

QList<QPair<QString, qint64>> PerfMonitor::marks() const {
    QList<QPair<QString, qint64>> out;
    for (const auto& m : m_marks) out << qMakePair(m.first, m.second - m_start);
    return out;
}

qint64 PerfMonitor::currentRssKb() {
    QFile f("/proc/self/statm");
    if (!f.open(QIODevice::ReadOnly)) return -1;
    const QList<QByteArray> parts = f.readAll().trimmed().split(' ');
    if (parts.size() < 2) return -1;
    // statm: toplam sayfa, yerleşik sayfa, ... (sayfa = 4 KB varsayımı)
    return parts[1].toLongLong() * 4;
}

QString PerfMonitor::summary() const {
    QString s;
    s += QString("Başlangıçtan bu yana: %1 ms\n").arg(sinceStartMs());
    for (const auto& m : marks())
        s += QString("  • %1: %2 ms\n").arg(m.first.leftJustified(22, ' ')).arg(m.second);
    const qint64 rss = currentRssKb();
    if (rss >= 0) s += QString("Bellek (RSS): %1 MB\n").arg(rss / 1024.0, 0, 'f', 1);
    return s;
}
