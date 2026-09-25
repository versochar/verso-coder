#include "SnippetManager.h"
#include <QSettings>
#include <algorithm>

QString SnippetManager::langFor(const QString& suffix) {
    const QString s = suffix.toLower();
    if (s == "py") return "py";
    if (s == "c" || s == "cpp" || s == "h" || s == "hpp" || s == "cc") return "cpp";
    if (s == "js" || s == "ts") return "js";
    if (s == "sh") return "sh";
    return "*";
}

QList<SnippetDef> SnippetManager::builtin() {
    return {
        {"for", "for döngüsü", "for (int ${1:i} = 0; $1 < ${2:n}; ++$1) {\n\t$0\n}", "cpp"},
        {"if", "if bloğu", "if (${1:koşul}) {\n\t$0\n}", "cpp"},
        {"while", "while döngüsü", "while (${1:koşul}) {\n\t$0\n}", "cpp"},
        {"func", "fonksiyon", "${1:void} ${2:ad}(${3:param}) {\n\t$0\n}", "cpp"},
        {"class", "sınıf", "class ${1:Ad} {\npublic:\n\t$1();\n\t$0\n};", "cpp"},
        {"test", "test", "TEST(${1:Suite}, ${2:Ad}) {\n\t$0\n}", "cpp"},
        {"main", "main", "int main(int argc, char** argv) {\n\t$0\n\treturn 0;\n}", "cpp"},
        {"inc", "include", "#include <${1:bits/stdc++.h}>$0", "cpp"},
        {"for", "for döngüsü", "for ${1:i} in range(${2:n}):\n    $0", "py"},
        {"def", "fonksiyon", "def ${1:ad}(${2:param}):\n    $0", "py"},
        {"class", "sınıf", "class ${1:Ad}:\n    def __init__(self):\n        $0", "py"},
        {"ifmain", "main guard", "if __name__ == \"__main__\":\n    $0", "py"},
        {"try", "try/except", "try:\n    $1\nexcept ${2:Exception} as ${3:e}:\n    $0", "py"},
        {"log", "console.log", "console.log(${1:x});$0", "js"},
        {"fn", "fonksiyon", "function ${1:ad}(${2:param}) {\n\t$0\n}", "js"},
        {"shebang", "shebang", "#!/usr/bin/env ${1:bash}\n$0", "sh"},
    };
}

QList<SnippetDef> SnippetManager::custom() {
    QList<SnippetDef> out;
    QSettings q("Verso", "VersoCoder");
    q.beginGroup("snippets");
    for (const QString& lang : q.childGroups()) {
        q.beginGroup(lang);
        for (const QString& prefix : q.childKeys()) {
            const QStringList parts = q.value(prefix).toString().split("\x1f");
            if (parts.size() < 2) continue;
            out << SnippetDef{prefix, parts[0], parts.mid(1).join("\x1f"), lang};
        }
        q.endGroup();
    }
    return out;
}

QList<SnippetDef> SnippetManager::forLang(const QString& suffix) {
    const QString lang = langFor(suffix);
    QList<SnippetDef> out;
    for (const SnippetDef& s : builtin())
        if (s.lang == lang || s.lang == "*") out << s;
    for (const SnippetDef& s : custom())
        if (s.lang == lang || s.lang == "*") out << s;
    return out;
}

bool SnippetManager::saveCustom(const SnippetDef& s) {
    if (s.prefix.trimmed().isEmpty() || s.body.isEmpty()) return false;
    QSettings q("Verso", "VersoCoder");
    q.beginGroup("snippets");
    q.beginGroup(s.lang.isEmpty() ? "*" : s.lang);
    q.setValue(s.prefix.trimmed(),
               s.name.trimmed() + "\x1f" + s.body);
    return true;
}

bool SnippetManager::removeCustom(const QString& lang, const QString& prefix) {
    QSettings q("Verso", "VersoCoder");
    q.beginGroup("snippets");
    q.beginGroup(lang.isEmpty() ? "*" : lang);
    if (!q.contains(prefix)) return false;
    q.remove(prefix);
    return true;
}

QList<SnippetDef> SnippetManager::match(const QString& word, const QString& suffix) {
    if (word.trimmed().isEmpty()) return {};
    const QString w = word.toLower();
    QList<QPair<int, SnippetDef>> scored;
    for (const SnippetDef& s : forLang(suffix)) {
        const QString p = s.prefix.toLower();
        if (p.startsWith(w)) scored << qMakePair(0, s);
        else if (p.contains(w)) scored << qMakePair(1, s);
    }
    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    QList<SnippetDef> out;
    for (const auto& pr : scored) out << pr.second;
    return out;
}
