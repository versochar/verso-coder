#include "RagProgress.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

int RagProgress::percent() const {
    if (total <= 0) return 0;
    return qBound(0, int((double(indexed) / double(total)) * 100.0), 100);
}

QString RagProgress::describe() const {
    if (root.isEmpty()) return QStringLiteral("indeksleme yok");
    if (waiting) return QStringLiteral("beklemede (hız sınırı) — %1/%2 parça")
                        .arg(indexed).arg(total);
    if (pending.isEmpty()) return QStringLiteral("tamamlandı: %1 parça").arg(indexed);
    return QStringLiteral("%1/%2 parça · %3 dosya sırada")
        .arg(indexed).arg(total).arg(int(pending.size()));
}

RagProgressStore::RagProgressStore(const QString& file) {
    m_file = file;
    if (m_file.isEmpty()) {
        m_file = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                 + QStringLiteral("/rag-progress.json");
    }
}

bool RagProgressStore::load() {
    QFile f(m_file);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
    if (o.isEmpty()) return false;
    RagProgress p;
    p.root = o.value("root").toString();
    for (const QJsonValue& v : o.value("pending").toArray()) p.pending << v.toString();
    p.indexed = o.value("indexed").toInt();
    p.total = o.value("total").toInt();
    p.waiting = o.value("waiting").toBool();
    p.waitUntilMs = qint64(o.value("waitUntil").toDouble());
    m_progress = p;
    return true;
}

bool RagProgressStore::save() const {
    if (m_file.isEmpty()) return false;
    QDir().mkpath(QFileInfo(m_file).absolutePath());
    QJsonObject o{{"root", m_progress.root},
                  {"pending", QJsonArray::fromStringList(m_progress.pending)},
                  {"indexed", m_progress.indexed},
                  {"total", m_progress.total},
                  {"waiting", m_progress.waiting},
                  {"waitUntil", double(m_progress.waitUntilMs)},
                  {"at", QDateTime::currentDateTime().toString(Qt::ISODate)}};
    QFile f(m_file);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(o).toJson(QJsonDocument::Compact));
    return true;
}

void RagProgressStore::clear() {
    m_progress = RagProgress();
    QFile::remove(m_file);
}

void RagProgressStore::setProgress(const RagProgress& p) {
    m_progress = p;
    save();
}

void RagProgressStore::markWaiting(qint64 untilEpochMs) {
    m_progress.waiting = true;
    m_progress.waitUntilMs = untilEpochMs > 0
                                ? untilEpochMs
                                : QDateTime::currentDateTime()
                                      .addSecs(60)
                                      .toMSecsSinceEpoch();
    save();
}

void RagProgressStore::clearWaiting() {
    if (!m_progress.waiting) return;
    m_progress.waiting = false;
    m_progress.waitUntilMs = 0;
    save();
}

bool RagProgressStore::isWaiting(qint64 nowEpochMs) const {
    if (!m_progress.waiting) return false;
    if (m_progress.waitUntilMs <= 0) return true;
    const qint64 now = nowEpochMs > 0 ? nowEpochMs : QDateTime::currentDateTime().toMSecsSinceEpoch();
    return now < m_progress.waitUntilMs;
}

int RagProgressStore::waitLeftSec(qint64 nowEpochMs) const {
    if (!isWaiting(nowEpochMs)) return 0;
    const qint64 now = nowEpochMs > 0 ? nowEpochMs : QDateTime::currentDateTime().toMSecsSinceEpoch();
    return int(qMax<qint64>(0, (m_progress.waitUntilMs - now) / 1000));
}

void RagProgressStore::advance(const QString& fileDone) {
    if (m_progress.pending.isEmpty()) return;
    m_progress.pending.removeOne(fileDone);
    ++m_progress.indexed;
    save();
}

bool RagProgressStore::matchesRoot(const QString& root) const {
    if (m_progress.root.isEmpty()) return false;
    return QDir::cleanPath(m_progress.root) == QDir::cleanPath(root);
}
