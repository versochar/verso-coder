#pragma once
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

// Stage 11: yapışkan kaydırma (sticky scroll) için girinti tabanlı kapsam zinciri.
// Dil bağımsız: daha az girintili üst satırlardan kapsam başlıklarını toplar.
struct ScopeFrame {
    int line0 = 0;      // 0-based başlık satırı
    QString text;       // kısaltılmış başlık metni
    int indent = 0;     // boşluk cinsinden girinti
};

class ScopeChain {
public:
    // line0 satırını kapsayan çerçeveler (dıştan içe sıralı)
    static QList<ScopeFrame> chain(const QStringList& lines, int line0, int tabWidth = 4);
    static QString label(const QList<ScopeFrame>& frames); // "A › b() › ..."
    static bool isHeaderLine(const QString& trimmed);      // { veya : ile biten / class/def
    static int indentOf(const QString& line, int tabWidth = 4);
};
