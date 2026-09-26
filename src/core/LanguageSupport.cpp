#include "LanguageSupport.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QSettings>
#include <QTextStream>

LanguageSupport::LanguageSupport(QObject* parent) : QObject(parent) {}

QString LanguageSupport::detectLanguage(const QString& filePath) const {
    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();

    // Stage 28: önce yeni-dosya dil tablosu (tek kaynak)
    if (!ext.isEmpty()) {
        for (const LangDef& d : newFileLanguages())
            if (d.exts.contains(ext)) return d.id;
    }

    QMap<QString, QString> extensions = {
        {"c", "c"},
        {"h", "c"},
        {"cpp", "cpp"},
        {"hpp", "cpp"},
        {"cxx", "cpp"},
        {"hxx", "cpp"},
        {"cc", "cpp"},
        {"hh", "cpp"},
        {"py", "python"},
        {"pyw", "python"},
        {"js", "javascript"},
        {"jsx", "javascript"},
        {"ts", "typescript"},
        {"tsx", "typescript"},
        {"java", "java"},
        {"go", "go"},
        {"rs", "rust"},
        {"rb", "ruby"},
        {"php", "php"},
        {"swift", "swift"},
        {"m", "objectivec"},
        {"mm", "objectivec"}
    };
    
    auto it = extensions.find(ext);
    if (it != extensions.end()) {
        return *it;
    }

    // Stage 28: uzantısız/özel adlar + shebang
    const QString base = fi.fileName();
    if (base == "Makefile" || base == "makefile" || base == "GNUmakefile") return "make";
    if (base == "Dockerfile" || base.startsWith("Dockerfile.")) return "docker";
    if (base == "CMakeLists.txt") return "cmake";
    if (ext.isEmpty()) {
        QFile f(filePath);
        if (f.open(QIODevice::ReadOnly)) {
            const QString first = QString::fromUtf8(f.readLine()).trimmed();
            if (first.startsWith("#!")) {
                if (first.contains("python")) return "python";
                if (first.contains("bash") || first.contains("sh")) return "shell";
                if (first.contains("node")) return "javascript";
                if (first.contains("ruby")) return "ruby";
                if (first.contains("perl")) return "perl";
            }
        }
    }
    return "unknown";
}

QStringList LanguageSupport::scanDirectoryLanguages(const QString& dirPath) const {
    QStringList languages;
    QDir dir(dirPath);
    if (!dir.exists()) return languages;
    
    // Dosyaları tara
    const QStringList filters = {"*.c", "*.h", "*.cpp", "*.hpp", "*.py",
                                 "*.js", "*.ts", "*.java", "*.go", "*.rs"};

    const QStringList files =
        dir.entryList(filters, QDir::Files | QDir::NoSymLinks | QDir::Hidden);

    QMap<QString, int> langCounts;
    for (const QString& file : files) {
        const QString lang = detectLanguage(file);
        if (!lang.isEmpty() && lang != "unknown") langCounts[lang]++;
    }

    // En yaygın dili bul
    if (!langCounts.isEmpty()) {
        QString mostCommon;
        int maxCount = 0;
        for (auto it = langCounts.constBegin(); it != langCounts.constEnd(); ++it) {
            if (it.value() > maxCount) {
                maxCount = it.value();
                mostCommon = it.key();
            }
        }
        if (!mostCommon.isEmpty()) languages << mostCommon;
    }
    
    return languages;
}

bool LanguageSupport::generateCompileCommands(const QString& projectDir, const QString& outputPath) {
    // Basit bir compile_commands.json üretici
    // Gerçek implementasyon, compile_commands.json'yi CMake ile oluşturur
    
    QFileInfo fi(projectDir);
    if (!fi.isDir()) {
        emit errorOccurred("Geçersiz proje dizini");
        return false;
    }

    // CMakeLists.txt var mı kontrol et
    QFile cmake(projectDir + "/CMakeLists.txt");
    if (!cmake.exists()) {
        emit errorOccurred("Proje içinde CMakeLists.txt bulunamadı");
        return false;
    }
    
    // compile_commands.json generation is typically done by CMake.
    // Burada geçerli (boş) bir dizi yazılır; gerçek üretim CMake'e aittir.
    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit errorOccurred("compile_commands.json yazılamıyor");
        return false;
    }

    outFile.write(QJsonDocument(QJsonArray()).toJson(QJsonDocument::Indented));
    outFile.close();

    return true;
}

QString LanguageSupport::currentProjectLanguage() const {
    return m_currentLanguage;
}

