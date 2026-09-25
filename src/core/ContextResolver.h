#pragma once
#include <QString>
#include <QStringList>
#include <functional>

// Sohbet girdisindeki @-etiketlerini genişletir:
//   @dosya:src/main.cpp            → dosya içeriği
//   @dosya:src/main.cpp:10-25      → satır aralığı
//   @seçim                         → editördeki seçim
//   @sorunlar                      → Sorunlar paneli metni
//   @proje                         → proje ağacı listesi
//   @proje:kelime                  → proje araması (searcher verilirse)
class ContextResolver {
public:
    struct Resolved {
        QString text;        // prompt'a eklenecek bağlam
        QStringList files;   // kullanılan dosyalar
        QStringList warnings;
    };

    using Searcher = std::function<QString(const QString& query)>;

    static QStringList parseMentions(const QString& input);
    static bool hasMentions(const QString& input);

    Resolved resolve(const QString& input, const QString& root,
                     const QString& currentFile, const QString& selection,
                     const QString& problems, int maxChars = 12000) const;

    Searcher projectSearcher; // @proje:sorgu için opsiyonel

private:
    static QString readRange(const QString& abs, int from, int to, QStringList& files, int maxChars);
    static QString listTree(const QString& root, int maxEntries);
};
