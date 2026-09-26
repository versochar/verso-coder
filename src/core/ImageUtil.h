#pragma once
#include <QByteArray>
#include <QImage>
#include <QString>

// Stage 33: görü bağlamı için resim hazırlama (ölçekleme + sıkıştırma).
struct PreparedImage {
    QByteArray bytes;
    QString mime = "image/jpeg";
    int width = 0;
    int height = 0;
    bool valid() const { return !bytes.isEmpty() && width > 0 && height > 0; }
};

class ImageUtil {
public:
    // En uzun kenarı maxDim'e indir (küçükse dokunmaz).
    static QImage scaled(const QImage& src, int maxDim = 1024);
    // Ölçekle + JPEG ile sıkıştır (saf/test edilebilir).
    static PreparedImage prepare(const QImage& src, int maxDim = 1024, int quality = 80);
    static bool withinBudget(const QByteArray& bytes, qint64 maxBytes = 4LL * 1024 * 1024);
    static QByteArray base64(const QByteArray& bytes);
    // Uzantıya göre resim mi?
    static bool isImagePath(const QString& path);
};
