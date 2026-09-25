#include "LanguageSupport.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QTextStream>

LanguageSupport::LanguageSupport(QObject* parent) : QObject(parent) {}

QString LanguageSupport::detectLanguage(const QString& filePath) const {
    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();
    
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
    
    // Dosya içeriğine bakarak dil belirle (basit örnek)
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
