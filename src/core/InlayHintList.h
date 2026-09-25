#pragma once
#include <QJsonObject>
#include <QList>
#include <QString>

// Stage 13: satır içi ipuçları — inlayHint parse.
struct InlayHint {
    int line = 0; // 0-based
    int col = 0;
    QString label;
    int kind = 0; // 1 Type, 2 Parameter
};

class InlayHintList {
public:
    static QList<InlayHint> parse(const QJsonObject& res);
    static QList<InlayHint> parseArray(const class QJsonArray& arr);
    static QList<InlayHint> forLine(const QList<InlayHint>& hints, int line);
};
