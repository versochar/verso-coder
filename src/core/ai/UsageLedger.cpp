#include "UsageLedger.h"
#include "ProviderPricing.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QStandardPaths>

UsageLedger::UsageLedger(const QString& file) {
    m_file = file;
    if (m_file.isEmpty()) {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        m_file = dir + "/ai-usage.json";
    }
    load();
}

UsageLedger& UsageLedger::instance() {
    static UsageLedger s;
    return s;
}

QString UsageLedger::dayKey(const QDate& d) { return d.toString(Qt::ISODate); }

bool UsageLedger::load() {
    if (m_file.isEmpty()) return false;
    QFile f(m_file);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    m_days = root.value("days").toObject();
    m_quota = root.value("quota").toObject();
    return true;
}

bool UsageLedger::save() const {
    if (m_file.isEmpty()) return false;
    QDir().mkpath(QFileInfo(m_file).absolutePath());
    QFile f(m_file);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    const QJsonObject root{{"days", m_days}, {"quota", m_quota}};
    f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    return true;
}

void UsageLedger::recordWithCost(const QString& providerId, const QString& model, int prompt,
                                 int eval, double usd) {
    const QString dk = dayKey();
    QJsonObject byProv = m_days.value(dk).toObject();
    QJsonObject e = byProv.value(providerId).toObject();
    e["calls"] = e.value("calls").toInt() + 1;
    e["prompt"] = double(e.value("prompt").toDouble()) + qMax(0, prompt);
    e["eval"] = double(e.value("eval").toDouble()) + qMax(0, eval);
    e["usd"] = e.value("usd").toDouble() + qMax(0.0, usd);
    byProv[providerId] = e;
    m_days[dk] = byProv;
    save();
}

void UsageLedger::record(const QString& providerId, const QString& model, int prompt, int eval,
                         int cached) {
    const ProviderSpec spec = ProviderRegistry::byId(providerId);
    const double usd = ProviderPricing::estimateUsd(spec, model, prompt, eval, cached);
    recordWithCost(providerId, model, prompt, eval, usd);
}

UsageLedger::Day UsageLedger::day(const QString& isoDate, const QString& providerId) const {
    Day d;
    const QJsonObject byProv = m_days.value(isoDate).toObject();
    if (providerId.isEmpty()) {
        for (auto it = byProv.begin(); it != byProv.end(); ++it) {
            const QJsonObject e = it.value().toObject();
            d.calls += e.value("calls").toInt();
            d.prompt += qint64(e.value("prompt").toDouble());
            d.eval += qint64(e.value("eval").toDouble());
            d.usd += e.value("usd").toDouble();
        }
        return d;
    }
    const QJsonObject e = byProv.value(providerId).toObject();
    d.calls = e.value("calls").toInt();
    d.prompt = qint64(e.value("prompt").toDouble());
    d.eval = qint64(e.value("eval").toDouble());
    d.usd = e.value("usd").toDouble();
    return d;
}

UsageLedger::Day UsageLedger::today(const QString& providerId) const {
    return day(dayKey(), providerId);
}

QList<QPair<QString, UsageLedger::Day>> UsageLedger::lastDays(int n,
                                                              const QString& providerId) const {
    QList<QPair<QString, Day>> out;
    if (n <= 0) return out;
    const QDate today = QDate::currentDate();
    for (int i = 0; i < n; ++i) {
        const QDate d = today.addDays(-i);
        const QString key = dayKey(d);
        if (!providerId.isEmpty() && !m_days.value(key).toObject().contains(providerId)) continue;
        if (m_days.value(key).toObject().isEmpty()) continue;
        out.append({key, day(key, providerId)});
    }
    return out;
}

QList<QPair<QString, UsageLedger::Day>> UsageLedger::lastDays(int n) const {
    return lastDays(n, QString());
}

int UsageLedger::dayCount() const { return m_days.size(); }

QStringList UsageLedger::providers() const {
    QStringList out;
    const QJsonObject today = m_days.value(dayKey()).toObject();
    for (auto it = today.begin(); it != today.end(); ++it) out << it.key();
    out.sort();
    return out;
}

void UsageLedger::setQuota(const QString& providerId, const Quota& q) {
    if (providerId.isEmpty()) return;
    QJsonObject e;
    e["maxCalls"] = q.maxCalls;
    e["maxTokens"] = double(q.maxTokens);
    if (q.limited())
        m_quota[providerId] = e;
    else
        m_quota.remove(providerId);
    save();
}

UsageLedger::Quota UsageLedger::quota(const QString& providerId) const {
    Quota q;
    const QJsonObject e = m_quota.value(providerId).toObject();
    if (e.isEmpty()) return q;
    q.maxCalls = e.value("maxCalls").toInt();
    q.maxTokens = qint64(e.value("maxTokens").toDouble());
    return q;
}

bool UsageLedger::quotaExceeded(const QString& providerId, QString& why) const {
    why.clear();
    const Quota q = quota(providerId);
    if (!q.limited()) return false;
    const Day d = today(providerId);
    if (q.maxCalls > 0 && d.calls >= q.maxCalls) {
        why = QString("Günlük istek kotası doldu (%1/%2)").arg(d.calls).arg(q.maxCalls);
        return true;
    }
    if (q.maxTokens > 0 && d.total() >= q.maxTokens) {
        why = QString("Günlük token kotası doldu (%1/%2)")
                  .arg(d.total())
                  .arg(q.maxTokens);
        return true;
    }
    return false;
}

int UsageLedger::remainingCalls(const QString& providerId) const {
    const Quota q = quota(providerId);
    if (q.maxCalls <= 0) return -1;
    return qMax(0, q.maxCalls - today(providerId).calls);
}

qint64 UsageLedger::remainingTokens(const QString& providerId) const {
    const Quota q = quota(providerId);
    if (q.maxTokens <= 0) return -1;
    return qMax<qint64>(0, q.maxTokens - today(providerId).total());
}

void UsageLedger::clearToday(const QString& providerId) {
    const QString dk = dayKey();
    QJsonObject byProv = m_days.value(dk).toObject();
    if (providerId.isEmpty())
        m_days.remove(dk);
    else {
        byProv.remove(providerId);
        if (byProv.isEmpty())
            m_days.remove(dk);
        else
            m_days[dk] = byProv;
    }
    save();
}

int UsageLedger::prune(int keepDays) {
    if (keepDays <= 0) return 0;
    const QDate cutoff = QDate::currentDate().addDays(-keepDays);
    int removed = 0;
    const QStringList keys = m_days.keys();
    for (const QString& k : keys) {
        const QDate d = QDate::fromString(k, Qt::ISODate);
        if (!d.isValid() || d < cutoff) {
            m_days.remove(k);
            ++removed;
        }
    }
    if (removed) save();
    return removed;
}

void UsageLedger::reset() {
    m_days = QJsonObject();
    m_quota = QJsonObject();
    save();
}
