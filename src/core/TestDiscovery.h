#pragma once
#include "TestParser.h"
#include <QString>
#include <QStringList>

// Stage 14: test keşfi — çalıştırıcıları bul + listeleme komutları.
struct TestRunner {
    QString kind; // "ctest" | "gtest" | "pytest" | "unittest"
    QString path; // binary / dizin
};

class TestDiscovery {
public:
    // Kök dizinde çalıştırıcıları bul (öncelik: ctest > gtest binary > pytest > unittest)
    static QList<TestRunner> findRunners(const QString& root);
    // Listeleme komutu (program + argümanlar)
    static QStringList listCommand(const TestRunner& r);
    // Çalıştırma komutu: tümü ya da tek test
    static QStringList runCommand(const TestRunner& r, const QString& root,
                                 const QString& testId = QString());
    // Listeleme çıktısını TestCase'lere çevir
    static QList<TestCase> parseList(const TestRunner& r, const QString& output);
    // python dosyalarından statik test keşfi (unittest.TestCase + test_* )
    static QList<TestCase> scanPythonFile(const QString& filePath);
};
