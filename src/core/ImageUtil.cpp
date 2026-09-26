#include "ImageUtil.h"
#include <QBuffer>
#include <QFileInfo>
#include <QStringList>

QImage ImageUtil::scaled(const QImage& src, int maxDim) {
    if (src.isNull() || maxDim <= 0) return src;
    const int longest = qMax(src.width(), src.height());
    if (longest <= maxDim) return src;
    return src.scaled(maxDim, maxDim, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

PreparedImage ImageUtil::prepare(const QImage& src, int maxDim, int quality) {
    PreparedImage out;
    QImage img = scaled(src, maxDim);
    if (img.isNull()) return out;
    if (img.format() != QImage::Format_RGB888 &&
        img.format() != QImage::Format_RGB32 &&
        img.format() != QImage::Format_ARGB32)
        img = img.convertToFormat(QImage::Format_RGB888);
    QBuffer buf(&out.bytes);
    if (!buf.open(QIODevice::WriteOnly)) return {};
    if (!img.save(&buf, "JPEG", qBound(10, quality, 100))) return {};
    out.mime = "image/jpeg";
    out.width = img.width();
    out.height = img.height();
    return out;
}

bool ImageUtil::withinBudget(const QByteArray& bytes, qint64 maxBytes) {
    return bytes.size() <= maxBytes;
}

QByteArray ImageUtil::base64(const QByteArray& bytes) {
    return bytes.toBase64();
}

bool ImageUtil::isImagePath(const QString& path) {
    static const QStringList exts = {"png", "jpg", "jpeg", "gif", "bmp", "webp"};
    return exts.contains(QFileInfo(path).suffix().toLower());
}
