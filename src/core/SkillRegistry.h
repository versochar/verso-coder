#pragma once
#include <QList>
#include <QString>
#include <QStringList>

// Stage 34: beceri (skill) tanımı. Beceri = çok adımlı, hazır ajan reçetesi.
struct Skill {
    QString name;
    QString description;
    QStringList steps;   // insan dili adımlar
    QStringList tools;   // gereken araçlar
    QStringList requires; // ön koşul becerileri (bağımlılık sırası)
    QStringList tags;
    bool builtin = false;
    bool isValid() const { return !name.trimmed().isEmpty(); }
};

// Dahili beceriler + kullanıcı becerileri (QSettings'de kalıcı).
class SkillRegistry {
public:
    static QList<Skill> builtin();
    static QList<Skill> custom();
    static QList<Skill> all();              // builtin + custom (özel isim çakışırsa özel kazanır)
    static Skill find(const QString& name);
    static bool exists(const QString& name);
    static QStringList names();
    static bool save(const Skill& s);       // yalnız custom
    static bool remove(const QString& name);
    static QString sanitize(const QString& name);
    // Beceri metnini model istemine çevir.
    static QString toPrompt(const Skill& s);
};
