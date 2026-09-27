// Stage 47 testleri: kontrast matematiği, yüksek kontrast teması kilidi,
// dil kapsama, renk körü modları.
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>

#include "../src/core/A11yCheck.h"
#include "../src/core/ColorBlind.h"
#include "../src/core/LanguageManager.h"

static int g_pass = 0, g_fail = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (cond) {                                                            \
            ++g_pass;                                                          \
        } else {                                                                \
            ++g_fail;                                                          \
            fprintf(stderr, "FAIL %d %s\n", __LINE__, #cond);                    \
            fflush(stderr);                                                    \
        }                                                                      \
    } while (0)

#define CHECK2(cond, why)                                                      \
    do {                                                                       \
        if (cond) {                                                            \
            ++g_pass;                                                          \
        } else {                                                                \
            ++g_fail;                                                          \
            fprintf(stderr, "FAIL %d %s (%s)\n", __LINE__, #cond, why);          \
            fflush(stderr);                                                    \
        }                                                                      \
    } while (0)

// --- 1: WCAG matematiği ---
static void testContrastMath() {
    CHECK(qAbs(A11yCheck::luminance(QColor("#ffffff")) - 1.0) < 0.01);
    CHECK(qAbs(A11yCheck::luminance(QColor("#000000")) - 0.0) < 0.01);
    CHECK(qAbs(A11yCheck::contrastRatio(QColor("#fff"), QColor("#000")) - 21.0) < 0.1);
    CHECK(qAbs(A11yCheck::contrastRatio(QColor("#888"), QColor("#888")) - 1.0) < 0.01);
    // Sıra bağımsız
    CHECK(A11yCheck::contrastRatio(QColor("#fff"), QColor("#000")) ==
          A11yCheck::contrastRatio(QColor("#000"), QColor("#fff")));
    CHECK(A11yCheck::textPasses(QColor("#fff"), QColor("#000"), false));
    CHECK(!A11yCheck::textPasses(QColor("#777"), QColor("#888"), false));
    CHECK(A11yCheck::textPasses(QColor("#777"), QColor("#000"), true)); // büyük metin
}

// --- 2: yüksek kontrast teması kilidi (gerçek dosya) ---
static void testHighContrastTheme() {
    const QStringList candidates = {
        "resources/themes/high-contrast.json",
        "../resources/themes/high-contrast.json",
        "/home/mitsune/mitsune-editor/resources/themes/high-contrast.json",
    };
    QString path;
    for (const QString& c : candidates)
        if (QFile::exists(c)) path = c;
    CHECK2(!path.isEmpty(), "high-contrast.json bulunamadı");
    if (path.isEmpty()) return;
    QFile f(path);
    CHECK(f.open(QIODevice::ReadOnly));
    const QJsonObject colors =
        QJsonDocument::fromJson(f.readAll()).object().value("colors").toObject();
    CHECK(!colors.isEmpty());
    auto pair = [&](const char* fg, const char* bg, bool large = false) {
        const double r = A11yCheck::contrastRatio(QColor(colors.value(fg).toString()),
                                                  QColor(colors.value(bg).toString()));
        if (r < (large ? 3.0 : 4.5))
            fprintf(stderr, "NOT: %s/%s orani %.2f\n", fg, bg, r);
        return r >= (large ? 3.0 : 4.5);
    };
    CHECK2(pair("text", "bg"), "metin/zemin 4.5 altında");
    CHECK2(pair("textStrong", "bg"), "vurgulu metin 4.5 altında");
    CHECK2(pair("textDim", "bg"), "soluk metin 4.5 altında");
    CHECK2(pair("gutterText", "gutterBg"), "satır numarası 4.5 altında");
    CHECK2(pair("accent", "bg"), "vurgu 4.5 altında");
    CHECK2(pair("error", "bg"), "hata 4.5 altında");
}

// --- 3: dil kapsama ---
static void testLanguageCoverage() {
    LanguageManager& L = LanguageManager::instance();
    const QStringList keys = L.allKeys();
    CHECK(!keys.isEmpty());
    int missing = 0;
    for (const QString& k : keys) {
        if (!L.hasTranslation(k, "tr")) {
            ++missing;
            fprintf(stderr, "NOT: tr eksik: %s\n", qPrintable(k));
        }
        if (!L.hasTranslation(k, "en")) {
            ++missing;
            fprintf(stderr, "NOT: en eksik: %s\n", qPrintable(k));
        }
    }
    CHECK2(missing == 0, "eksik çeviri var (yukarıda)");
    // Dil değiştirme çalışıyor
    L.setLanguage("en");
    CHECK(L.lang() == "en");
    CHECK(!L.t(keys.first()).isEmpty());
    L.setLanguage("tr");
    CHECK(L.lang() == "tr");
}

// --- 4: renk körü modları ---
static void testColorBlind() {
    CHECK(ColorBlind::fromName("deuteranopia") == ColorBlind::Mode::Deuteranopia);
    CHECK(ColorBlind::fromName("protanopia") == ColorBlind::Mode::Protanopia);
    CHECK(ColorBlind::fromName("tritanopia") == ColorBlind::Mode::Tritanopia);
    CHECK(ColorBlind::fromName("yok") == ColorBlind::Mode::None);
    // Modlar gerçekten farklı dönüştürür
    const QColor red("#ff0000"), green("#00ff00");
    const QColor dR = ColorBlind::adjust(red, ColorBlind::Mode::Deuteranopia);
    const QColor pR = ColorBlind::adjust(red, ColorBlind::Mode::Protanopia);
    CHECK(dR != red || pR != red); // en az biri değiştirir
    CHECK(ColorBlind::adjust(red, ColorBlind::Mode::None) == red);
    // Adlar yuvarlanır
    CHECK(ColorBlind::fromName(ColorBlind::name(ColorBlind::Mode::Tritanopia)) ==
          ColorBlind::Mode::Tritanopia);
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir home;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, home.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, home.path());
    qputenv("XDG_DATA_HOME", home.path().toUtf8());
    qputenv("HOME", home.path().toUtf8());

    testContrastMath();
    testHighContrastTheme();
    testLanguageCoverage();
    testColorBlind();

    fprintf(stderr, "STAGE47: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
