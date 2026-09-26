#include "SkillRegistry.h"
#include <QSettings>

static const char* kGroup = "agentSkills";

QList<Skill> SkillRegistry::builtin() {
    auto mk = [](const QString& name, const QString& desc, const QStringList& steps,
                 const QStringList& tools, const QStringList& req, const QStringList& tags) {
        Skill s;
        s.name = name;
        s.description = desc;
        s.steps = steps;
        s.tools = tools;
        s.requires = req;
        s.tags = tags;
        s.builtin = true;
        return s;
    };
    return {
        mk("incele", "Kodu ve proje yapısını okuyup özetle",
           {"Proje dizinini listele", "İlgili dosyaları oku", "Bulguları raporla"},
           {"list_dir", "read_file", "search"}, {}, {"analiz", "okuma", "başlangıç"}),
        mk("hata-ayikla", "Bir hatayı bulup düzelt",
           {"Hata mesajını ve dosyayı oku", "Kök nedeni belirle", "Düzeltmeyi yaz",
            "Testi çalıştır"},
           {"get_problems", "read_file", "search", "write_file", "run_tests"},
           {"incele"}, {"hata", "düzeltme", "test"}),
        mk("test-yaz", "Birim testleri ekle",
           {"Kaynak dosyayı oku", "Test çerçevesini incele", "Test dosyasını yaz",
            "Testleri çalıştır"},
           {"read_file", "search", "write_file", "run_tests"},
           {"incele"}, {"test", "yazma"}),
        mk("refactor", "Kodu okunurluğunu artıracak şekilde yeniden düzenle",
           {"Hedef dosyaları oku", "Refactor sınırlarını belirle", "Küçük adımlarla yaz",
            "Derleme/test ile doğrula"},
           {"read_file", "search", "write_file", "run_command"},
           {"incele"}, {"refactor", "temizlik"}),
        mk("dokumante-et", "Dosyaları dokümante et",
           {"Genel yapıyı çıkar", "Bölüm başlıklarını yaz", "Karmaşık yerleri açıkla"},
           {"list_dir", "read_file", "write_file"}, {"incele"}, {"doküman"}),
        mk("commit-hazirla", "Değişiklikleri commit'e hazırla",
           {"git durumunu incele", "diff özetini çıkar", "Commit mesajı öner"},
           {"git_status", "git_diff", "read_file"}, {"incele"}, {"git", "commit"}),
        mk("gözden-gecir", "Değişiklikleri eleştirel gözden geçir (okuma + rapor)",
           {"Değişen dosyaları listele", "Diff'leri oku", "Riskleri ve eksikleri yaz"},
           {"git_status", "git_diff", "read_file", "health_scan"}, {},
           {"denetim", "rapor", "otonom"}),
        mk("saglik-raporu", "Proje sağlık durumunu ölç ve raporla",
           {"Sağlık taraması çalıştır", "Sorunları sırala", "Öneriler üret"},
           {"health_scan", "read_file"}, {}, {"denetim", "rapor", "otonom"}),
    };
}

QString SkillRegistry::sanitize(const QString& name) {
    QString out;
    for (const QChar& c : name.trimmed().toLower())
        if (c.isLetterOrNumber()) out += c;
        else if (c == ' ' || c == '-' || c == '_') out += '-';
    while (out.contains("--")) out.replace("--", "-");
    return out;
}

QList<Skill> SkillRegistry::custom() {
    QList<Skill> out;
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    for (const QString& k : q.childKeys()) {
        Skill s;
        s.name = k;
        s.builtin = false;
        // Alanlar '|' ile ayrılmış: desc|step,step,step|tool,tool|req,req|tag,tag
        const QStringList parts = q.value(k).toString().split('|');
        s.description = parts.value(0);
        auto splitList = [](const QString& s) {
            return s.isEmpty() ? QStringList() : s.split(',', Qt::SkipEmptyParts);
        };
        s.steps = splitList(parts.value(1));
        s.tools = splitList(parts.value(2));
        s.requires = splitList(parts.value(3));
        s.tags = splitList(parts.value(4));
        if (s.isValid()) out << s;
    }
    return out;
}

QList<Skill> SkillRegistry::all() {
    QList<Skill> out = builtin();
    for (const Skill& s : custom()) {
        bool replaced = false;
        for (Skill& b : out)
            if (b.name == s.name) { b = s; replaced = true; break; }
        if (!replaced) out << s;
    }
    return out;
}

Skill SkillRegistry::find(const QString& name) {
    const QString n = name.trimmed().toLower();
    for (const Skill& s : all())
        if (s.name == n) return s;
    return {};
}

bool SkillRegistry::exists(const QString& name) { return find(name).isValid(); }

QStringList SkillRegistry::names() {
    QStringList out;
    for (const Skill& s : all()) out << s.name;
    return out;
}

bool SkillRegistry::save(const Skill& s) {
    const QString n = sanitize(s.name);
    if (n.isEmpty() || s.description.trimmed().isEmpty()) return false;
    Skill copy = s;
    copy.name = n;
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    q.setValue(n, QStringList{copy.description, copy.steps.join(","), copy.tools.join(","),
                              copy.requires.join(","), copy.tags.join(",")}
                          .join('|'));
    return true;
}

bool SkillRegistry::remove(const QString& name) {
    const QString n = sanitize(name);
    if (n.isEmpty()) return false;
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    if (!q.contains(n)) return false;
    q.remove(n);
    return true;
}

QString SkillRegistry::toPrompt(const Skill& s) {
    if (!s.isValid()) return {};
    QString out = QString("\n[BECERİ: %1] %2\n").arg(s.name, s.description);
    if (!s.steps.isEmpty()) {
        out += "Adımlar:\n";
        for (int i = 0; i < s.steps.size(); ++i)
            out += QString("  %1. %2\n").arg(i + 1).arg(s.steps[i]);
    }
    if (!s.tools.isEmpty()) out += "Araçlar: " + s.tools.join(", ") + "\n";
    out += "Her adımda önce gözlem yap, sonra bir sonraki adıma geç.\n";
    return out;
}
