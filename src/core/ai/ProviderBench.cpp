#include "ProviderBench.h"
#include "LlmProvider.h"
#include "ProviderPricing.h"
#include <QSet>
#include <algorithm>
#include <cmath>

QList<BenchTask> ProviderBench::tasks() {
    QList<BenchTask> out;
    out << BenchTask{"explain", "Kısa açıklama",
                     "Şu C++ kodunun ne yaptığını iki cümlede anlat: "
                     "int f(const std::vector<int>& v){int s=0;for(int x:v) if(x%2)s+=x; return s;}",
                     {"tek", "çift", "toplam", "vektör"}, 1, false};
    out << BenchTask{"bug", "Hata bulma",
                     "Şu kodda bellek sızıntısı var mı? Neden? "
                     "char* p = new char[10]; p[10] = 0; delete[] p; return p[0];",
                     {"taşma", "out of bounds", "sınır", "10", "undefined"}, 2, false};
    out << BenchTask{"refactor", "Yeniden yazım",
                     "Şu fonksiyonu hata denetimli hâle getir ve tek bir kod bloğu içinde yaz: "
                     "int parse(const char* s){ return atoi(s); }",
                     {"nullptr", "hata", "return", "strtol", "geçersiz"}, 2, true};
    out << BenchTask{"test", "Test yazma",
                     "add(int a, int b) için 3 birim testi yaz (çerçeve dili: pytest benzeri).",
                     {"assert", "test", "toplam", "sıfır"}, 2, true};
    out << BenchTask{"reason", "Gerekçelendirme",
                     "Bu kod neden yavaş? n=1e6; for(int i=0;i<n;i++) a[i] = sqrt(a[i])*2;",
                     {"bellek", "önbellek", "karmaşık", "cache"}, 3, false};
    return out;
}

BenchTask ProviderBench::taskById(const QString& id) {
    for (const BenchTask& t : tasks())
        if (t.id == id) return t;
    return {};
}

int ProviderBench::tokensEstimate(const QString& text) {
    // Kaba tahmin: ~4 karakter/token (Stage 15 ile aynı yaklaşım)
    return qMax(1, text.size() / 4);
}

double ProviderBench::qualityOf(const QString& taskId, const QString& answer) {
    const BenchTask t = taskById(taskId);
    if (t.id.isEmpty()) return 0.0;
    const QString a = answer.trimmed();
    if (a.isEmpty()) return 0.0;
    double score = 25.0; // boş değilse taban puan
    // Anahtar kelime eşleşmesi (%55)
    const QString low = a.toLower();
    int hits = 0;
    for (const QString& k : t.keywords)
        if (low.contains(k.toLower())) ++hits;
    if (!t.keywords.isEmpty())
        score += 55.0 * double(hits) / double(t.keywords.size());
    else
        score += 55.0;
    // Uzunluk uygunluğu (%12): çok kısa ya da aşırı uzun cezalanır
    const int len = a.size();
    if (len < 40)
        score += 12.0 * double(len) / 40.0;
    else if (len <= 1200)
        score += 12.0;
    else
        score += 12.0 * qMax(0.0, 1.0 - double(len - 1200) / 4000.0);
    // Kod isteniyorsa kod bloğu olmalı (%8)
    if (t.wantsCode)
        score += (a.contains("```") || a.contains('{') || a.contains("(")) ? 8.0 : 0.0;
    else
        score += 8.0;
    // Zorluk cezası: zor görevde eksik anahtar kelime daha ağır
    if (t.difficulty >= 3 && hits < t.keywords.size()) score -= 5.0;
    return qBound(0.0, score, 100.0);
}

double ProviderBench::composite(const BenchResult& r) {
    // Kalite %60
    double s = r.quality * 0.60;
    // Gecikme %25: 1 sn = 60 puan, 10 sn = 0 (üstel azalma)
    const double sec = qMax(0.0, r.latencyMs / 1000.0);
    const double lat = 60.0 * std::exp(-sec / 3.0);
    s += lat * 0.25;
    // Maliyet %15: 0 $ = 60 puan, 0.05 $ = 0
    const double cost = 60.0 * std::exp(-r.usd / 0.02);
    s += cost * 0.15;
    return qBound(0.0, s, 100.0);
}

