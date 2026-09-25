#include "CollabMerge.h"
#include <QJsonArray>
#include <QJsonValue>
#include <algorithm>

bool CollabMerge::validType(const QString& t) {
    return t == "hello" || t == "cursor" || t == "edit" || t == "sync"
        || t == "bye" || t == "term";
}

QJsonObject CollabMerge::makeHello(const QString& user, int version) {
    return {{"t", "hello"}, {"user", user}, {"version", version}};
}

QJsonObject CollabMerge::makeCursor(const QString& user, int line, int col) {
    return {{"t", "cursor"}, {"user", user}, {"line", line}, {"col", col}};
}

QJsonObject CollabMerge::makeEdit(const QString& user, int base,
                                  const QList<CollabOp>& ops) {
    QJsonArray a;
    for (const CollabOp& o : ops)
        a.append(QJsonObject{{"k", o.kind},
                             {"p", o.pos},
                             {"t", o.text},
                             {"l", o.len}});
    return {{"t", "edit"}, {"user", user}, {"base", base}, {"ops", a}};
}

QList<CollabOp> CollabMerge::parseOps(const QJsonObject& msg) {
    QList<CollabOp> out;
    for (const QJsonValue& v : msg.value("ops").toArray()) {
        const QJsonObject o = v.toObject();
        CollabOp op;
        op.kind = o.value("k").toString();
        if (op.kind != "ins" && op.kind != "del") continue;
        op.pos = qMax(0, o.value("p").toInt());
        op.text = o.value("t").toString();
        op.len = qMax(0, o.value("l").toInt());
        if (op.kind == "ins" && op.text.isEmpty()) continue;
        if (op.kind == "del" && op.len <= 0) continue;
        out << op;
    }
    return out;
}

CollabOp CollabMerge::transform(CollabOp op, const QList<CollabOp>& against) {
    for (const CollabOp& a : against) {
        if (a.kind == "ins") {
            if (a.pos < op.pos || (a.pos == op.pos && op.kind == "del"))
                op.pos += a.text.size();
        } else if (a.kind == "del") {
            const int dEnd = a.pos + a.len;
            if (dEnd <= op.pos) {
                op.pos -= a.len;
            } else if (a.pos < op.pos) {
                // Kısmi örtüşme: işlem silinen bölgenin başına çekilir
                const int overlap = dEnd - op.pos;
                op.pos = a.pos;
                if (op.kind == "del") op.len = qMax(0, op.len - overlap);
            } else if (op.kind == "del" && a.pos < op.pos + op.len) {
                op.len = qMax(0, op.len - qMin(op.pos + op.len, dEnd) + qMax(op.pos, a.pos));
            }
        }
    }
    op.pos = qMax(0, op.pos);
    return op;
}

QString CollabMerge::apply(const QString& local, const QList<CollabOp>& remoteOps,
                           const QList<CollabOp>& localOps, int& newRev) {
    QString out = local;
    // Uzak işlemleri artan konumda uygula (konum kaymasını önlemek için sondan başa)
    QList<CollabOp> ops;
    for (CollabOp o : remoteOps) ops << transform(o, localOps);
    std::sort(ops.begin(), ops.end(),
              [](const CollabOp& a, const CollabOp& b) { return a.pos > b.pos; });
    for (const CollabOp& o : ops) {
        if (o.kind == "ins") {
            const int p = qBound(0, o.pos, out.size());
            out.insert(p, o.text);
        } else {
            const int p = qBound(0, o.pos, out.size());
            out.remove(p, qMin(o.len, out.size() - p));
        }
    }
    ++newRev;
    return out;
}

QList<CollabOp> CollabMerge::diff(const QString& before, const QString& after) {
    QList<CollabOp> out;
    if (before == after) return out;
    int pre = 0;
    while (pre < before.size() && pre < after.size() && before[pre] == after[pre])
        ++pre;
    int suf = 0;
    while (suf < before.size() - pre && suf < after.size() - pre
           && before[before.size() - 1 - suf] == after[after.size() - 1 - suf])
        ++suf;
    const int delLen = before.size() - pre - suf;
    const QString ins = after.mid(pre, after.size() - pre - suf);
    if (delLen > 0) out << CollabOp{"del", pre, QString(), delLen};
    if (!ins.isEmpty()) out << CollabOp{"ins", pre, ins, 0};
    return out;
}
