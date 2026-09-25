#include "ThreadMonitor.h"

ThreadMonitor::ThreadMonitor(QObject* parent) : QObject(parent) {}

ThreadMonitor::~ThreadMonitor() {
    qDeleteAll(m_watchers);
    m_watchers.clear();
}

void ThreadMonitor::unwatch(const QString& id) {
    auto it = m_watchers.find(id);
    if (it == m_watchers.end()) return;
    (*it)->cancel();
    (*it)->deleteLater();
    m_watchers.erase(it);
}

QMap<QString, QString> ThreadMonitor::status() const {
    QMap<QString, QString> out;
    for (auto it = m_watchers.constBegin(); it != m_watchers.constEnd(); ++it) {
        const QFutureWatcherBase* w = it.value();
        if (w->isCanceled())
            out[it.key()] = "iptal";
        else if (w->isFinished())
            out[it.key()] = "bitti";
        else
            out[it.key()] = "calisiyor";
    }
    return out;
}

void ThreadMonitor::cancelAll() {
    for (QFutureWatcherBase* w : m_watchers) {
        w->cancel();
        w->deleteLater();
    }
    m_watchers.clear();
}
