#pragma once
#include <QList>
#include <QString>

// Stage 17: snippet gövde motoru (saf) — VS Code tarzı sözdizimi alt kümesi:
//   $1 $2 ... $0 (durak), ${1:varsayılan} (yer tutuculu), $1 tekrarı (ayna).
struct SnippetSegment {
    enum class Type { Text, Tabstop, Placeholder, Mirror } type = Type::Text;
    QString text; // Text: düz metin | Placeholder: varsayılan | diğer: ""
    int index = 0; // durak numarası ($0 = son durak)
};

struct SnippetExpand {
    QString text;              // yerleştirilecek düz metin
    QList<int> stopOffsets;    // belge içi durak konumları (belge başından, $0 dahil en sonda)
    QList<QString> stopNames;  // yer tutucu varsayılanları (durakla aynı sırada)
};

class SnippetEngine {
public:
    static QList<SnippetSegment> parse(const QString& body);
    static SnippetExpand expand(const QString& body, int baseOffset = 0,
                                const QString& clipboard = QString());
    // $TM_FILENAME / $TM_SELECTED_TEXT gibi basit değişkenler
    static QString expandVars(QString body, const QString& fileName,
                              const QString& selected);
    static bool hasStops(const QString& body);
    static int stopCount(const QString& body);
};
