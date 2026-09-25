#include "FileIcons.h"
#include <QDir>
#include <QPainter>
#include <QPixmap>

QColor FileIconProvider::colorFor(const QString& suffix) {
    static const QMap<QString, QColor> map = {
        {"cpp", "#569cd6"}, {"h", "#569cd6"}, {"hpp", "#569cd6"}, {"c", "#4ec9b0"},
        {"py", "#3572A5"},  {"js", "#d7ba7d"}, {"ts", "#3178c6"}, {"qml", "#41cd52"},
        {"md", "#858585"},  {"txt", "#858585"}, {"json", "#d7ba7d"}, {"qss", "#c586c0"},
        {"pro", "#6a9955"}, {"cmake", "#e05050"}, {"sh", "#6a9955"}, {"java", "#e05050"},
        {"rs", "#e0a050"},  {"go", "#00ADD8"},
    };
    return map.value(suffix.toLower(), QColor("#858585"));
}

QIcon FileIconProvider::icon(IconType type) const {
    return QFileIconProvider::icon(type);
}

QIcon FileIconProvider::icon(const QFileInfo& info) const {
    if (info.isDir()) return QFileIconProvider::icon(info);
    QColor c = colorFor(info.suffix());
    QPixmap pm(16, 16);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(c.darker(150));
    p.setPen(c);
    p.drawRoundedRect(1, 2, 14, 12, 2, 2);
    p.setPen(Qt::white);
    p.setFont(QFont("monospace", 7, QFont::Bold));
    QString letter = info.suffix().left(1).toUpper();
    if (letter.isEmpty()) letter = "?";
    p.drawText(pm.rect(), Qt::AlignCenter, letter);
    return QIcon(pm);
}
