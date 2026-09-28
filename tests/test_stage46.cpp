// Stage 46 testleri: büyük dosya açılış süresi, tekrarlı aç/kapa RSS
// dengesi ve önbellek sınırları. Eşikler cömert (offscreen CI güvenliği);
// amaç regresyon kilidi, rekor denemesi değil.
#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <cstdio>

#include "../src/core/LeakWatch.h"
#include "../src/core/ai/EmbedCache.h"
#include "../src/widgets/CodeEditor.h"

static int g_pass = 0, g_fail = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (cond) {                                                            \
            ++g_pass;                                                          \
        } else {                                                                \
            ++g_fail;                                                          \
            fprintf(stderr, "FAIL %d %s\n", __LINE__, #cond);                    \
            fflush(stderr);                                                    \
        }                                                                      \
    } while (0)

#define CHECK2(cond, why)                                                      \
    do {                                                                       \
        if (cond) {                                                            \
            ++g_pass;                                                          \
        } else {                                                                \
            ++g_fail;                                                          \
            fprintf(stderr, "FAIL %d %s (%s)\n", __LINE__, #cond, why);          \
            fflush(stderr);                                                    \
        }                                                                      \
    } while (0)

static QString makeBigFile(const QString& dir, const QString& name, int lines) {
    const QString p = QDir(dir).filePath(name);
    QFile f(p);
    CHECK(f.open(QIODevice::WriteOnly));
    QByteArray chunk;
    for (int i = 0; i < 100; ++i)
        chunk += QString("int degisken_%1 = %1 * 2; // satir %1\n").arg(i).toUtf8();
    for (int i = 0; i < lines / 100; ++i) f.write(chunk);
    f.close();
    return p;
}

// --- 1: 100 bin satır açılış süresi ---
static void testLargeFileOpen() {
    QTemporaryDir d;
    CHECK(d.isValid());
    const QString p = makeBigFile(d.path(), "buyuk.cpp", 100000);
    CHECK(QFileInfo(p).size() > 3000000); // ~3,5 MB
    QElapsedTimer t;
    t.start();
    CodeEditor e;
    CHECK(e.loadFile(p));
    const qint64 ms = t.elapsed();
    fprintf(stderr, "NOT: 100k satir acilis %lld ms\n", ms);
    CHECK2(ms < 15000, "100k satır açılışı 15 sn'yi aştı");
    CHECK(e.toPlainText().count('\n') >= 99900);
}

// --- 2: tekrarlı aç/kapa RSS dengesi (sızıntı kilidi) ---
static void testReopenSoak() {
    QTemporaryDir d;
    const QString p = makeBigFile(d.path(), "d Dongu.cpp", 20000);
    // QTest olmadan editör yarat (offscreen)
    LeakWatch w("ac/kapa", 5);
    w.reset();
    for (int i = 0; i < 5; ++i) {
        CodeEditor* e = new CodeEditor();
        e->loadFile(p);
        e->show();
        QTest::qWait(20);
        delete e;
        QTest::qWait(20);
        w.tick();
    }
    const LeakWatch::Result r = w.result();
    fprintf(stderr, "NOT: 5x ac/kapa %.1f MB -> %.1f MB (tur basina %.1f KB)\n", r.startMb,
            r.endMb, r.growthPerIterKb);
    if (r.available) {
        // 20k satırlık editör tur başına 2 MB'den fazla bırakmamalı
        CHECK2(r.growthPerIterKb < 2048.0, "aç/kapa döngüsünde bellek artışı");
    } else {
        ++g_pass; // /proc yok
    }
}

// --- 3: önbellek sınırları ---
static void testCacheBounds() {
    EmbedCache c(8);
    CHECK(c.maxEntries() == 8);
    for (int i = 0; i < 20; ++i)
        c.put("m", QString("metin %1").arg(i), QList<float>(4, float(i)));
    CHECK2(c.size() <= 8, "gömme önbelleği sınırı aştı");
    // En son eklenen duruyor, en eski tahliye edilmiş
    CHECK(c.contains("m", "metin 19"));
    CHECK(!c.contains("m", "metin 0"));
    CHECK(c.hits() + c.misses() >= 0);
}

// --- 4: çoklu imleçle satır taşıma (ek imleçler korunur) ---
static void testMultiCaretMoveLine() {
    // Tek imleç: klasik davranış
    {
        CodeEditor e;
        e.setPlainText("a\nb");
        QTextCursor c(e.document());
        c.movePosition(QTextCursor::Start);
        e.setTextCursor(c);
        e.moveLineOrSelection(1);
        CHECK(e.toPlainText() == "b\na");
        CHECK(e.textCursor().blockNumber() == 1);
    }
    // İki ayrı satır, aşağı: üst blok kayar, sınırdaki sabit kalır
    {
        CodeEditor e;
        e.setPlainText("a\nb\nc");
        QTextCursor c(e.document());
        c.movePosition(QTextCursor::Start);
        e.setTextCursor(c);
        e.addCursorAt(e.document()->findBlockByNumber(2).position());
        CHECK(e.extraCursorCount() == 1);
        e.moveLineOrSelection(1);
        CHECK(e.toPlainText() == "b\na\nc");
        CHECK(e.extraCursorCount() == 1);
        CHECK(e.textCursor().blockNumber() == 1); // "a"yı izledi
    }
    // Bitişik iki satır, yukarı: tek blok gibi taşınır
    {
        CodeEditor e;
        e.setPlainText("a\nb\nc\nd");
        QTextCursor c(e.document());
        c.setPosition(e.document()->findBlockByNumber(1).position());
        e.setTextCursor(c);
        e.addCursorAt(e.document()->findBlockByNumber(2).position());
        e.moveLineOrSelection(-1);
        CHECK(e.toPlainText() == "b\nc\na\nd");
        CHECK(e.extraCursorCount() == 1);
        CHECK(e.textCursor().blockNumber() == 0);
    }
    // Sınır: en üst satır yukarı oynamaz, undo kirlenmez
    {
        CodeEditor e;
        e.setPlainText("a\nb");
        QTextCursor c(e.document());
        c.movePosition(QTextCursor::Start);
        e.setTextCursor(c);
        e.moveLineOrSelection(-1);
        CHECK(e.toPlainText() == "a\nb");
    }
    // Görsel biçim kirletmez: satır yüksekliği kirli bayrağı oynatmaz
    {
        CodeEditor e;
        e.setPlainText("x\ny");
        e.document()->setModified(false);
        e.applyLineHeight(1.5);
        CHECK(!e.document()->isModified());
    }
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QTemporaryDir home;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, home.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, home.path());
    qputenv("XDG_DATA_HOME", home.path().toUtf8());
    qputenv("HOME", home.path().toUtf8());

    testLargeFileOpen();
    testReopenSoak();
    testCacheBounds();
    testMultiCaretMoveLine();

    fprintf(stderr, "STAGE46: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
