#include "ContextResolver.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

QStringList ContextResolver::parseMentions(const QString& input) {
    static const QRegularExpression rx(
        "@(dosya|file|seçim|secim|selection|sorunlar|problems|proje|project)(?::([^\\s]+))?",
        QRegularExpression::CaseInsensitiveOption);
    QStringList out;
    for (auto it = rx.globalMatch(input); it.hasNext();) {
        auto m = it.next();
        out << (m.captured(2).isEmpty() ? m.captured(0) : m.captured(0));
    }
    return out;
}

bool ContextResolver::hasMentions(const QString& input) {
    return !parseMentions(input).isEmpty();
}

QString ContextResolver::readRange(const QString& abs, int from, int to, QStringList& files, int maxChars) {
    QFile f(abs);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    QStringList lines = QString::fromUtf8(f.read(1024 * 1024)).split('\n');
    int a = from > 0 ? from - 1 : 0;
    int b = (to > 0) ? qMin(lines.size(), to) : lines.size();
    QString body;
    for (int i = a; i < b && body.size() < maxChars; ++i)
        body += lines[i] + "\n";
    files << abs;
    QString rel = QFileInfo(abs).fileName();
    return QString("\n[Dosya: %1%2]\n```\n%3```\n")
        .arg(abs)
        .arg(from > 0 ? QString(":%1-%2").arg(from).arg(to) : QString())
        .arg(body.left(maxChars));
}

QString ContextResolver::listTree(const QString& root, int maxEntries) {
    QStringList out;
    QDirIterator it(root, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext() && out.size() < maxEntries) {
        QString p = it.next();
        if (p.contains("/.git/") || p.contains("/build/")) continue;
        out << QDir(root).relativeFilePath(p);
    }
    return out.join('\n');
}

ContextResolver::Resolved ContextResolver::resolve(const QString& input, const QString& root,
                                                   const QString& currentFile, const QString& selection,
                                                   const QString& problems, int maxChars) const {
    Resolved r;
    static const QRegularExpression rx(
        "@(dosya|file|seçim|secim|selection|sorunlar|problems|proje|project)(?::([^\\s]+))?",
        QRegularExpression::CaseInsensitiveOption);
    int budget = maxChars;
    for (auto it = rx.globalMatch(input); it.hasNext();) {
        auto m = it.next();
        const QString kind = m.captured(1).toLower();
        const QString arg = m.captured(2);
        if (kind == "seçim" || kind == "secim" || kind == "selection") {
            if (selection.isEmpty()) { r.warnings << "@seçim: seçili kod yok"; continue; }
            r.text += "\n[Seçili kod]\n```\n" + selection.left(budget) + "\n```\n";
            budget = qMax(0, budget - selection.size());
        } else if (kind == "sorunlar" || kind == "problems") {
            if (problems.isEmpty()) { r.warnings << "@sorunlar: sorun yok"; continue; }
            r.text += "\n[Sorunlar]\n```\n" + problems.left(4000) + "\n```\n";
            budget = qMax(0, budget - problems.size());
        } else if (kind == "proje" || kind == "project") {
            if (!arg.isEmpty() && projectSearcher) {
                r.text += "\n[Proje araması: " + arg + "]\n```\n" + projectSearcher(arg).left(6000) + "\n```\n";
            } else {
                r.text += "\n[Proje dosyaları]\n```\n" + listTree(root, 200) + "\n```\n";
            }
            budget = qMax(0, budget - 4000);
        } else { // dosya / file
            QString spec = arg;
            if (spec.isEmpty() && !currentFile.isEmpty()) spec = currentFile;
            if (spec.isEmpty()) { r.warnings << "@dosya: yol belirtilmedi"; continue; }
            int from = 0, to = 0;
            QString pathPart = spec;
            int colon = spec.lastIndexOf(':');
            if (colon > 1 && spec.indexOf('-', colon) >= 0) {
                const QString range = spec.mid(colon + 1);
                pathPart = spec.left(colon);
                const QStringList pr = range.split('-');
                if (!pr.isEmpty()) from = pr[0].toInt();
                if (pr.size() > 1) to = pr[1].toInt();
            }
            QString abs = QDir::isAbsolutePath(pathPart)
                              ? pathPart
                              : QDir(root).absoluteFilePath(pathPart);
            abs = QDir::cleanPath(abs);
            if (!QFileInfo::exists(abs)) { r.warnings << "@dosya bulunamadı: " + pathPart; continue; }
            r.text += readRange(abs, from, to, r.files, qMin(budget, 6000));
            budget = qMax(0, budget - 3000);
        }
    }
    return r;
}
