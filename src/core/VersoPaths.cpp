#include "VersoPaths.h"
#include <QCoreApplication>
#include <QDir>

#ifndef VERSO_DATA_DIR
#define VERSO_DATA_DIR "/usr/share/verso-coder"
#endif

QString VersoPaths::resourceDir() {
    // 1) binary yanı (derleme ağacı + AppImage + Windows/macOS paketi)
    const QString beside = QCoreApplication::applicationDirPath() + "/resources";
    if (QDir(beside).exists()) return beside;
    // 2) çalışma dizini (geliştirici: proje kökünden çalıştırma)
    if (QDir(QStringLiteral("resources")).exists()) return QStringLiteral("resources");
    // 3) sistem kurulumu (cmake --install / AUR paketi)
    if (QDir(QString::fromUtf8(VERSO_DATA_DIR)).exists())
        return QString::fromUtf8(VERSO_DATA_DIR);
    return beside; // yoksa bile yanında ara (eski davranış)
}

QString VersoPaths::subDir(const QString& sub) {
    const QString root = resourceDir();
    if (root == QStringLiteral("resources")) return root + "/" + sub;
    return QDir(root).filePath(sub);
}
