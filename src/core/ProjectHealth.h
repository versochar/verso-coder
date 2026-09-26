#pragma once
#include <QString>
#include <QStringList>

// Stage 34: proje sağlık skoru. Puanlama saf mantıktır; tarayıcı ayrıdır.
struct ProjectHealth {
    int score = 0;      // 0..100
    QString grade;      // A | B | C | D
    int files = 0;
    int todoCount = 0;
    int fixmeCount = 0;
    int longFiles = 0;   // > 400 satır
    int hugeFiles = 0;   // > 1000 satır
    int maxFileLines = 0;
    QString maxFilePath;
    int buildOk = -1;    // -1 bilinmiyor, 0 hatalı, 1 başarılı
    int testsOk = -1;
    bool dirtyGit = false;
    QStringList notes;   // öneriler (en fazla 6)

    QString summary() const;
    // Saf puanlama — girdi doğrudan verilir, dosya sistemi yok.
    static ProjectHealth scoreOf(int files, int todo, int fixme, int longFiles, int hugeFiles,
                                 int buildOk, int testsOk, bool dirtyGit,
                                 int maxFileLines = 0, const QString& maxFilePath = QString());
    static QString gradeFor(int score);
    // Kaynak taraması (TODO/FIXME/uzun dosya) — dosya sistemi gerekir.
    static ProjectHealth scan(const QString& root, int maxFiles = 2000);
    static bool isSourceExt(const QString& suffix);
};
