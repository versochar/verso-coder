// Stage 22 testleri: erişilebilirlik (kontrast) + çeviri kapsama.
// Çıkış kodu 0 = hepsi geçti.
#include <QCoreApplication>
#include <cstdio>

#include "../src/core/A11yCheck.h"
#include "../src/core/LanguageManager.h"
#include "../src/core/ThemeTokens.h"

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

static void testContrast() {
    // Beyaz/siyah oranı ~21
    const double bw = A11yCheck::contrastRatio(QColor("#ffffff"), QColor("#000000"));
    CHECK(bw > 20.0 && bw <= 21.0);
    // Aynı renk oranı 1
    CHECK(A11yCheck::contrastRatio(QColor("#888888"), QColor("#888888")) == 1.0);
    // Simetri
    CHECK(A11yCheck::contrastRatio(QColor("#ffffff"), QColor("#123456")) ==
          A11yCheck::contrastRatio(QColor("#123456"), QColor("#ffffff")));
    CHECK(A11yCheck::textPasses(QColor("#ffffff"), QColor("#000000")));
    CHECK(!A11yCheck::textPasses(QColor("#777777"), QColor("#888888")));
    CHECK(A11yCheck::fontSizeOk(11) && !A11yCheck::fontSizeOk(7) &&
          !A11yCheck::fontSizeOk(30));
}

static void testTheme() {
    ThemeTokens tk;
    tk.name = "test";
    tk.bg = QColor("#1e1e1e");
    tk.text = QColor("#d4d4d4");
    tk.textStrong = QColor("#ffffff");
    tk.textDim = QColor("#9a9a9a");
    tk.accent = QColor("#007acc");
    const QStringList ok = A11yCheck::checkTheme(tk);
    CHECK(ok.size() == 1 && ok[0] == "geçti");
    ThemeTokens bad;
    bad.name = "kotu";
    bad.bg = QColor("#888888");
    bad.text = QColor("#777777");
    bad.textStrong = QColor("#888888");
    bad.textDim = QColor("#999999");
    bad.accent = QColor("#888888");
    const QStringList issues = A11yCheck::checkTheme(bad);
    CHECK(issues.size() > 1 && !issues.contains("geçti"));
}

static void testI18n() {
    const QStringList keys = LanguageManager::instance().allKeys();
    CHECK(!keys.isEmpty());
    for (const QString& k : keys) {
        if (!LanguageManager::instance().hasTranslation(k, "tr")) {
            fprintf(stderr, "FAIL missing tr: %s\n", qPrintable(k));
            ++g_fail;
        } else {
            ++g_pass;
        }
        if (!LanguageManager::instance().hasTranslation(k, "en")) {
            fprintf(stderr, "FAIL missing en: %s\n", qPrintable(k));
            ++g_fail;
        } else {
            ++g_pass;
        }
    }
    CHECK(!LanguageManager::instance().hasTranslation("yok-boyle-anahtar", "tr"));
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    testContrast();
    testTheme();
    testI18n();
    fprintf(stderr, "STAGE22: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
