#include "GitBlame.h"
#include <QRegularExpression>

QList<BlameLine> GitBlame::parse(const QString& porcelain) {
    QList<BlameLine> out;
    static QRegularExpression headRe(R"(^([0-9a-f]{7,40})\s+(\d+)\s+(\d+))");
    BlameLine cur;
    bool inBlock = false;
    for (const QString& raw : porcelain.split('\n')) {
        if (raw.startsWith('\t')) {
            if (inBlock && cur.line > 0) out << cur;
            cur = BlameLine();
            inBlock = false;
            continue;
        }
        auto hm = headRe.match(raw);
        if (hm.hasMatch()) {
            if (inBlock && cur.line > 0) out << cur;
            cur = BlameLine();
            cur.line = hm.captured(3).toInt();
            inBlock = true;
            continue;
        }
        if (!inBlock) continue;
        if (raw.startsWith("author ")) cur.author = raw.mid(7).trimmed();
        else if (raw.startsWith("author-time "))
            cur.authorTime = raw.mid(12).trimmed().toLongLong();
    }
    if (inBlock && cur.line > 0) out << cur;
    return out;
}

int GitBlame::ageDays(const BlameLine& b, qint64 nowSecs) {
    if (b.authorTime <= 0 || nowSecs <= b.authorTime) return b.authorTime > 0 ? 0 : -1;
    return int((nowSecs - b.authorTime) / 86400);
}

double GitBlame::heat(const BlameLine& b, qint64 nowSecs, int maxAgeDays) {
    const int d = ageDays(b, nowSecs);
    if (d < 0) return 0.0;
    return qBound(0.0, double(d) / qMax(1, maxAgeDays), 1.0);
}