double ProviderBench::usdFor(const QString& providerId, const QString& model, int p, int e) {
    return ProviderPricing::estimateUsd(ProviderRegistry::byId(providerId), model, p, e);
}

BenchResult ProviderBench::score(const QString& providerId, const QString& model,
                                 const QList<QPair<QString, QString>>& answers,
                                 const QList<int>& latenciesMs) {
    BenchResult r;
    r.providerId = providerId;
    r.model = model;
    if (providerId.isEmpty() || answers.isEmpty()) {
        r.notes << "veri yok";
        return r;
    }
    double totalQ = 0.0;
    int n = 0;
    for (const auto& kv : answers) {
        const BenchTask t = taskById(kv.first);
        if (t.id.isEmpty()) continue;
        const double q = qualityOf(kv.first, kv.second);
        if (q < 40.0) r.notes << QString("%1: zayıf (%2 puan)").arg(t.title).arg(q, 0, 'f', 0);
        totalQ += q * (1.0 + 0.15 * (t.difficulty - 1)); // zor görevler daha ağır
        r.promptTokens += tokensEstimate(t.prompt);
        r.evalTokens += tokensEstimate(kv.second);
        ++n;
    }
    if (n == 0) {
        r.notes << "bilinen görev yok";
        return r;
    }
    r.quality = totalQ / double(n);
    // Gecikme: ortalaması (görev sayısı yoksa ilk değer)
    if (latenciesMs.isEmpty()) {
        r.notes << "gecikme ölçülmedi";
    } else {
        int sum = 0;
        for (int ms : latenciesMs) sum += ms;
        r.latencyMs = sum / latenciesMs.size();
    }
    r.usd = usdFor(providerId, model, r.promptTokens, r.evalTokens);
    if (ProviderPricing::isFreeTier(ProviderRegistry::byId(providerId), model))
        r.notes << "ücretsiz katman";
    else if (!ProviderPricing::priceFor(ProviderRegistry::byId(providerId), model).known)
        r.notes << "fiyat bilinmiyor (0 varsayıldı)";
    r.score = composite(r);
    r.ok = true;
    return r;
}

QString ProviderBench::verdict(const BenchResult& r) {
    if (!r.ok) return "sonuç yok";
    if (r.score >= 75.0) return "çok iyi";
    if (r.score >= 55.0) return "iyi";
    if (r.score >= 35.0) return "idare eder";
    return "zayıf";
}

QString ProviderBench::explain(const BenchResult& r) {
    if (!r.ok) return "sonuç yok";
    return QString("kalite %1 · gecikme %2 ms · ~$%3 → %4 puan (%5)")
        .arg(r.quality, 0, 'f', 0)
        .arg(r.latencyMs)
        .arg(r.usd, 0, 'f', 4)
        .arg(r.score, 0, 'f', 0)
        .arg(verdict(r));
}

QString ProviderBench::recommend(const QList<BenchResult>& results) {
    QList<BenchResult> valid;
    for (const BenchResult& r : results)
        if (r.ok) valid << r;
    if (valid.isEmpty()) return {};
    std::sort(valid.begin(), valid.end(), [](const BenchResult& a, const BenchResult& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.usd < b.usd;
    });
    const BenchResult& best = valid.first();
    // Ücretsiz olan en iyiye %8'in içindeyse onu öner (maliyet farkı puan farkını hak ediyor)
    for (const BenchResult& r : valid) {
        if (r.usd > 0.0) continue;
        if (r.score >= best.score * 0.92) {
            return QString("%1 (%2) — puan farkı ihmal edilebilir, maliyet sıfır")
                .arg(r.model.isEmpty() ? r.providerId
                                       : QString("%1 · %2").arg(r.providerId, r.model));
        }
        break;
    }
    return QString("%1 · %2").arg(best.providerId, best.model);
}
