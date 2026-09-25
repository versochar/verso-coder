#pragma once
#include <QString>

// Stage 15: test üretimi — hedef dosya çözümleyici + istem.
class TestGen {
public:
    // foo.cpp → tests/test_foo.cpp ; foo.py → test_foo.py ; başkasına tests/ öneki
    static QString targetFor(const QString& sourcePath);
    static QString buildPrompt(const QString& code, const QString& lang,
                               const QString& targetName);
    static QString frameworkFor(const QString& suffix); // "gtest" | "pytest" | "unittest"
};
