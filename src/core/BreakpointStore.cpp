#include "BreakpointStore.h"
#include <QSettings>

static const char* kGroup = "breakpoints";
static const char* kKey = "entries"; // tek anahtar: "dosya\x1fsatır\x1fenabled\x1fkoşul\x1flog"

// Dosya yolları QSettings anahtarı olamaz ('/' grup ayracıdır) —
// bu yüzden tüm noktalar tek listede, \x1f ayraçlı saklanır.
void BreakpointStore::load(QList<Breakpoint>& out) const {
    out.clear();
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    for (const QString& row : q.value(kKey).toStringList()) {
        const QStringList p = row.split('\x1f');
        if (p.size() < 3) continue;
        Breakpoint b;
        b.file = p[0];
        b.line = p[1].toInt();
        b.enabled = p[2] != "0";
        if (p.size() > 3) b.condition = p[3];
        if (p.size() > 4) b.log = p[4];
        if (p.size() > 5) b.hitCount = p[5].toInt(); // Stage 26
        if (!b.file.isEmpty() && b.line > 0) out << b;
    }
}

void BreakpointStore::save(const QList<Breakpoint>& bps) const {
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    QStringList rows;
    for (const Breakpoint& b : bps)
        rows << QString("%1\x1f%2\x1f%3\x1f%4\x1f%5\x1f%6")
                    .arg(b.file)
                    .arg(b.line)
                    .arg(b.enabled ? 1 : 0)
                    .arg(b.condition)
                    .arg(b.log)
                    .arg(b.hitCount);
    q.setValue(kKey, rows);
}

QList<Breakpoint> BreakpointStore::all() const {
    QList<Breakpoint> out;
    load(out);
    return out;
}

QList<Breakpoint> BreakpointStore::forFile(const QString& file) const {
    QList<Breakpoint> out;
    for (const Breakpoint& b : all())
        if (b.file == file) out << b;
    std::sort(out.begin(), out.end(),
              [](const Breakpoint& a, const Breakpoint& b) { return a.line < b.line; });
    return out;
}

bool BreakpointStore::has(const QString& file, int line) const {
    for (const Breakpoint& b : forFile(file))
        if (b.line == line) return true;
    return false;
}

bool BreakpointStore::toggle(const QString& file, int line) {
    QList<Breakpoint> bps = all();
    for (int i = 0; i < bps.size(); ++i)
        if (bps[i].file == file && bps[i].line == line) {
            bps.removeAt(i);
            save(bps);
            return true; // vardı, kaldırıldı
        }
    Breakpoint b;
    b.file = file;
    b.line = line;
    bps << b;
    save(bps);
    return false;
}

void BreakpointStore::setEnabled(const QString& file, int line, bool on) {
    QList<Breakpoint> bps = all();
    for (Breakpoint& b : bps)
        if (b.file == file && b.line == line) b.enabled = on;
    save(bps);
}

void BreakpointStore::setCondition(const QString& file, int line, const QString& cond) {
    QList<Breakpoint> bps = all();
    for (Breakpoint& b : bps)
        if (b.file == file && b.line == line) b.condition = cond;
    save(bps);
}

void BreakpointStore::setHitCount(const QString& file, int line, int n) {
    QList<Breakpoint> bps = all();
    for (Breakpoint& b : bps)
        if (b.file == file && b.line == line) b.hitCount = qMax(0, n);
    save(bps);
}

void BreakpointStore::remove(const QString& file, int line) {
    QList<Breakpoint> bps = all();
    for (int i = bps.size() - 1; i >= 0; --i)
        if (bps[i].file == file && bps[i].line == line) bps.removeAt(i);
    save(bps);
}

void BreakpointStore::clearFile(const QString& file) {
    QList<Breakpoint> bps = all();
    for (int i = bps.size() - 1; i >= 0; --i)
        if (bps[i].file == file) bps.removeAt(i);
    save(bps);
}

void BreakpointStore::clearAll() {
    QSettings q("Verso", "VersoCoder");
    q.beginGroup(kGroup);
    q.remove(kKey);
    // Eski şemadan kalma anahtar/grupları da temizle
    for (const QString& k : q.childKeys()) q.remove(k);
    for (const QString& g : q.childGroups()) q.remove(g);
}
