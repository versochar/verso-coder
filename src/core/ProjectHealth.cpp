#include "ProjectHealth.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QStringList>

bool ProjectHealth::isSourceExt(const QString& suffix) {
    static const QSet<QString> exts = {"cpp", "h", "hpp", "c", "py", "js", "ts", "qml",
                                       "java", "rs", "go", "cs", "rb", "php", "swift", "kt"};
    return exts.contains(suffix.toLower());
}

QString ProjectHealth::gradeFor(int score) {
    if (score >= 85) return "A";
    if (score >= 70) return "B";
    if (score >= 50) return "C";
    return "D";
}

ProjectHealth ProjectHealth::scoreOf(int files, int todo, int fixme, int longFiles, int hugeFiles,
                                     int buildOk, int testsOk, bool dirtyGit, int maxFileLines,
                                     const QString& maxFilePath) {
    ProjectHealth h;
    h.files = files;
    h.todoCount = todo;
    h.fixmeCount = fixme;
    h.longFiles = longFiles;
    h.hugeFiles = hugeFiles;
    h.buildOk = buildOk;
    h.testsOk = testsOk;
    h.dirtyGit = dirtyGit;
    h.maxFileLines = maxFileLines;
    h.maxFilePath = maxFilePath;

    double s = 100.0;
    // Derleme / test durumu en ağır ceza
    if (buildOk == 0) s -= 35;
    if (testsOk == 0) s -= 25;
    if (files <= 0) s -= 20; // taranacak kaynak yok

    // TODO/FIXME yoğunluğu (1000 satır başına)
    const int lines = qMax(1, files) * 150; // kaba satır tahmini
    const double density = double(todo + fixme) * 1000.0 / double(lines);
    s -= qBound(0.0, density * 6.0, 25.0);

    // Uzun dosyalar bakım maliyeti
    s -= qBound(0, hugeFiles * 6, 24);
    s -= qBound(0, longFiles * 2, 16);
    if (maxFileLines > 1500) s -= 6;

    if (dirtyGit) s -= 5; // commit edilmemiş değişiklik

    h.score = int(qBound(0.0, s, 100.0));
    h.grade = gradeFor(h.score);

    if (buildOk == 0) h.notes << "Derleme hatalı — önce build'i düzelt.";
    else if (buildOk == -1) h.notes << "Derleme durumu bilinmiyor.";
    if (testsOk == 0) h.notes << "Testler başarısız.";
    else if (testsOk == -1 && files > 0) h.notes << "Test durumu bilinmiyor.";
    if (todo + fixme > 10) h.notes << QString("%1 TODO/FIXME var — temizleme oturumu planla.")
                                    .arg(todo + fixme);
    if (hugeFiles > 0) h.notes << QString("%1 dosya 1000+ satır; bölünmeli.").arg(hugeFiles);
    if (longFiles > 5) h.notes << QString("%1 dosya 400+ satır; okunabilirlik riski.").arg(longFiles);
    if (dirtyGit) h.notes << "Çalışma ağacı kirli; commit et.";
    while (h.notes.size() > 6) h.notes.removeLast();
    return h;
}

QString ProjectHealth::summary() const {
    return QString("Sağlık %1/100 (%2) · %3 dosya · TODO %4 · FIXME %5")
        .arg(score)
        .arg(grade)
        .arg(files)
        .arg(todoCount)
        .arg(fixmeCount);
}

ProjectHealth ProjectHealth::scan(const QString& root, int maxFiles) {
    int files = 0, todo = 0, fixme = 0, longFiles = 0, hugeFiles = 0;
    int maxLines = 0;
    QString maxPath;
    if (root.isEmpty()) return scoreOf(0, 0, 0, 0, 0, -1, -1, false);
    QDirIterator it(root, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext() && files < maxFiles) {
        const QString p = it.next();
        if (p.contains("/.git/") || p.contains("/build/") || p.contains("/node_modules/") ||
            p.contains("/.venv/") || p.contains("/__pycache__/"))
            continue;
        if (!isSourceExt(QFileInfo(p).suffix())) continue;
        QFile f(p);
        if (!f.open(QIODevice::ReadOnly)) continue;
        const QByteArray raw = f.read(600 * 1024);
        if (raw.contains('\0')) continue;
        const QStringList lines = QString::fromUtf8(raw).split('\n');
        ++files;
        for (const QString& line : lines) {
            if (line.contains("TODO") || line.contains("TODO(")) ++todo;
            if (line.contains("FIXME") || line.contains("XXX") || line.contains("HACK")) ++fixme;
        }
        const int n = lines.size();
        if (n > 1000) ++hugeFiles;
        else if (n > 400) ++longFiles;
        if (n > maxLines) { maxLines = n; maxPath = p; }
    }
    return scoreOf(files, todo, fixme, longFiles, hugeFiles, -1, -1, false, maxLines, maxPath);
}
