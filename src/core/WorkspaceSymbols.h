#pragma once
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

// Stage 24: çalışma alanı sembol dizini — regex tabanlı hızlı tarama
// (LSP yokken de çalışır). Saf mantık (test edilebilir).
struct WorkspaceSymbol {
    QString name;
    QString kind; // "func" | "class"
    QString file;
    int line = 1; // 1-based
};

class WorkspaceSymbols {
public:
    // Tek dosya metninden semboller (uzantıya göre desen)
    static QList<WorkspaceSymbol> scanFile(const QString& filePath, const QString& text);
    // Dizin tarama (saf, eşzamanlı): kök + dosya listesi → semboller
    static QList<WorkspaceSymbol> scanFiles(const QMap<QString, QString>& pathToText);
    // Aday eşleşme: desen alt-dize araması (küçük harf)
    static QList<WorkspaceSymbol> query(const QList<WorkspaceSymbol>& all,
                                        const QString& pattern, int max = 100);
    // Metinde ada yapılan gönderme sayısı (tanım satırı hariç, kelime sınırlı)
    static int refCount(const QString& text, const QString& name, int defLine);
};
