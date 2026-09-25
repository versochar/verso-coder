#pragma once
#include <QString>
#include <QStringList>

// Stage 14: başlatma yapılandırması — <root>/.verso/launch.json (+ .mitsune okuma).
struct LaunchConfig {
    QString program;          // çalıştırılacak binary (göreli olabilir)
    QStringList args;
    QString cwd;              // boşsa proje kökü
    QString preBuild;         // task etiketi (boşsa yok)
    bool stopAtEntry = false; // main'de dur

    static QString configPath(const QString& root);
    static LaunchConfig load(const QString& root);
    bool save(const QString& root) const;
    static LaunchConfig defaults(const QString& root);
    QString resolvedProgram(const QString& root) const;
    QString resolvedCwd(const QString& root) const;
};
