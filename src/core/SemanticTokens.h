#pragma once
#include <QJsonObject>
#include <QList>
#include <QStringList>

// Stage 13: semantik token — LSP delta kodlamasını çöz.
struct SemanticToken {
    int line = 0; // mutlak 0-based
    int col = 0;
    int len = 0;
    int type = 0; // legend index
    int mods = 0;
};

class SemanticTokens {
public:
    static QList<SemanticToken> decode(const QJsonObject& res);
    static QList<SemanticToken> decodeData(const QList<int>& data, int& lineBase);
    static QStringList legend(const QJsonObject& res); // resultLegend üstünden de okunabilir
    // Tip adından renk rolü: keyword/string/comment/number/func/type/diğer
    static QString roleFor(const QString& tokenType);
};
