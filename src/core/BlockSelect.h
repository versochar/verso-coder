#pragma once
#include <QList>
#include <QString>
#include <QStringList>

// Stage 17: dikdörtgen (sütun) blok seçimi — saf aralık hesabı.
// CodeEditor fare/klavye olayları bu aralıkları çoklu imlece çevirir.
struct BlockSpan {
    int line0 = 0; // 0-based satır
    int colA = 0;  // görsel sütun (küçük)
    int colB = 0;  // görsel sütun (büyük, hariç)
};

class BlockSelect {
public:
    // Sekme genişletmeli görsel sütun hesabı
    static int visualCol(const QString& line, int charCol, int tabWidth = 4);
    static int charCol(const QString& line, int visualCol, int tabWidth = 4);
    // İki köşe (satır+sütun) → satır başına aralık listesi
    static QList<BlockSpan> spans(int lineA, int visA, int lineB, int visB);
    // Blok metni çıkar (kısa satırlar boşlukla doldurulur)
    static QStringList extract(const QStringList& lines, const QList<BlockSpan>& spans,
                               int tabWidth = 4);
    // Sekme içeren satırda görsel sütundan gerçek metin dilimi
    static QString sliceVisual(const QString& line, int visA, int visB, int tabWidth = 4);
};
