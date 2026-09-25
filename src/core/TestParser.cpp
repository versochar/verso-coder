#include "TestParser.h"
#include <QRegularExpression>

QList<TestCase> TestParser::parseCtest(const QString& output) {
    QList<TestCase> out;
    // "Test   #1: mytest .........   Passed    0.01 sec" / "***Failed", "***Timeout", "***Skipped"
    static QRegularExpression re(
        R"(Test\s+#\d+:\s+(\S+).*?(\*\*\*)?(Passed|Failed|Timeout|Skipped|Not Run))");
    for (const QString& ln : output.split('\n')) {
        auto m = re.match(ln);
        if (!m.hasMatch()) continue;
        TestCase t;
        t.name = m.captured(1);
        const QString st = m.captured(3);
        t.status = (st == "Passed") ? "pass" : (st == "Skipped" || st == "Not Run") ? "skip" : "fail";
        static QRegularExpression tm(R"(([\d.]+)\s*sec)");
        auto mm = tm.match(ln);
        if (mm.hasMatch()) t.timeMs = mm.captured(1).toDouble() * 1000.0;
        out << t;
    }
    return out;
}

QList<TestCase> TestParser::parseGtest(const QString& output) {
    QList<TestCase> out;
    // "[ RUN      ] Suite.Name" ... "[       OK ] Suite.Name (1 ms)" / "[  FAILED  ]"
    static QRegularExpression run(R"(\[\s*RUN\s*\]\s+(\S+))");
    static QRegularExpression ok(R"(\[\s*OK\s*\]\s+(\S+)(?:\s+\((\d+)\s*ms\))?)");
    static QRegularExpression fail(R"(\[\s*FAILED\s*\]\s+(\S+))");
    static QRegularExpression skip(R"(\[\s*SKIPPED\s*\]\s+(\S+))");
    QMap<QString, TestCase> pending;
    for (const QString& ln : output.split('\n')) {
        auto m = run.match(ln);
        if (m.hasMatch()) {
            TestCase t;
            const QString full = m.captured(1);
            int dot = full.indexOf('.');
            t.suite = full.left(dot);
            t.name = full.mid(dot + 1);
            pending[full] = t;
            continue;
        }
        m = ok.match(ln);
        if (m.hasMatch()) {
            const QString full = m.captured(1);
            TestCase t = pending.value(full);
            if (t.name.isEmpty()) {
                int dot = full.indexOf('.');
                t.suite = full.left(dot);
                t.name = full.mid(dot + 1);
            }
            t.status = "pass";
            t.timeMs = m.captured(2).toDouble();
            out << t;
            pending.remove(full);
            continue;
        }
        m = fail.match(ln);
        if (m.hasMatch() && !ln.contains("listed below") && !ln.contains("tests,")) {
            const QString full = m.captured(1);
            TestCase t = pending.value(full);
            if (t.name.isEmpty()) {
                int dot = full.indexOf('.');
                t.suite = full.left(dot);
                t.name = full.mid(dot + 1);
            }
            t.status = "fail";
            out << t;
            pending.remove(full);
            continue;
        }
        m = skip.match(ln);
        if (m.hasMatch()) {
            const QString full = m.captured(1);
            TestCase t = pending.value(full);
            t.status = "skip";
            if (t.name.isEmpty()) {
                int dot = full.indexOf('.');
                t.suite = full.left(dot);
                t.name = full.mid(dot + 1);
            }
            out << t;
            pending.remove(full);
        }
    }
    // Sonucu gelmeyen RUN'lar (crash) → fail
    for (auto it = pending.begin(); it != pending.end(); ++it) {
        TestCase t = it.value();
        t.status = "fail";
        t.detail = "sonuç alınamadı (çökme?)";
        out << t;
    }
    return out;
}

QList<TestCase> TestParser::parsePytest(const QString& output) {
    QList<TestCase> out;
    // pytest -v: "test_x.py::TestCls::test_a PASSED [ 33%]" / FAILED / SKIPPED / XFAIL
    static QRegularExpression re(
        R"((\S+\.py)::(\S+)\s+(PASSED|FAILED|SKIPPED|XFAIL|XPASS|ERROR))");
    // unittest -v: "test_a (mod.TestCls) ... ok" / FAIL / ERROR / skipped
    static QRegularExpression ure(
        R"(^(\S+)\s+\((\S+)\)\s+\.\.\.\s+(ok|FAIL|ERROR|skipped.*|expected failure.*))");
    for (const QString& ln : output.split('\n')) {
        auto m = re.match(ln);
        if (m.hasMatch()) {
            TestCase t;
            t.suite = m.captured(1);
            t.name = m.captured(2);
            const QString st = m.captured(3);
            t.status = (st == "PASSED" || st == "XPASS") ? "pass"
                     : (st == "SKIPPED" || st == "XFAIL") ? "skip" : "fail";
            out << t;
            continue;
        }
        m = ure.match(ln.trimmed());
        if (m.hasMatch()) {
            TestCase t;
            t.name = m.captured(1);
            t.suite = m.captured(2);
            const QString st = m.captured(3);
            t.status = st.startsWith("ok") ? "pass"
                     : (st.startsWith("skip") || st.startsWith("expected")) ? "skip" : "fail";
            out << t;
        }
    }
    return out;
}

QList<TestCase> TestParser::parseCtestList(const QString& output) {
    QList<TestCase> out;
    static QRegularExpression re(R"(Test\s+#\d+:\s+(\S+))");
    for (const QString& ln : output.split('\n')) {
        auto m = re.match(ln);
        if (m.hasMatch()) {
            TestCase t;
            t.name = m.captured(1);
            out << t;
        }
    }
    return out;
}

QList<TestCase> TestParser::parseGtestList(const QString& output) {
    QList<TestCase> out;
    QString suite;
    for (const QString& ln : output.split('\n')) {
        const QString t = ln.trimmed();
        if (t.isEmpty()) continue;
        if (!ln.startsWith(" ") && t.endsWith('.')) { suite = t.chopped(1); continue; }
        if (t.endsWith('.')) { suite = t.chopped(1); continue; }
        TestCase c;
        c.suite = suite;
        c.name = t;
        out << c;
    }
    return out;
}

QList<TestCase> TestParser::parsePytestCollect(const QString& output) {
    QList<TestCase> out;
    static QRegularExpression re(R"((\S+\.py)::(\S+))");
    for (const QString& ln : output.split('\n')) {
        auto m = re.match(ln.trimmed());
        if (!m.hasMatch()) continue;
        TestCase t;
        t.suite = m.captured(1);
        t.name = m.captured(2);
        out << t;
    }
    return out;
}
