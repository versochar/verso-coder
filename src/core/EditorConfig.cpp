#include "EditorConfig.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

// glob -> regex: **/ -> (.*/)?   ** -> .*   * -> [^/]*   ? -> [^/]
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

bool EditorConfigParser::patternMatches(const QString& pattern, const QString& relPath) {
    QString pat = pattern.trimmed();
    if (pat.startsWith('/')) pat = pat.mid(1);
    bool hasSlash = pat.contains('/');
    QString target = hasSlash ? relPath : QFileInfo(relPath).fileName();
    // Baştaki **/ opsiyoneldir; desende yoksa dosya adına da bak
    QRegularExpression rx("^" + globToRegex(pat) + "$", QRegularExpression::CaseInsensitiveOption);
    if (rx.match(target).hasMatch()) return true;
    if (!hasSlash) {
        QRegularExpression rx2("^(.+/)?" + globToRegex(pat) + "$",
                               QRegularExpression::CaseInsensitiveOption);
        return rx2.match(relPath).hasMatch();
    }
    return false;
}

EditorConfig EditorConfigParser::forFile(const QString& filePath) {
    EditorConfig cfg;
    QFileInfo fi(filePath);
    QDir dir = fi.isDir() ? QDir(filePath) : fi.dir();
    // Kökten dosyaya doğru (en yakındaki .editorconfig en yüksek öncelik)
    QList<QPair<QString, QString>> chain; // (dir, relPath)
    QString absFile = fi.absoluteFilePath();
    QDir d = dir;
    while (true) {
        QString ec = d.absoluteFilePath(".editorconfig");
        if (QFile::exists(ec)) {
            QString rel = d.relativeFilePath(absFile);
            chain.prepend({ec, rel});
        }
        if (!d.cdUp()) break;
    }
    QMap<QString, QString> kv; // son yazan kazanır (öncelik sırasıyla uygula)
    for (const auto& [ecPath, rel] : chain) {
        QFile f(ecPath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        bool isRoot = false;
        QString section;
        bool sectionHit = false;
        for (QString ln : QString::fromUtf8(f.readAll()).split('\n')) {
            ln = ln.trimmed();
            if (ln.isEmpty() || ln.startsWith('#') || ln.startsWith(';')) continue;
            if (ln.startsWith('[') && ln.endsWith(']')) {
                section = ln.mid(1, ln.size() - 2).trimmed();
                sectionHit = patternMatches(section, rel);
                continue;
            }
            int eq = ln.indexOf('=');
            if (eq < 0) eq = ln.indexOf(':');
            if (eq < 0) continue;
            QString k = ln.left(eq).trimmed().toLower();
            QString v = ln.mid(eq + 1).trimmed().toLower();
            if (section.isEmpty()) {
                if (k == "root" && v == "true") isRoot = true;
                continue;
            }
            if (sectionHit) kv[k] = v;
        }
        if (isRoot) break; // daha üst dizinlere bakma
    }
    if (kv.isEmpty()) return cfg;
    cfg.found = true;
    QString style = kv.value("indent_style", "");
    if (style == "tab") cfg.useSpaces = false;
    else if (style == "space") cfg.useSpaces = true;
    bool ok = false;
    int size = kv.value("indent_size", "").toInt(&ok);
    if (ok && size > 0) cfg.indentSize = qBound(1, size, 16);
    else {
        int tw = kv.value("tab_width", "").toInt(&ok);
        if (ok && tw > 0) cfg.indentSize = qBound(1, tw, 16);
    }
    QString eol = kv.value("end_of_line", "");
    if (eol == "crlf" || eol == "cr") cfg.endOfLine = "crlf";
    cfg.trimTrailing = (kv.value("trim_trailing_whitespace", "") == "true");
    cfg.insertFinalNewline = (kv.value("insert_final_newline", "") == "true");
    return cfg;
}
