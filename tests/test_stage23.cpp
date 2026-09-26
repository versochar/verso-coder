// Stage 23 testleri: piksel-hizalı ızgara ölçümü (saf mantık).
// Çıkış kodu 0 = hepsi geçti.
#include <QApplication>
#include <QFontMetrics>
#include <cstdio>

#include "../src/core/GridCheck.h"

static int g_pass = 0, g_fail = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (cond) {                                                            \
            ++g_pass;                                                          \
        } else {                                                               \
            ++g_fail;                                                          \
            fprintf(stderr, "FAIL %d %s\n", __LINE__, #cond);                  \
            fflush(stderr);                                                    \
        }                                                                      \
    } while (0)

static void testMono() {
    const QFont mono(GridCheck::systemMonospace());
    CHECK(!GridCheck::systemMonospace().isEmpty());
    const int w = GridCheck::cellWidth(mono);
    CHECK(w > 0 && GridCheck::isMonospace(mono));
    // Kriter: "iere"/"teat" alt alta — her harf aynı pikselde başlar
    const QString a = "iere", b = "teat";
    CHECK(a.size() == b.size());
    for (int i = 0; i < a.size(); ++i) {
        CHECK(GridCheck::columnX(mono, i) == i * w);
    }
    // Ölçüm tutarlılığı: her harfin advance'ı hücreye eşit
    QFontMetrics fm(mono);
    for (QChar ch : a)
        CHECK(fm.horizontalAdvance(ch) == w);
    for (QChar ch : b)
        CHECK(fm.horizontalAdvance(ch) == w);
}

static void testColumnX() {
    const QFont mono(GridCheck::systemMonospace());
    const int w = GridCheck::cellWidth(mono);
    if (w <= 0) {
        fprintf(stderr, "SKIP columnX (monospace yok)\n");
        return;
    }
    CHECK(GridCheck::columnX(mono, 0) == 0);
    CHECK(GridCheck::columnX(mono, 5) == 5 * w);
    CHECK(GridCheck::columnX(mono, 40) == 40 * w);
}

int main(int argc, char** argv) {
    QApplication app(argc, argv); // QFontMetrics için
    testMono();
    testColumnX();
    fprintf(stderr, "STAGE23: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
