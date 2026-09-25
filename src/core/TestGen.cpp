#include "TestGen.h"
#include <QDir>
#include <QFileInfo>

QString TestGen::frameworkFor(const QString& suffix) {
    const QString s = suffix.toLower();
    if (s == "py") return "pytest";
    if (s == "cpp" || s == "h" || s == "hpp" || s == "c" || s == "cc") return "gtest";
    return "unittest";
}

QString TestGen::targetFor(const QString& sourcePath) {
    const QFileInfo fi(sourcePath);
    const QString base = fi.completeBaseName();
    const QString suf = fi.suffix().toLower();
    const QString dir = fi.absolutePath();
    if (suf == "py") return QDir(dir).absoluteFilePath("test_" + base + ".py");
    // C++: tests/ altına test_<ad>.cpp
    return QDir(dir).absoluteFilePath("tests/test_" + base + ".cpp");
}

QString TestGen::buildPrompt(const QString& code, const QString& lang,
                             const QString& targetName) {
    return QString(
               "Şu %1 kodu için %3 çerçevesinde birim testleri yaz (%2 dosyasına). "
               "SADECE test kodunu ``` bloğunda döndür:\n```%1\n%4\n```")
        .arg(lang, targetName, frameworkFor(lang == "C++" ? "cpp" : lang),
             code.left(6000));
}
