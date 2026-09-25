#pragma once
#include <QJsonObject>
#include <QList>
#include <QString>

// Stage 13: sembol ağacı — documentSymbol + workspaceSymbol ortak modeli.
struct SymbolNode {
    QString name;
    QString detail;
    int kind = 0; // 5 Class, 6 Method, 12 Function, 13 Variable...
    int line = 0; // 0-based
    int col = 0;
    int endLine = 0;
    QString container;
    QString path; // WorkspaceSymbol / SymbolInformation konumu (boşsa geçerli dosya)
    QList<SymbolNode> children;
};

class SymbolTree {
public:
    // DocumentSymbol yanıtı (dizi ya da {result:[...]})
    static QList<SymbolNode> parseDocument(const QJsonObject& res);
    static QList<SymbolNode> parseDocumentArray(const class QJsonArray& arr,
                                               const QString& container = QString());
    // WorkspaceSymbol yanıtı (SymbolInformation dizisi: location.range.start)
    static QList<SymbolNode> parseWorkspace(const QJsonObject& res);
    // Ağacı düz liste yap (outline + fuzzy arama için)
    static QList<SymbolNode> flatten(const QList<SymbolNode>& roots);
    static QString kindTitle(int kind);
    static QString kindIcon(int kind);
};
