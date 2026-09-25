#include "IconTheme.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QHash>
#include <QPainter>
#include <QRegularExpression>
#include <QSvgRenderer>

static QHash<QString, QPixmap> g_iconCache;

QString IconTheme::iconsDir() {
    const QString beside = QCoreApplication::applicationDirPath() + "/resources/icons";
    if (QDir(beside).exists()) return beside;
    return "resources/icons";
}

bool IconTheme::hasIcon(const QString& name) {
    return QFile::exists(QDir(iconsDir()).absoluteFilePath(name + ".svg"));
}

QStringList IconTheme::availableIcons() {
    QStringList out;
    QDir d(iconsDir());
    for (const QFileInfo& f : d.entryInfoList(QStringList{"*.svg"}, QDir::Files, QDir::Name))
        out << f.baseName();
    return out;
}

QByteArray IconTheme::recoloredSvg(const QString& name, const QColor& color, QString* err) {
    if (err) err->clear();
    QFile f(QDir(iconsDir()).absoluteFilePath(name + ".svg"));
    if (!f.open(QIODevice::ReadOnly)) {
        if (err) *err = "ikon yok: " + name;
        return {};
    }
    QString svg = QString::fromUtf8(f.readAll());
    svg.replace("currentColor", color.name());
    // sabit renkli fill/stroke'ları da hedef renge çevir (renkli ikon setleri için opsiyonel)
    return svg.toUtf8();
}

QString IconTheme::cacheKey(const QString& name, const QColor& c, int size) {
    return QString("%1|%2|%3|%4").arg(name, c.name()).arg(c.alpha()).arg(size);
}

QPixmap IconTheme::pixmap(const QString& name, const QColor& color, int size) {
    const QString key = cacheKey(name, color, size);
    const auto it = g_iconCache.constFind(key);
    if (it != g_iconCache.constEnd()) return *it;

    QString err;
    const QByteArray svg = recoloredSvg(name, color, &err);
    if (svg.isEmpty()) return QPixmap();

    qreal dpr = 1.0;
    if (auto* inst = QGuiApplication::instance())
        if (auto* gui = qobject_cast<QGuiApplication*>(inst))
            dpr = gui->devicePixelRatio();
    QPixmap pm(qRound(size * dpr), qRound(size * dpr));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    QSvgRenderer renderer(svg);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    renderer.render(&p, QRectF(0, 0, size, size));
    p.end();

    g_iconCache.insert(key, pm);
    return pm;
}

QIcon IconTheme::icon(const QString& name, const QColor& color, int size) {
    const QPixmap pm = pixmap(name, color, size);
    return pm.isNull() ? QIcon() : QIcon(pm);
}

void IconTheme::clearCache() {
    g_iconCache.clear();
}
