#include "Typography.h"
#include <QFontDatabase>
#include <QFontMetrics>
#include <QStringList>
#include <QtGlobal>

static const char* kDefaultUiFamily = "Inter, Segoe UI, DejaVu Sans, system-ui, sans-serif";
static const char* kDefaultEditorFamily = "JetBrains Mono, Fira Code, Consolas, DejaVu Sans Mono, monospace";

static void applySpacingAndFeatures(QFont& f, const TypographySettings& t) {
    if (t.letterSpacing != 0.0)
        f.setLetterSpacing(QFont::PercentageSpacing, 100.0 + t.letterSpacing);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    // Ligature kontrolü (Qt 6.7+): kapatma isteğinde "liga" ve "clig" özellikleri söndürülür
    if (!t.ligatures) {
        f.setFeature(QFont::Tag("liga"), 0u);
        f.setFeature(QFont::Tag("clig"), 0u);
    }
#else
    Q_UNUSED(t);
#endif
}

QFont Typography::uiFont(const TypographySettings& t) {
    QFont f(QString::fromUtf8(kDefaultUiFamily));
    if (!t.uiFamily.isEmpty()) f.setFamilies(QStringList{t.uiFamily});
    f.setPixelSize(qMax(9, t.uiSize));
    applySpacingAndFeatures(f, t);
    return f;
}

QFont Typography::editorFont(const TypographySettings& t) {
    QFont f(QString::fromUtf8(kDefaultEditorFamily));
    if (!t.editorFamily.isEmpty()) f.setFamilies(QStringList{t.editorFamily});
    f.setPointSize(qMax(6, t.editorSize));
    applySpacingAndFeatures(f, t);
    return f;
}

int Typography::lineSpacingPx(const QFont& f, double lineHeight) {
    return int(QFontMetrics(f).height() * qBound(1.0, lineHeight, 2.0) + 0.5);
}

double Typography::clampLineHeight(double v) { return qBound(1.0, v, 2.0); }
double Typography::clampLetterSpacing(double v) { return qBound(-5.0, v, 25.0); }

QStringList Typography::editorFontSuggestions() {
    QStringList out;
    const QStringList fams = QFontDatabase::families();
    for (const QString& f : fams)
        if (f.contains("Mono", Qt::CaseInsensitive) || f.contains("Code", Qt::CaseInsensitive)
            || f.contains("Consol", Qt::CaseInsensitive) || f.contains("Courier", Qt::CaseInsensitive))
            out << f;
    if (out.isEmpty()) out = fams.mid(0, qMin(6, fams.size()));
    return out;
}
