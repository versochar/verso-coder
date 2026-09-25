#include "TestDiscovery.h"
#include "TestParser.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

QList<TestRunner> TestDiscovery::findRunners(const QString& root) {
    QList<TestRunner> out;
    QDir d(root);
    // 1) ctest: build dizinlerinde CTestTestfile.cmake ara
    const QStringList buildDirs = {root + "/build", root};
    for (const QString& bd : buildDirs) {
        QDir b(bd);
        if (b.exists("CTestTestfile.cmake")) {
            TestRunner r;
            r.kind = "ctest";
            r.path = bd;
            out << r;
            break;
        }
    }
    // 2) gtest: çalıştırılabilir + --gtest_list_tests desteği heuristic'i (adayları çağıran bulur)
    // 3) pytest
    if (!QStandardPaths::findExecutable("pytest").isEmpty()) {
        TestRunner r;
        r.kind = "pytest";
        r.path = root;
        out << r;
    } else {
        // unittest her zaman (python3 varsa)
        if (!QStandardPaths::findExecutable("python3").isEmpty()) {
            bool any = false;
            QDirIterator it(root, {"test*.py", "*_test.py"}, QDir::Files,
                            QDirIterator::Subdirectories);
            if (it.hasNext()) { it.next(); any = true; }
            if (any) {
                TestRunner r;
                r.kind = "unittest";
                r.path = root;
                out << r;
            }
        }
    }
    return out;
}

QStringList TestDiscovery::listCommand(const TestRunner& r) {
    if (r.kind == "ctest") return {"ctest", "-N"};
    if (r.kind == "gtest") return {r.path, "--gtest_list_tests"};
    if (r.kind == "pytest") return {"pytest", "--collect-only", "-q"};
    return {"python3", "-m", "unittest", "discover", "-s", ".", "-p", "test*.py", "-v"};
}

QStringList TestDiscovery::runCommand(const TestRunner& r, const QString& root,
                                      const QString& testId) {
    Q_UNUSED(root);
    if (r.kind == "ctest")
        return testId.isEmpty() ? QStringList({"ctest", "--output-on-failure"})
                                : QStringList({"ctest", "-R", testId, "--output-on-failure"});
    if (r.kind == "gtest")
        return testId.isEmpty() ? QStringList({r.path})
                                : QStringList({r.path, "--gtest_filter=" + testId});
    if (r.kind == "pytest")
        return testId.isEmpty() ? QStringList({"pytest", "-v"})
                                : QStringList({"pytest", "-v", testId});
    return testId.isEmpty() ? QStringList({"python3", "-m", "unittest", "discover", "-v"})
                            : QStringList({"python3", "-m", "unittest", "-v", testId});
}

QList<TestCase> TestDiscovery::parseList(const TestRunner& r, const QString& output) {
    if (r.kind == "ctest") return TestParser::parseCtestList(output);
    if (r.kind == "gtest") return TestParser::parseGtestList(output);
    if (r.kind == "pytest") return TestParser::parsePytestCollect(output);
    // unittest: statik tarama (çalıştırma yok)
    QList<TestCase> out;
    QDirIterator it(r.path, {"test*.py", "*_test.py"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) out << scanPythonFile(it.next());
    return out;
}

QList<TestCase> TestDiscovery::scanPythonFile(const QString& filePath) {
    QList<TestCase> out;
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return out;
    const QString text = QString::fromUtf8(f.readAll());
    static QRegularExpression clsRe(R"(^class\s+(\w+)\s*\([^)]*TestCase[^)]*\))");
    static QRegularExpression anyFnRe(R"(^\s*def\s+(test\w*)\s*\()");
    QString cur;
    bool inCase = false;
    for (const QString& raw : text.split('\n')) {
        // Girinti sıfırlanınca sınıf kapsamı biter
        if (!raw.isEmpty() && !raw[0].isSpace() && !raw.startsWith("class")
            && !raw.startsWith("#"))
            inCase = false;
        auto m = clsRe.match(raw);
        if (m.hasMatch()) {
            cur = QFileInfo(filePath).fileName() + "::" + m.captured(1);
            inCase = true;
            continue;
        }
        auto m2 = anyFnRe.match(raw);
        if (m2.hasMatch() && (inCase || raw.startsWith("def"))) {
            TestCase t;
            t.suite = inCase ? cur : QFileInfo(filePath).fileName();
            t.name = m2.captured(1);
            out << t;
        }
    }
    return out;
}
