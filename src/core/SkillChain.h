#pragma once
#include "SkillRegistry.h"
#include <QList>
#include <QString>
#include <QStringList>

// Stage 34: beceri zinciri — hedefe uygun becerileri seçer, bağımlılık sırasına
// göre dizer ve ilerlemeyi izler. Saf mantık (ağ/UI yok), test edilebilir.
struct ChainStep {
    QString skill;
    QString status; // pending | active | done | skipped
    QString note;
};

class SkillChain {
public:
    explicit SkillChain(const QList<Skill>& skills = SkillRegistry::all());

    // Hedefe uygun becerileri seçip bağımlılık sırasıyla dizer (en fazla maxSteps).
    static QList<Skill> planFor(const QString& goal, const QList<Skill>& skills, int maxSteps = 6);
    // Topolojik sıralama: requires'ta geçen beceriler önce gelir. Döngüde kalanlar sona.
    static QList<Skill> orderByDependencies(const QList<Skill>& skills);
    // Hedef ile becerinin alakalılığı (0..1): ad/açıklama/etiket kesişimi.
    static double matchScore(const QString& goal, const Skill& s);

    // Zincir durumu
    int size() const { return m_steps.size(); }
    bool isEmpty() const { return m_steps.isEmpty(); }
    QStringList skillNames() const;
    QList<ChainStep> steps() const { return m_steps; }
    int doneCount() const;
    int currentIndex() const;          // -1 = bitti
    QString currentSkill() const;
    void startNext();                  // bir sonraki adımı aktif eder
    void completeCurrent(const QString& note = QString());
    void skipCurrent(const QString& note = QString());
    void reset();
    void clear() { m_steps.clear(); } // zinciri tamamen boşalt
    int percent() const;

    // Zinciri model istemine çevir (önceki adımlar tamamlandı bilgisiyle).
    QString toPrompt(const QString& goal) const;

    // Hedefe göre otomatik kur.
    static SkillChain forGoal(const QString& goal, const QList<Skill>& skills, int maxSteps = 6);

private:
    QList<ChainStep> m_steps;
};
