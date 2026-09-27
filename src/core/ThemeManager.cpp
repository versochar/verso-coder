#include "ThemeManager.h"
#include "VersoPaths.h"
#include "ColorBlind.h"
#include "QssBuilder.h"
#include "ThemeStore.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

ThemeManager& ThemeManager::instance() {
    static ThemeManager m;
    return m;
}

QString ThemeManager::customPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/themes/custom.qss";
}

QStringList ThemeManager::availableThemes() const {
    QStringList t = ThemeStore::instance().themeNames();
    if (QFile::exists(customPath()) && !t.contains("custom")) t << "custom";
    return t;
}

bool ThemeManager::importCustomTheme(const QString& srcPath) {
    // Yol 1: JSON palet → ThemeStore'a
    if (srcPath.endsWith(".json", Qt::CaseInsensitive))
        return ThemeStore::instance().importTheme(srcPath);
    // Yol 2 (eski): QSS kopyala
    QString dst = customPath();
    QDir().mkpath(QFileInfo(dst).dir().absolutePath());
    QFile::remove(dst);
    return QFile::copy(srcPath, dst);
}

ThemeTokens ThemeManager::tokens() const {
    ThemeTokens t;
    if (m_hasPreview) t = m_preview;
    else t = ThemeStore::instance().theme(m_current);
    if (m_accent.isValid()) t.accent = m_accent;
    // Stage 12: renk körü dönüşümü (önizlemeye de uygulanır)
    return ColorBlind::applyTo(t, ColorBlind::fromName(m_vision));
}

// Stage 12: canlı önizleme
void ThemeManager::previewTokens(const ThemeTokens& tk) {
    m_preview = tk;
    m_hasPreview = true;
    applyTokens();
}

void ThemeManager::clearPreview() {
    if (!m_hasPreview) return;
    m_hasPreview = false;
    applyTokens();
}

void ThemeManager::onApplied(std::function<void()> cb) {
    m_callbacks << cb;
}

void ThemeManager::notify() {
    for (auto& cb : m_callbacks) cb();
}

void ThemeManager::setAccent(const QColor& c) {
    m_accent = c;
    applyTokens();
}

void ThemeManager::setTypography(const TypographySettings& t) {
    m_typo = t;
    applyTokens();
}

void ThemeManager::apply(const QString& themeName) {
    m_current = themeName;
    // JSON palet varsa token hattı; yoksa eski QSS fallback
    if (ThemeStore::instance().hasTheme(themeName)) {
        applyTokens();
        return;
    }
    QStringList candidates;
    if (themeName == "custom") candidates << customPath();
    candidates << VersoPaths::subDir("themes") + QString("/%1.qss").arg(themeName)
               << QString("resources/themes/%1.qss").arg(themeName)
               << QCoreApplication::applicationDirPath()
                      + QString("/resources/themes/%1.qss").arg(themeName);
    for (const QString& p : candidates) {
        QFile f(p);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
            if (!m_typo.uiFamily.isEmpty())
                qApp->setFont(Typography::uiFont(m_typo));
            notify();
            return;
        }
    }
    // hiçbir şey bulunamadı → güvenli token varsayılanı
    applyTokens();
}

void ThemeManager::applyTokens() {
    QssBuilder::Input in;
    in.tokens = tokens();
    in.accentOverride = m_accent;
    in.typo = m_typo;
    in.selectionOpacity = m_selOpacity;
    qApp->setStyleSheet(QssBuilder::build(in));
    qApp->setFont(Typography::uiFont(m_typo));
    notify();
}
