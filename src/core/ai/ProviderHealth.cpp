#include "ProviderHealth.h"
#include "ProviderPrefs.h"
#include "SecretStore.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QStandardPaths>
#include <algorithm>

qint64 ProviderHealthEntry::uptimeMs() const {
    if (!lastOk.isValid()) return 0;
    if (!lastFail.isValid() || lastFail < lastOk) return lastOk.secsTo(QDateTime::currentDateTime()) * 1000;
    return lastOk.secsTo(lastFail) * 1000;
}

ProviderHealth::ProviderHealth(const QString& file) {
    m_file = file;
    if (m_file.isEmpty()) {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        m_file = dir + "/ai-health.json";
    }
    load();
}

ProviderHealth& ProviderHealth::instance() {
    static ProviderHealth h;
    return h;
}

bool ProviderHealth::load() {
    if (m_file.isEmpty()) return false;
    QFile f(m_file);
    if (!f.open(QIODevice::ReadOnly)) return false;
    m_data = QJsonDocument::fromJson(f.readAll()).object();
    return true;
}

bool ProviderHealth::save() const {
    if (m_file.isEmpty()) return false;
    QDir().mkpath(QFileInfo(m_file).absolutePath());
    QFile f(m_file);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(m_data).toJson(QJsonDocument::Compact));
    return true;
}

void ProviderHealth::write(const QString& id, const ProviderHealthEntry& e) {
    QJsonObject o;
    o["calls"] = e.calls;
    o["errors"] = e.errors;
    o["consec"] = e.consecutiveErrors;
    o["lat"] = double(e.totalLatencyMs);
    o["status"] = e.lastStatus;
    o["busy"] = e.busyFails;
    if (e.openUntil.isValid()) o["openUntil"] = e.openUntil.toString(Qt::ISODate);
    if (e.lastOk.isValid()) o["lastOk"] = e.lastOk.toString(Qt::ISODate);
    if (e.lastFail.isValid()) o["lastFail"] = e.lastFail.toString(Qt::ISODate);
    m_data[id] = o;
    save();
}

ProviderHealthEntry ProviderHealth::read(const QString& id) const {
    ProviderHealthEntry e;
    const QJsonObject o = m_data.value(id).toObject();
    if (o.isEmpty()) return e;
    e.calls = o.value("calls").toInt();
    e.errors = o.value("errors").toInt();
    e.consecutiveErrors = o.value("consec").toInt();
    e.totalLatencyMs = qint64(o.value("lat").toDouble());
    e.lastStatus = o.value("status").toInt();
    e.busyFails = o.value("busy").toInt();
    if (o.contains("openUntil"))
        e.openUntil = QDateTime::fromString(o.value("openUntil").toString(), Qt::ISODate);
    if (o.contains("lastOk")) e.lastOk = QDateTime::fromString(o.value("lastOk").toString(), Qt::ISODate);
    if (o.contains("lastFail"))
        e.lastFail = QDateTime::fromString(o.value("lastFail").toString(), Qt::ISODate);
    return e;
}

void ProviderHealth::record(const QString& providerId, bool ok, int latencyMs, int httpStatus) {
    if (providerId.isEmpty()) return;
    ProviderHealthEntry e = read(providerId);
    e.calls++;
    e.lastStatus = httpStatus;
    if (ok) {
        e.consecutiveErrors = 0;
        if (e.tripped()) e.openUntil = QDateTime(); // başarı → devre kapandı
        e.totalLatencyMs += qMax(0, latencyMs);
        e.lastOk = QDateTime::currentDateTime();
    } else {
        e.errors++;
        e.consecutiveErrors++;
        e.totalLatencyMs += qMax(0, latencyMs);
        e.lastFail = QDateTime::currentDateTime();
        // 429/503/408 → "yoğun" sayılır (sabit hata değil, geçici)
        if (httpStatus == 429 || httpStatus == 503 || httpStatus == 408) {
            ++e.busyFails;
            if (e.busyFails >= busyThreshold()) {
                e.openUntil = QDateTime::currentDateTime().addSecs(cooldownSec());
                e.busyFails = 0;
            }
        }
    }
    write(providerId, e);
}

void ProviderHealth::recordSuccess(const QString& providerId, int latencyMs) {
    record(providerId, true, latencyMs, 200);
}

void ProviderHealth::recordFailure(const QString& providerId, int latencyMs, int httpStatus) {
    record(providerId, false, latencyMs, httpStatus);
}

ProviderHealthEntry ProviderHealth::entry(const QString& providerId) const {
    return read(providerId);
}

double ProviderHealth::errorRate(const QString& providerId) const {
    return read(providerId).errorRate();
}

bool ProviderHealth::isHealthy(const QString& providerId) const {
    return read(providerId).healthy();
}

