#include "SkillChain.h"
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>

SkillChain::SkillChain(const QList<Skill>& skills) {
    for (const Skill& s : skills) {
        if (!s.isValid()) continue;
        m_steps.append({s.name, "pending", QString()});
    }
}

double SkillChain::matchScore(const QString& goal, const Skill& s) {
    if (goal.trimmed().isEmpty() || !s.isValid()) return 0.0;
    const QString g = goal.toLower();
    double score = 0.0;
    // ad tam eşleşmesi
    if (g.contains(s.name.toLower()) || s.name.toLower().contains(g))
        score += 1.0;
    // etiket
    for (const QString& t : s.tags)
        if (!t.isEmpty() && g.contains(t.toLower())) score += 0.6;
    // açıklama kelime kesişimi
    const QStringList gw = g.split(QRegularExpression("[^\\p{L}\\p{N}]+"), Qt::SkipEmptyParts);
    const QString sw = QString(s.description).toLower() + " " + s.steps.join(" ").toLower();
    int hit = 0;
    for (const QString& w : gw) {
        if (w.size() < 3) continue;
        if (sw.contains(w)) ++hit;
    }
    if (!gw.isEmpty()) score += double(hit) / double(gw.size());
    return score;
}

QList<Skill> SkillChain::orderByDependencies(const QList<Skill>& skills) {
    QHash<QString, Skill> byName;
    for (const Skill& s : skills)
        if (s.isValid()) byName.insert(s.name, s);

    QList<Skill> out;
    QSet<QString> done;
    QList<Skill> remaining = skills;
    // Kahn benzeri: bağımlılığı çözülmeyenleri atla, döngü kalanları sona ekle
    bool progress = true;
    while (!remaining.isEmpty() && progress) {
        progress = false;
        for (int i = 0; i < remaining.size();) {
            const Skill& s = remaining[i];
            bool ready = true;
            for (const QString& r : s.requires) {
                if (byName.contains(r) && !done.contains(r)) { ready = false; break; }
            }
            if (ready) {
                out << s;
                done.insert(s.name);
                remaining.removeAt(i);
                progress = true;
            } else {
                ++i;
            }
        }
    }
    out << remaining; // döngü/çözülemeyenler sona
    return out;
}

QList<Skill> SkillChain::planFor(const QString& goal, const QList<Skill>& skills, int maxSteps) {
    struct Scored { double s; Skill skill; };
    QList<Scored> scored;
    for (const Skill& s : skills) {
        if (!s.isValid()) continue;
        const double sc = matchScore(goal, s);
        if (sc > 0) scored.append({sc, s});
    }
    std::sort(scored.begin(), scored.end(), [](const Scored& a, const Scored& b) {
        if (a.s != b.s) return a.s > b.s;
        return a.skill.name < b.skill.name;
    });
    QList<Skill> picked;
    QSet<QString> have;
    // Ön koşulları da dahil et (beceri bütünlüğü için)
    auto push = [&picked, &have, &skills](const QString& name) {
        if (have.contains(name)) return;
        for (const Skill& s : skills)
            if (s.name == name) { picked << s; have.insert(name); return; }
    };
    const int limit = maxSteps > 0 ? maxSteps : 6;
    for (const Scored& sc : scored) {
        if (picked.size() >= limit) break;
        for (const QString& r : sc.skill.requires) push(r);
        push(sc.skill.name);
    }
    return orderByDependencies(picked);
}

QStringList SkillChain::skillNames() const {
    QStringList out;
    for (const ChainStep& s : m_steps) out << s.skill;
    return out;
}

int SkillChain::doneCount() const {
    int n = 0;
    for (const ChainStep& s : m_steps)
        if (s.status == "done" || s.status == "skipped") ++n;
    return n;
}

int SkillChain::currentIndex() const {
    for (int i = 0; i < m_steps.size(); ++i)
        if (m_steps[i].status == "active" || m_steps[i].status == "pending") return i;
    return -1;
}

QString SkillChain::currentSkill() const {
    const int i = currentIndex();
    return i < 0 ? QString() : m_steps[i].skill;
}

void SkillChain::startNext() {
    const int i = currentIndex();
    if (i < 0) return;
    m_steps[i].status = "active";
}

void SkillChain::completeCurrent(const QString& note) {
    const int i = currentIndex();
    if (i < 0) return;
    m_steps[i].status = "done";
    m_steps[i].note = note;
    startNext();
}

void SkillChain::skipCurrent(const QString& note) {
    const int i = currentIndex();
    if (i < 0) return;
    m_steps[i].status = "skipped";
    m_steps[i].note = note;
    startNext();
}

void SkillChain::reset() {
    for (ChainStep& s : m_steps) {
        s.status = "pending";
        s.note.clear();
    }
}

int SkillChain::percent() const {
    if (m_steps.isEmpty()) return 0;
    return int((doneCount() * 100) / m_steps.size());
}

QString SkillChain::toPrompt(const QString& goal) const {
    if (m_steps.isEmpty()) return {};
    QString out = goal.trimmed().isEmpty() ? QString() : QString("[HEDEF] ") + goal + "\n";
    out += "\n[BECERİ ZİNCİRİ]\n";
    for (int i = 0; i < m_steps.size(); ++i) {
        const ChainStep& s = m_steps[i];
        const QString mark = s.status == "done" ? "[✓]" : (s.status == "skipped" ? "[-]"
                                                                                 : "[ ]");
        out += QString("%1 %2. %3").arg(mark).arg(i + 1).arg(s.skill);
        if (!s.note.isEmpty()) out += " — " + s.note.left(120);
        out += "\n";
    }
    const QString cur = currentSkill();
    if (!cur.isEmpty()) out += QString("Şu an %1 becerisindesin; tamamlananları tekrarlama.\n").arg(cur);
    return out;
}

SkillChain SkillChain::forGoal(const QString& goal, const QList<Skill>& skills, int maxSteps) {
    return SkillChain(planFor(goal, skills, maxSteps));
}
