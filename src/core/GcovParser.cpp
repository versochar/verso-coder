#include "GcovParser.h"

bool GcovParser::parseLine(const QString& raw, GcovLine& out) {
    // "        5:   12:kod" | "    #####:   12:kod" | "        -:    0:Source:..." | "====="
    if (raw.startsWith("=====")) return false;
    const int c1 = raw.indexOf(':');
    if (c1 < 0) return false;
    const int c2 = raw.indexOf(':', c1 + 1);
    if (c2 < 0) return false;
    const QString hits = raw.left(c1).trimmed();
    bool ok = false;
    const int line = raw.mid(c1 + 1, c2 - c1 - 1).trimmed().toInt(&ok);
    if (!ok || line <= 0) return false;
    out.line = line;
    if (hits == "-") out.hits = -1;
    else if (hits.startsWith('#')) out.hits = 0;
    else out.hits = hits.toLong();
    return true;
}

GcovFile GcovParser::parse(const QString& gcovText) {
    GcovFile f;
    for (const QString& raw : gcovText.split('\n')) {
        if (raw.startsWith("        -:    0:Source:")) {
            f.source = raw.mid(raw.indexOf("Source:") + 7).trimmed();
            continue;
        }
        GcovLine l;
        if (parseLine(raw, l)) {
            if (l.hits < 0) continue; // çalıştırılamaz satırları atla
            // Aynı satır birden çok kez geçebilir (şablon/makro) — en büyüğü al
            bool found = false;
            for (GcovLine& e : f.lines)
                if (e.line == l.line) {
                    e.hits = qMax(e.hits, l.hits);
                    found = true;
                    break;
                }
            if (!found) f.lines << l;
        }
    }
    std::sort(f.lines.begin(), f.lines.end(),
              [](const GcovLine& a, const GcovLine& b) { return a.line < b.line; });
    return f;
}

int GcovFile::covered() const {
    int n = 0;
    for (const GcovLine& l : lines)
        if (l.hits > 0) ++n;
    return n;
}

int GcovFile::coverable() const {
    int n = 0;
    for (const GcovLine& l : lines)
        if (l.hits >= 0) ++n;
    return n;
}

double GcovFile::percent() const {
    const int t = coverable();
    return t == 0 ? 100.0 : 100.0 * covered() / t;
}
