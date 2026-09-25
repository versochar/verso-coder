#pragma once
#include <QPair>
#include <QString>

// Stage 11: akıllı seçim genişletme (saf mantık).
// kelime → satır içeriği → tam satır → saran parantez bloğu → belge.
class SelectionGrow {
public:
    static QPair<int, int> grow(const QString& text, int start, int end);
    static bool isWordChar(QChar ch);
};

inline bool SelectionGrow::isWordChar(QChar ch) {
    return ch.isLetterOrNumber() || ch == '_';
}