void ProviderHealth::reset(const QString& providerId) {
    if (providerId.isEmpty()) {
        m_data = QJsonObject();
    } else {
        m_data.remove(providerId);
    }
    save();
}

QStringList ProviderHealth::tracked() const {
    QStringList out = m_data.keys();
    out.sort();
    return out;
}

bool ProviderHealth::isEquivalent(const ProviderSpec& a, const ProviderSpec& b,
                                  const QString& requiredModel) {
    if (a.id.isEmpty() || b.id.isEmpty() || a.id == b.id) return false;
    // Gövde/araç desteği eşdeğer olmalı
    if (a.supportsTools != b.supportsTools) return false;
    if (a.supportsVision != b.supportsVision) return false;
    if (requiredModel.contains("embed")) {
        if (!b.supportsEmbed) return false;   // yedek de gömme yapabilmeli
    }
    // Anahtarı olmayan ücretli sağlayıcı yedek olamaz
    if (b.requiresKey()) {
        SecretStore store;
        if (store.effectiveKey(b.id).isEmpty()) return false;
    }
    // Aynı tür tercih edilir ama farklı tür de olabilir
    return true;
}

QStringList ProviderHealth::failoverCandidates(const QString& failedProviderId,
                                               const QList<ProviderSpec>& pool,
                                               const QString& requiredModel) {
    const ProviderSpec failed = ProviderRegistry::byId(failedProviderId);
    struct Cand {
        QString id;
        int score;
    };
    QList<Cand> cands;
    for (const ProviderSpec& s : pool) {
        if (s.id == failedProviderId) continue;
        if (failed.id.isEmpty() || !isEquivalent(failed, s, requiredModel)) continue;
        const ProviderHealthEntry e = ProviderHealth::instance().entry(s.id);
        int score = 0;
        if (e.healthy()) score += 1000;                 // sağlıklı olan önce
        score -= int(e.errorRate() * 500);               // hata oranı cezası
        score -= int(e.avgLatencyMs() / 200);            // hızlı olan önce
        if (s.kind == failed.kind && !failed.id.isEmpty()) score += 200;
        if (s.kind == ProviderKind::Ollama) score += 150; // yerel = ücretsiz/erken
        if (!s.supportsVision) score -= 50;              // yetersiz yetenek cezası
        cands.append({s.id, score});
    }
    std::sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) {
        return a.score != b.score ? a.score > b.score : a.id < b.id;
    });
    QStringList out;
    for (const Cand& c : cands) out << c.id;
    return out;
}

QString ProviderHealth::pickFailover(const QString& failedProviderId, const QList<ProviderSpec>& pool,
                                     const QString& requiredModel) {
    return failoverCandidates(failedProviderId, pool, requiredModel).value(0);
}

QString ProviderHealth::verdict(const ProviderHealthEntry& e) {
    if (e.calls == 0) return "bilinmiyor";
    if (!e.healthy()) return "bozuk";
    if (e.errorRate() < 0.02) return "sağlıklı";
    if (e.errorRate() < 0.10) return "kararsız";
    return "zayıf";
}

QString ProviderHealth::statusLine(const QString& providerId) const {
    const ProviderHealthEntry e = entry(providerId);
    if (e.calls == 0) return "henüz istek yok";
    return QString("%1 · %2/%3 hata · ~%4 ms")
        .arg(verdict(e))
        .arg(e.errors)
        .arg(e.calls)
        .arg(e.avgLatencyMs());
}

void ProviderHealth::recordBusy(const QString& providerId) {
    if (providerId.isEmpty()) return;
    ProviderHealthEntry e = read(providerId);
    ++e.busyFails;
    if (e.busyFails >= busyThreshold()) {
        e.openUntil = QDateTime::currentDateTime().addSecs(cooldownSec());
        e.busyFails = 0; // sayaç sıfırlanır, açık devre durumu korunur
    }
    write(providerId, e);
}

void ProviderHealth::clearTripped(const QString& providerId) {
    if (providerId.isEmpty()) return;
    ProviderHealthEntry e = read(providerId);
    const bool was = e.tripped();
    e.openUntil = QDateTime();
    e.busyFails = 0;
    if (was) write(providerId, e);
}

bool ProviderHealth::isTripped(const QString& providerId) const {
    return read(providerId).tripped();
}

int ProviderHealth::cooldownLeft(const QString& providerId) const {
    const ProviderHealthEntry e = read(providerId);
    if (!e.openUntil.isValid()) return 0;
    const qint64 ms = QDateTime::currentDateTime().msecsTo(e.openUntil);
    return ms <= 0 ? 0 : int(ms / 1000);
}

bool ProviderHealth::isUsable(const QString& providerId) const {
    return !read(providerId).tripped();
}

QStringList ProviderHealth::filterUsable(const QStringList& providerIds) const {
    QStringList out;
    for (const QString& id : providerIds)
        if (isUsable(id)) out << id;
    return out;
}
