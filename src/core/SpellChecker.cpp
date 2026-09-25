#include "SpellChecker.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <hunspell/hunspell.hxx>

SpellChecker::SpellChecker() = default;

SpellChecker::~SpellChecker() {
    delete static_cast<Hunspell*>(m_h);
}

QStringList SpellChecker::dictionaryDirs() {
    QStringList dirs = {
        "/usr/share/hunspell",
        "/usr/share/myspell",
        "/usr/share/hunspell-tr",
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/dicts",
    };
    if (const char* e = getenv("HUNSPELL_DICT_DIR")) dirs.prepend(QString::fromUtf8(e));
    return dirs;
}

QString SpellChecker::findDict(const QString& lang) {
    for (const QString& d : dictionaryDirs()) {
        QString p = d + "/" + lang + ".dic";
        QString a = d + "/" + lang + ".aff";
        if (QFileInfo::exists(p) && QFileInfo::exists(a)) return p;
    }
    return {};
}

bool SpellChecker::load(const QString& lang) {
    delete static_cast<Hunspell*>(m_h);
    m_h = nullptr;
    m_ok = false;
    QString dic = findDict(lang);
    if (dic.isEmpty()) return false;
    QString aff = dic;
    aff.replace(".dic", ".aff");
    try {
        m_h = new Hunspell(aff.toUtf8().constData(), dic.toUtf8().constData());
    } catch (...) {
        m_h = nullptr;
        return false;
    }
    m_lang = lang;
    m_ok = true;
    return true;
}

bool SpellChecker::check(const QString& word) {
    if (!m_ok || word.size() < 3) return true;
    return static_cast<Hunspell*>(m_h)->spell(word.toUtf8().constData()) != 0;
}

QStringList SpellChecker::suggest(const QString& word, int max) {
    QStringList out;
    if (!m_ok) return out;
    std::vector<std::string> w = static_cast<Hunspell*>(m_h)->suggest(word.toUtf8().constData());
    for (size_t i = 0; i < w.size() && (int)out.size() < max; ++i)
        out << QString::fromUtf8(w[i].c_str());
    return out;
}
