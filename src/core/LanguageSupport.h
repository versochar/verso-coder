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

    // --- Stage 20: yeni dosya dilleri ---
    struct LangDef {
        QString id;          // "python" | "plain"…
        QString title;       // "Python"
        QString chip;        // durum çubuğu kısa adı
        QStringList exts;    // {"py"} (plain → boş)
        QString skeleton;    // yeni dosyaya konulacak iskelet ("" = yok)
        QString comment;     // Stage 28: satır yorumu öneki ("//" | "#" | "" = yok)
    };
    // Dosya/uzantı/dil-id → yorum öneki
    static QString commentPrefix(const QString& fileOrLang);
    // Düz metin dahil 21 kayıt (sıralı, başlıklar Türkçe değil evrensel)
    static QList<LangDef> newFileLanguages();
    static const LangDef* findLang(const QString& id);
    static QString defaultExtension(const QString& id); // "py" | ""
    static QString skeleton(const QString& id);
    // "Adsız-3 • py" ya da uzantısızda "Adsız-3"
    static QString untitledTitle(int n, const QString& langId);
    // Son kullanılan diller (en çok 3, QSettings kalıcı)
    static QStringList recentLangs();
    static void pushRecentLang(const QString& id);

signals:
    void languageDetected(const QString& language);
    void errorOccurred(const QString& message);

private:
    QString m_currentLanguage;
};
