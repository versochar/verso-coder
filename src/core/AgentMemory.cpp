#include "AgentMemory.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <algorithm>

AgentMemory::AgentMemory(const QString& filePath) : m_file(filePath) {
    if (!m_file.isEmpty()) load();
}

QStringList AgentMemory::tags() {
    return {"rule", "path", "command", "fact", "failure"};
}

QString AgentMemory::normalizeTag(const QString& tag) {
    const QString t = tag.trimmed().toLower();
    return tags().contains(t) ? t : "fact";
}

QStringList AgentMemory::tokens(const QString& text) {
    static const QStringList stopWords = {"bir", "bu", "ve", "ile", "için", "the", "and",
                                          "for", "that", "this", "ile", "de", "da"};
    QStringList out;
    const QString norm = QString(text).toLower();
    for (const QString& raw : norm.split(QRegularExpression("[^\\p{L}\\p{N}_]+"),
                                         Qt::SkipEmptyParts)) {
        if (raw.size() < 3) continue;
        if (stopWords.contains(raw)) continue;
        out << raw;
    }
    return out;
}

double AgentMemory::relevance(const QString& query, const MemoryNote& n) {
    const QStringList qt = tokens(query);
    if (qt.isEmpty()) return 0.0;
    const QStringList nt = tokens(n.text);
    if (nt.isEmpty()) return 0.0;
    int hit = 0;
    for (const QString& t : qt)
        for (const QString& u : nt)
            if (t == u) { ++hit; break; }
    if (hit == 0) return 0.0;
    const double cover = double(hit) / double(qt.size());          // sorgu ne kadar kapsandı
    const double density = double(hit) / double(qt.size() + nt.size());
    double score = cover * 0.7 + density * 0.3;
    // kural ve komut notları taranabilir bilgi olarak daha değerli
    if (n.tag == "rule") score *= 1.25;
    else if (n.tag == "command") score *= 1.15;
    else if (n.tag == "failure") score *= 0.85; // eski başarısızlıklar daha az ağırlık
    score *= qBound(0.25, double(qMax(1, n.weight)) / 3.0, 2.0);
    return score;
}

QList<MemoryNote> AgentMemory::recall(const QList<MemoryNote>& all, const QString& query, int topK,
                                      double minScore) {
    struct Scored { double s; MemoryNote n; };
    QList<Scored> hits;
    for (const MemoryNote& n : all) {
        const double s = relevance(query, n);
        if (s >= minScore) hits.append({s, n});
    }
    std::sort(hits.begin(), hits.end(), [](const Scored& a, const Scored& b) {
        if (a.s != b.s) return a.s > b.s;
        return a.n.id < b.n.id;
    });
    QList<MemoryNote> out;
    for (int i = 0; i < hits.size() && (topK <= 0 || out.size() < topK); ++i) out << hits[i].n;
    return out;
}

QString AgentMemory::formatRecall(const QList<MemoryNote>& hits, const QString& query, int maxChars) {
    if (hits.isEmpty()) return {};
    QString out = "\n[AJAN BELLEĞİ] (ilgili notlar)";
    if (!query.trimmed().isEmpty()) out += " — \"" + query.left(80) + "\"";
    out += ":\n";
    for (const MemoryNote& n : hits) {
        const QString line = QString("- [%1] %2").arg(n.tag, n.text.left(300));
        if (out.size() + line.size() > maxChars) break;
        out += line + "\n";
    }
    return out;
}

QString AgentMemory::add(const QString& text, const QString& tag, int weight) {
    const QString t = text.trimmed();
    if (t.isEmpty()) return {};
    const QString tg = normalizeTag(tag);
    for (MemoryNote& n : m_notes) {
        if (n.text.compare(t, Qt::CaseInsensitive) == 0) {
            ++n.weight;
            n.weight = qMin(n.weight, 20);
            save();
            return n.id; // tekilleştir: aynı notu tekrar ekleme
        }
    }
    MemoryNote n;
    n.id = QString("n%1").arg(QDateTime::currentMSecsSinceEpoch());
    n.text = t;
    n.tag = tg;
    n.weight = qBound(1, weight, 20);
    n.whenMs = QDateTime::currentMSecsSinceEpoch();
    m_notes << n;
    save();
    return n.id;
}

bool AgentMemory::remove(const QString& id) {
    for (int i = 0; i < m_notes.size(); ++i)
        if (m_notes[i].id == id) {
            m_notes.removeAt(i);
            save();
            return true;
        }
    return false;
}

void AgentMemory::clear() {
    m_notes.clear();
    save();
}

QStringList AgentMemory::texts() const {
    QStringList out;
    for (const MemoryNote& n : m_notes) out << QString("[%1] %2").arg(n.tag, n.text);
    return out;
}

bool AgentMemory::load() {
    m_notes.clear();
    if (m_file.isEmpty()) return false;
    QFile f(m_file);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    for (const QJsonValue& v : root.value("notes").toArray()) {
        const QJsonObject o = v.toObject();
        MemoryNote n;
        n.id = o.value("id").toString();
        n.text = o.value("text").toString();
        n.tag = normalizeTag(o.value("tag").toString());
        n.weight = qMax(1, o.value("weight").toInt(1));
        n.uses = o.value("uses").toInt(0);
        n.whenMs = qint64(o.value("when").toDouble());
        if (!n.text.isEmpty()) m_notes << n;
    }
    return true;
}

bool AgentMemory::save() const {
    if (m_file.isEmpty()) return false;
    QDir().mkpath(QFileInfo(m_file).absolutePath());
    QJsonArray arr;
    for (const MemoryNote& n : m_notes) {
        arr.append(QJsonObject{{"id", n.id},
                               {"text", n.text},
                               {"tag", n.tag},
                               {"weight", n.weight},
                               {"uses", n.uses},
                               {"when", double(n.whenMs)}});
    }
    QJsonObject root;
    root["notes"] = arr;
    QFile f(m_file);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}
