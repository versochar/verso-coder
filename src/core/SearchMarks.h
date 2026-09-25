#pragma once
#include <QList>
#include <QMap>
#include <QPair>
#include <QString>

// Stage 11: editör-içi arama isabetleri (saf mantık).
// EditorFindBar + overview ruler + minimap işaretleri bu listeyi paylaşır.
struct FindHit {
    int start = 0;
    int end = 0;
    int line0 = 0; // 0-based satır
};

class SearchMarks {
public:
    static QList<FindHit> findAll(const QString& text, const QString& needle,
                                    bool caseSensitive = false, bool wholeWord = false,
                                    int maxHits = 2000);
    static bool isWordChar(QChar ch);
};

inline bool SearchMarks::isWordChar(QChar ch) {
    return ch.isLetterOrNumber() || ch == '_';
}
