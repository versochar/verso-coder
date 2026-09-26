#pragma once
#include <QString>

// Stage 24: kod metriği — saf sayaçlar (test edilebilir).
struct CodeMetrics {
    int lines = 0;
    int codeLines = 0; // boş + yorum dışı
    int functions = 0; // WorkspaceSymbols::scanFile sayımı
    int branches = 0;  // if/for/while/case/&&/||/?:
    static CodeMetrics analyze(const QString& filePath, const QString& text,
                               int functionCount);
    int complexity() const { return 1 + branches; }
};
