#include "PatchQueue.h"
#include <QDir>
#include <QFileInfo>
#include <QStringList>

void PatchQueue::add(const QueuedEdit& e) {
    for (QueuedEdit& x : m_edits)
        if (x.path == e.path) { // özgün içeriği koru, yeni içeriği güncelle
            x.newText = e.newText;
            x.wholeFile = e.wholeFile;
            return;
        }
    m_edits << e;
}

// --- Basit LCS tabanlı satır diff'i ---
namespace {
struct Op { char kind; QString text; }; // ' ' bağlam, '-' sil, '+' ekle

QList<Op> lcsOps(const QStringList& a, const QStringList& b) {
    const int n = a.size(), m = b.size();
    QList<Op> ops;
    if (qint64(n) * qint64(m) > 4'000'000) { // çok büyük: tümünü değiştir
        for (const QString& l : a) ops << Op{'-', l};
        for (const QString& l : b) ops << Op{'+', l};
        return ops;
    }
    // DP (n+1)x(m+1)
    QList<QList<int>> dp(n + 1, QList<int>(m + 1, 0));
    for (int i = n - 1; i >= 0; --i)
        for (int j = m - 1; j >= 0; --j)
            dp[i][j] = (a[i] == b[j]) ? dp[i + 1][j + 1] + 1
                                       : qMax(dp[i + 1][j], dp[i][j + 1]);
    int i = 0, j = 0;
    while (i < n && j < m) {
        if (a[i] == b[j]) { ops << Op{' ', a[i]}; ++i; ++j; }
        else if (dp[i + 1][j] >= dp[i][j + 1]) { ops << Op{'-', a[i]}; ++i; }
        else { ops << Op{'+', b[j]}; ++j; }
    }
    while (i < n) ops << Op{'-', a[i++]};
    while (j < m) ops << Op{'+', b[j++]};
    return ops;
}
} // namespace

QString PatchQueue::unifiedDiff(const QueuedEdit& e, int context) {
    const QString rel = QFileInfo(e.path).fileName();
    const QStringList a = e.oldText.isEmpty() ? QStringList() : e.oldText.split('\n');
    const QStringList b = e.newText.isEmpty() ? QStringList() : e.newText.split('\n');
    const QList<Op> ops = lcsOps(a, b);

    QString out;
    out += QString("diff --git a/%1 b/%1\n").arg(rel);
    out += QString("--- a/%1\n+++ b/%1\n").arg(rel);

    // Değişen satırların op indekslerini topla
    QList<int> changes;
    for (int i = 0; i < ops.size(); ++i)
        if (ops[i].kind != ' ') changes << i;
    if (changes.isEmpty()) return out + "(değişiklik yok)\n";

    // context mesafesine göre hunk grupları ([start,end) op aralığı)
    QList<QPair<int, int>> groups;
    int gs = -1, ge = -1;
    for (int c : changes) {
        const int s = qMax(0, c - context);
        const int en = qMin(int(ops.size()), c + context + 1);
        if (gs < 0) { gs = s; ge = en; }
        else if (s <= ge) { ge = qMax(ge, en); }
        else { groups << qMakePair(gs, ge); gs = s; ge = en; }
    }
    groups << qMakePair(gs, ge);

    int oldNo = 1, newNo = 1, idx = 0;
    for (const auto& gr : groups) {
        const int gs2 = gr.first, ge2 = gr.second;
        while (idx < gs2) {
            if (ops[idx].kind != '+') ++oldNo;
            if (ops[idx].kind != '-') ++newNo;
            ++idx;
        }
        const int aStart = oldNo, bStart = newNo;
        int aCount = 0, bCount = 0;
        QString body;
        while (idx < ge2) {
            const Op& o = ops[idx];
            body += o.kind + o.text + "\n";
            if (o.kind != '+') { ++aCount; ++oldNo; }
            if (o.kind != '-') { ++bCount; ++newNo; }
            ++idx;
        }
        out += QString("@@ -%1,%2 +%3,%4 @@\n").arg(aStart).arg(aCount).arg(bStart).arg(bCount);
        out += body;
    }
    return out;
}

QString PatchQueue::allDiffs(const QList<QueuedEdit>& edits) {
    QString out;
    for (const QueuedEdit& e : edits) out += unifiedDiff(e) + "\n";
    return out;
}
