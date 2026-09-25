#pragma once
#include <QList>
#include <QString>

// Stage 14: test sonucu modeli — ctest / gtest / pytest+unittest çıktıları.
struct TestCase {
    QString suite;   // ctest: ""; gtest: Suite; pytest: dosya
    QString name;
    QString status;  // "pass" | "fail" | "skip"
    double timeMs = 0;
    QString detail;  // hata çıktısı özeti
    QString id() const { return suite.isEmpty() ? name : suite + "." + name; }
};

class TestParser {
public:
    static QList<TestCase> parseCtest(const QString& output);
    static QList<TestCase> parseGtest(const QString& output);
    static QList<TestCase> parsePytest(const QString& output); // -v biçimi + unittest -v

    // Keşif çıktıları
    static QList<TestCase> parseCtestList(const QString& output);  // ctest -N
    static QList<TestCase> parseGtestList(const QString& output);  // --gtest_list_tests
    static QList<TestCase> parsePytestCollect(const QString& output); // --collect-only -q
};
