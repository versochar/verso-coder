#include "LeakWatch.h"
#include <QCoreApplication>
#include <QFile>
#include <QLocale>
#include <QThread>

namespace {
double g_thresholdKb = 64.0;
}

LeakWatch::LeakWatch(const QString& label, int iterations)
    : m_label(label), m_target(iterations) {
    reset();
}

double LeakWatch::rssMb() {
    // Linux: /proc/self/statm (sayfa × sayfa boyutu) — hızlı ve ayrıcalıksız
    QFile f(QStringLiteral("/proc/self/statm"));
    if (!f.open(QIODevice::ReadOnly)) return 0.0;
    const QByteArray line = f.readLine();
    f.close();
    const QList<QByteArray> parts = line.split(' ');
    if (parts.size() < 2) return 0.0;
    bool ok = false;
    const qint64 pages = parts.at(1).toLongLong(&ok);
    if (!ok || pages <= 0) return 0.0;
    // Sayfa boyutu 4 KiB (x86_64 ve aarch64 Linux)
    return double(pages) * 4096.0 / (1024.0 * 1024.0);
}

void LeakWatch::reset() {
    m_iterations = 0;
    m_start = rssMb();
    m_end = m_start;
    m_peak = m_start;
    m_prev = m_start;
    m_worst = 0.0;
}

void LeakWatch::tick() {
    ++m_iterations;
    m_prev = m_end;
    m_end = rssMb();
    if (m_end > m_peak) m_peak = m_end;
    if (m_start > 0.0 && m_prev > 0.0) {
        const double deltaKb = (m_end - m_prev) * 1024.0;
        if (deltaKb > m_worst) m_worst = deltaKb;
    }
}

LeakWatch::Result LeakWatch::result() const {
    Result r;
    r.available = m_start > 0.0;
    r.iterations = m_iterations;
    r.startMb = m_start;
    r.peakMb = m_peak;
    r.endMb = m_end;
    if (!r.available) {
        r.verdict = QStringLiteral("RSS ölçülemiyor (bu sistemde /proc yok)");
        return r;
    }
    r.growthMb = m_end - m_start;
    r.growthPerIterKb = m_iterations > 0 ? r.growthMb * 1024.0 / m_iterations : 0.0;
    r.leaky = m_iterations > 0 && m_worst > growthThresholdKb() && r.growthPerIterKb > 4.0;
    QString v;
    if (m_iterations == 0)
        v = QStringLiteral("ölçüm yok");
    else if (r.leaky)
        v = QStringLiteral("şüpheli sızıntı: en büyük tur artışı %1 KB")
                .arg(m_worst, 0, 'f', 0);
    else
        v = QStringLiteral("sabit: tur başına %1 KB (en çok %2 KB)")
                .arg(r.growthPerIterKb, 0, 'f', 1)
                .arg(m_worst, 0, 'f', 0);
    if (!m_label.isEmpty()) v = m_label + QStringLiteral(": ") + v;
    r.verdict = v;
    return r;
}

double LeakWatch::growthThresholdKb() { return g_thresholdKb; }

void LeakWatch::setGrowthThresholdKb(double kb) { g_thresholdKb = qMax(1.0, kb); }