// Stage 28: satır yorumu öneki (dosya yolu ya da dil id kabul eder)
QString LanguageSupport::commentPrefix(const QString& fileOrLang) {
    QString id = fileOrLang;
    if (id.contains('/') || id.contains('.')) {
        const QString det = LanguageSupport().detectLanguage(id);
        if (det != "unknown") id = det;
    }
    static const QMap<QString, QString> cm = {
        {"c", "//"}, {"cpp", "//"}, {"java", "//"}, {"javascript", "//"},
        {"typescript", "//"}, {"go", "//"}, {"rust", "//"}, {"php", "//"},
        {"swift", "//"}, {"kotlin", "//"}, {"csharp", "//"}, {"css", "//"},
        {"python", "#"}, {"shell", "#"}, {"ruby", "#"}, {"perl", "#"},
        {"sql", "--"}, {"html", "<!--"}, {"xml", "<!--"}, {"markdown", "<!--"},
        {"make", "#"}, {"docker", "#"}, {"cmake", "#"},
    };
    return cm.value(id);
}

// --- Stage 20: yeni dosya dilleri ---

QList<LanguageSupport::LangDef> LanguageSupport::newFileLanguages() {
    return {
        {"plain", "Düz metin", "Metin", {}, ""},
        {"c", "C", "C", {"c", "h"}, "#include <stdio.h>\n\nint main(void) {\n    return 0;\n}\n"},
        {"cpp", "C++", "C++", {"cpp", "hpp"}, "#include <iostream>\n\nint main() {\n    return 0;\n}\n"},
        {"python", "Python", "Python", {"py"}, "def main():\n    pass\n\n\nif __name__ == \"__main__\":\n    main()\n"},
        {"javascript", "JavaScript", "JS", {"js"}, "function main() {\n}\n\nmain();\n"},
        {"typescript", "TypeScript", "TS", {"ts"}, "function main(): void {\n}\n\nmain();\n"},
        {"java", "Java", "Java", {"java"}, "public class Main {\n    public static void main(String[] args) {\n    }\n}\n"},
        {"go", "Go", "Go", {"go"}, "package main\n\nfunc main() {\n}\n"},
        {"rust", "Rust", "Rust", {"rs"}, "fn main() {\n}\n"},
        {"ruby", "Ruby", "Ruby", {"rb"}, "def main\nend\n\nmain\n"},
        {"php", "PHP", "PHP", {"php"}, "<?php\n\nfunction main(): void {\n}\n\nmain();\n"},
        {"swift", "Swift", "Swift", {"swift"}, "func main() {\n}\n\nmain()\n"},
        {"kotlin", "Kotlin", "Kotlin", {"kt"}, "fun main() {\n}\n"},
        {"csharp", "C#", "C#", {"cs"}, "class Program {\n    static void Main() {\n    }\n}\n"},
        {"html", "HTML", "HTML", {"html"}, "<!DOCTYPE html>\n<html>\n<head><meta charset=\"utf-8\"></head>\n<body>\n</body>\n</html>\n"},
        {"css", "CSS", "CSS", {"css"}, "/* stil */\n"},
        {"json", "JSON", "JSON", {"json"}, "{}\n"},
        {"xml", "XML", "XML", {"xml"}, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<root>\n</root>\n"},
        {"markdown", "Markdown", "MD", {"md"}, "# Başlık\n"},
        {"shell", "Shell", "Shell", {"sh"}, "#!/usr/bin/env bash\nset -euo pipefail\n"},
        {"sql", "SQL", "SQL", {"sql"}, "SELECT 1;\n"},
    };
}

const LanguageSupport::LangDef* LanguageSupport::findLang(const QString& id) {
    static const QList<LangDef> all = newFileLanguages();
    for (const LangDef& d : all)
        if (d.id == id) return &d;
    return nullptr;
}

QString LanguageSupport::defaultExtension(const QString& id) {
    const LangDef* d = findLang(id);
    return (d && !d->exts.isEmpty()) ? d->exts.first() : QString();
}

QString LanguageSupport::skeleton(const QString& id) {
    const LangDef* d = findLang(id);
    return d ? d->skeleton : QString();
}

QString LanguageSupport::untitledTitle(int n, const QString& langId) {
    const QString ext = defaultExtension(langId);
    QString t = QString("Adsız-%1").arg(n);
    if (!ext.isEmpty()) t += " • " + ext;
    return t;
}

QStringList LanguageSupport::recentLangs() {
    return QSettings("Verso", "VersoCoder").value("newfile/recent").toStringList();
}

void LanguageSupport::pushRecentLang(const QString& id) {
    if (id.isEmpty()) return;
    QSettings q("Verso", "VersoCoder");
    QStringList r = q.value("newfile/recent").toStringList();
    r.removeAll(id);
    r.prepend(id);
    while (r.size() > 3) r.removeLast();
    q.setValue("newfile/recent", r);
}
