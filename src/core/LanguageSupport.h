#pragma once
#include <QString>
#include <QObject>

class LanguageSupport : public QObject {
    Q_OBJECT
public:
    explicit LanguageSupport(QObject* parent = nullptr);
    ~LanguageSupport() override = default;

    // Dosya uzantısına göre dil belirle
    QString detectLanguage(const QString& filePath) const;

    // Klasör içindeki dilleri tara
    QStringList scanDirectoryLanguages(const QString& dirPath) const;

    // compile_commands.json oluştur (basit sürüm)
    bool generateCompileCommands(const QString& projectDir, const QString& outputPath);

    // Mevcut proje dilini getir
    QString currentProjectLanguage() const;

signals:
    void languageDetected(const QString& language);
    void errorOccurred(const QString& message);

private:
    QString m_currentLanguage;
};
