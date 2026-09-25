#pragma once
#include <QFuture>
#include <QFutureWatcher>
#include <QMap>
#include <QObject>
#include <QString>

// Stage 19: arka plan iş parçacığı izleyicisi — QFutureWatcher tabanlı.
// Her işin durumu (çalışıyor/bitti/iptal) + iptal desteği.
class ThreadMonitor : public QObject {
    Q_OBJECT
public:
    explicit ThreadMonitor(QObject* parent = nullptr);
    ~ThreadMonitor() override;

    // Bir işi izlemeye al (tipli future; izleyici sahipliği ThreadMonitor'dadır)
    template <typename T> void watch(const QString& id, const QFuture<T>& future) {
        unwatch(id);
        auto* w = new QFutureWatcher<T>(this);
        m_watchers.insert(id, w);
        connect(w, &QFutureWatcher<T>::finished, this, [this, id]() {
            emit statusChanged(id, "bitti");
            emit finished(id);
        });
        connect(w, &QFutureWatcher<T>::canceled, this, [this, id]() {
            emit statusChanged(id, "iptal");
        });
        emit statusChanged(id, "calisiyor");
        w->setFuture(future);
    }

    // İzlemeyi bırak + işi iptal et
    void unwatch(const QString& id);
    // İzlenen işlerin durumu: id → calisiyor|bitti|iptal
    QMap<QString, QString> status() const;
    // Tüm izlenen işleri iptal et
    void cancelAll();

signals:
    void statusChanged(const QString& id, const QString& status);
    void finished(const QString& id);

private:
    QMap<QString, QFutureWatcherBase*> m_watchers;
};
