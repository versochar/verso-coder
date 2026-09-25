#pragma once
#include <QJsonObject>
#include <QList>
#include <QString>

// Stage 18: işbirliği mesaj modeli + hafif metin birleştirme (saf, soketsiz).
// Protokol (JSON, satır başına bir ileti, WebSocket metin çerçevesi):
//   {t:"hello",name,version} {t:"cursor",user,line,col} {t:"edit",user,base,ops:[...]}
//   {t:"sync",text,rev} {t:"bye",user} {t:"term",user,chunk}
struct CollabOp {
    QString kind; // "ins" | "del"
    int pos = 0;
    QString text; // ins: eklenen | del: silinen uzunluğu kadar yer tutucu (uzunluk için)
    int len = 0;  // del uzunluğu
};

class CollabMerge {
public:
    // Uzak işlemi yerel metne uygula (konumları rev farkına göre kaydır — basit OT).
    // localOps: baz rev'den sonra yerelde yapılan işlemler.
    static QString apply(const QString& local, const QList<CollabOp>& remoteOps,
                         const QList<CollabOp>& localOps, int& newRev);
    // Tek işlemi işlem listesine göre dönüştür (konum kaydırma)
    static CollabOp transform(CollabOp op, const QList<CollabOp>& against);
    static QList<CollabOp> parseOps(const QJsonObject& msg);
    static QJsonObject makeEdit(const QString& user, int base,
                                const QList<CollabOp>& ops);
    static QJsonObject makeCursor(const QString& user, int line, int col);
    static QJsonObject makeHello(const QString& user, int version);
    // İleti türü geçerli mi? (bilinmeyen türler yok sayılır)
    static bool validType(const QString& t);
    // İki rev arası farktan işlem üret (basit önek/sonek kırpma)
    static QList<CollabOp> diff(const QString& before, const QString& after);
};
