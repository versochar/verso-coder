#pragma once
#include <QJsonObject>
#include <QList>
#include <QString>

// Stage 13: imza yardımı — SignatureHelp parse.
struct SigParam {
    QString label;
    QString doc;
};
struct SigInfo {
    QString label;
    QString doc;
    QList<SigParam> params;
};
struct SignatureHelpData {
    QList<SigInfo> sigs;
    int activeSig = 0;
    int activeParam = 0;
    bool empty() const { return sigs.isEmpty(); }
};

class SignatureHelp {
public:
    static SignatureHelpData parse(const QJsonObject& res);
    // Gösterim metni: aktif parametreyi *...* ile işaretler
    static QString render(const SignatureHelpData& d);
};
