#include "SelectionGrow.h"

QPair<int, int> SelectionGrow::grow(const QString& text, int start, int end) {
    const int n = text.size();
    if (n == 0) return {0, 0};
    start = qBound(0, start, n);
    end = qBound(0, end, n);
    if (start > end) qSwap(start, end);

    auto wordAt = [&](int pos) -> QPair<int, int> {
        int s = qBound(0, pos, n), e = s;
        while (s > 0 && isWordChar(text[s - 1])) --s;
        while (e < n && isWordChar(text[e])) ++e;
        return {s, e};
    };
    auto lineOf = [&](int pos, bool contentOnly) -> QPair<int, int> {
        int s = pos;
        while (s > 0 && text[s - 1] != '\n') --s;
        int e = pos;
        while (e < n && text[e] != '\n') ++e;
        if (contentOnly) {
            int cs = s, ce = e;
            while (cs < ce && (text[cs] == ' ' || text[cs] == '\t')) ++cs;
            while (ce > cs && (text[ce - 1] == ' ' || text[ce - 1] == '\t' || text[ce - 1] == '\r')) --ce;
            return {cs, ce};
        }
        if (e < n) ++e; // '\n' dahil (son satır hariç)
        return {s, e};
    };

    // 1) Seçim yoksa / kelimeden küçükse → kelime
    if (start == end || (end - start) == 0) {
        auto w = wordAt(start);
        if (w.second > w.first) return w;
        return lineOf(start, true);
    }
    // 2) Kelime seçiliyse → satır içeriği
    {
        auto w = wordAt(start);
        if (w.first == start && w.second == end)
            return lineOf(start, true);
    }
    // 3) Satır içeriği seçiliyse → tam satır
    {
        auto c = lineOf(start, true);
        if (c.first == start && c.second == end) {
            auto full = lineOf(start, false);
            if (full != c) return full;
        }
    }
    // 4) Tam satır(lar) seçiliyse → saran parantez bloğu
    {
        auto full = lineOf(start, false);
        auto fullEnd = lineOf(qMax(start, end - 1), false);
        if (full.first == start && fullEnd.second == end) {
            // Geriye doğru ilk kapanmamış açılışı bul
            int depth = 0;
            for (int i = start - 1; i >= 0; --i) {
                const QChar ch = text[i];
                if (ch == ')' || ch == '}' || ch == ']') ++depth;
                else if (ch == '(' || ch == '{' || ch == '[') {
                    if (depth == 0) {
                        // Eşleşen kapanışı bul
                        QChar want = (ch == '(') ? ')' : (ch == '{' ? '}' : ']');
                        int d2 = 0;
                        for (int j = i; j < n; ++j) {
                            if (text[j] == ch) ++d2;
                            else if (text[j] == want) {
                                if (--d2 == 0) {
                                    if (j + 1 > end) return {i, j + 1};
                                    break;
                                }
                            }
                        }
                        break;
                    }
                    --depth;
                }
            }
        }
    }
    // 5) Hepsi seçiliyse aynı kal, değilse belge
    if (start == 0 && end == n) return {start, end};
    return {0, n};
}
