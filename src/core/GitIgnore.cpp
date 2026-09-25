#include "GitIgnore.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>

static QString globToRegex(const QString& glob) {
    QString out;
    for (int i = 0; i < glob.size();) {
        if (glob.mid(i, 3) == "**/") { out += "(.*/)?"; i += 3; }
        else if (glob.mid(i, 2) == "**") { out += ".*"; i += 2; }
        else if (glob[i] == '*') { out += "[^/]*"; ++i; }
        else if (glob[i] == '?') { out += "[^/]"; ++i; }
        else if (glob[i] == '{') {
            int j = glob.indexOf('}', i);
            if (j > 0) {
                out += "(" + glob.mid(i + 1, j - i - 1).split(',').join("|") + ")";
                i = j + 1;
            } else { out += "\\{"; ++i; }
        } else {
            static const QString meta = ".^$+()|[]\\";
            if (meta.contains(glob[i])) out += '\\';
            out += glob[i];
            ++i;
        }
    }
    return out;
}

QList<GitIgnore::Rule> GitIgnore::parseFile(const QString& filePath) {
    QList<Rule> out;
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return out;
    for (QString ln : QString::fromUtf8(f.readAll()).split('\n')) {
        if (ln.endsWith('\r')) ln.chop(1);
        QString t = ln.trimmed();
        if (t.isEmpty() || t.startsWith('#')) continue;
        if (t.startsWith("\\#") || t.startsWith("\\!")) t = t.mid(1);
        Rule r;
        if (t.startsWith('!')) { r.neg = true; t = t.mid(1); }
        if (t.endsWith('/')) { r.dirOnly = true; t.chop(1); }
        // Git kuralı: başta VEYA ortada '/' varsa köke çapa (leading dahil, silinmeden önce!)
        bool anchored = t.contains('/');
        if (t.startsWith('/')) { anchored = true; t = t.mid(1); }
        QString rx;
        rx = anchored ? "^" + globToRegex(t) + "(/.*)?$"
                      : "^(.*/)?" + globToRegex(t) + "(/.*)?$";
        r.rx = QRegularExpression(rx);
        if (r.rx.isValid()) out << r;
    }
    return out;
}

bool GitIgnore::matchRules(const QList<Rule>& rules, const QString& relPath, bool isDir) {
    bool ignored = false;
    for (const Rule& r : rules) {
        if (r.dirOnly && !isDir) {
            // "build/" kuralı yalnızca dizinleri hedefler: dosyayı yok say,
            // üst dizin üzerinden eşleştir (build/deep/x → üst zincir match).
            int k = relPath.lastIndexOf('/');
            if (k < 0) continue; // kök dosya, dizin değil
            if (r.rx.match(relPath.left(k)).hasMatch())
                ignored = !r.neg;
            continue;
        }
        if (r.rx.match(relPath).hasMatch())
            ignored = !r.neg;
    }
    return ignored;
}

void GitIgnore::load(const QString& root) {
    m_root = root;
    m_rules = parseFile(QDir(root).absoluteFilePath(".gitignore"));
}

bool GitIgnore::isIgnored(const QString& absPath, bool isDir) const {
    if (m_rules.isEmpty()) return false;
    QString rel = QDir(m_root).relativeFilePath(absPath);
    if (rel.startsWith("..")) return false;
    return matchRules(m_rules, rel, isDir);
}
