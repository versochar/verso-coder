#include "GridCheck.h"
#include <QFontDatabase>
#include <QFontMetrics>
#include <QStringList>

bool GridCheck::isMonospace(const QFont& f) {
    return cellWidth(f) > 0;
}

int GridCheck::cellWidth(const QFont& f) {
    QFontMetrics fm(f);
    int w = -1;
    for (int c = 32; c <= 126; ++c) {
        const int a = fm.horizontalAdvance(QChar(c));
        if (a <= 0) return -1;
        if (w < 0) w = a;
        else if (a != w) return -1;
    }
    return w;
}

int GridCheck::columnX(const QFont& f, int col) {
    const int w = cellWidth(f);
    return w > 0 ? col * w : -1;
}

QString GridCheck::systemMonospace() {
    const QStringList cands = {"JetBrains Mono", "Fira Code", "Consolas",
                               "DejaVu Sans Mono", "Liberation Mono", "monospace"};
    const QStringList fams = QFontDatabase::families();
    for (const QString& c : cands)
        if (fams.contains(c, Qt::CaseInsensitive)) return c;
    return "monospace";
}
