#pragma once
#include <QJsonObject>
#include <QList>
#include <QString>

// Stage 13: çağrı hiyerarşisi — prepare + incoming/outgoing → ağaç.
struct CallNode {
    QString name;
    QString detail;
    QString uri;
    int line = 0;
    int col = 0;
    QList<CallNode> children;
    // fromRanges (incoming) veya toRanges (outgoing) özeti
    QString rangeSummary;
};

class CallHierarchyTree {
public:
    static QList<CallNode> parsePrepare(const QJsonObject& res);
    // incoming/outgoing yanıtı: [{from:{...}, fromRanges:[...]}] veya [{to...}]
    static QList<CallNode> parseCalls(const QJsonObject& res, bool incoming);
    static QJsonObject itemParams(const CallNode& n); // sonraki seviye isteği için
};
