#include "BlockSelect.h"
#include <algorithm>

int BlockSelect::visualCol(const QString& line, int charCol, int tabWidth) {
    int v = 0;
    for (int i = 0; i < qMin(charCol, line.size()); ++i) {
        if (line[i] == '\t') v += tabWidth - (v % tabWidth);
        else ++v;
    }
    return v;
}

int BlockSelect::charCol(const QString& line, int visualCol, int tabWidth) {
    int v = 0;
    int i = 0;
    while (i < line.size() && v < visualCol) {
        if (line[i] == '\t') v += tabWidth - (v % tabWidth);
        else ++v;
        ++i;
    }
    return i;
}

QList<BlockSpan> BlockSelect::spans(int lineA, int visA, int lineB, int visB) {
    QList<BlockSpan> out;
    const int top = qMin(lineA, lineB), bot = qMax(lineA, lineB);
    const int left = qMin(visA, visB), right = qMax(visA, visB);
    if (left == right) return out;
    for (int l = top; l <= bot; ++l) out << BlockSpan{l, left, right};
    return out;
}

QString BlockSelect::sliceVisual(const QString& line, int visA, int visB, int tabWidth) {
    if (visB <= visA) return QString();
    const int a = charCol(line, visA, tabWidth);
    const int b = charCol(line, visB, tabWidth);
    QString mid = line.mid(a, qMax(0, b - a));
    // Hedef görsel genişlikten kısaysa boşlukla doldur (dikdörtgen bütünlüğü)
    const int want = visB - visA;
    const int got = visualCol(line, b, tabWidth) - visualCol(line, a, tabWidth);
    if (got < want) mid += QString(want - got, ' ');
    return mid;
}

QStringList BlockSelect::extract(const QStringList& lines, const QList<BlockSpan>& spans,
                                 int tabWidth) {
    QStringList out;
    for (const BlockSpan& s : spans) {
        if (s.line0 < 0 || s.line0 >= lines.size()) { out << QString(); continue; }
        out << sliceVisual(lines[s.line0], s.colA, s.colB, tabWidth);
    }
    return out;
}
