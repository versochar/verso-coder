// Stage 32 testleri: hız yardımcıları + artımlı RAG + ters indeks.
// Çıkış kodu 0 = hepsi geçti.
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <cstdio>

#include "../src/core/PerfTools.h"
#include "../src/core/RagIndexer.h"

static int g_pass = 0, g_fail = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (cond) {                                                            \
            ++g_pass;                                                          \
        } else {                                                               \
            ++g_fail;                                                          \
            fprintf(stderr, "FAIL %d %s\n", __LINE__, #cond);                  \
            fflush(stderr);                                                    \
        }                                                                      \
    } while (0)

static void testBinary() {
    CHECK(!PerfTools::isLikelyBinary("merhaba dünya\nikinci satır"));
    CHECK(PerfTools::isLikelyBinary(QByteArray("abc\0def", 7)));
    QByteArray ctrl;
    for (int i = 0; i < 100; ++i) ctrl.append(char(i % 8));
    CHECK(PerfTools::isLikelyBinary(ctrl));
    CHECK(!PerfTools::isLikelyBinary(QByteArray()));
}

static void testSkip() {
    QStringList exts = {"cpp", "h", "py"};
    CHECK(PerfTools::skipByExtAndSize(100, "cpp", exts, 1000) == false);
    CHECK(PerfTools::skipByExtAndSize(100, "CPP", exts, 1000) == false); // büyük/küçük
    CHECK(PerfTools::skipByExtAndSize(100, "exe", exts, 1000) == true);
    CHECK(PerfTools::skipByExtAndSize(5000, "cpp", exts, 1000) == true); // boyut
    CHECK(PerfTools::skipByExtAndSize(5000, "exe", QStringList(), 1000) == true);
}

static void testLargeMode() {
    CHECK(PerfTools::largeFileMode(100, 2, 8) == 0);
    CHECK(PerfTools::largeFileMode(3 * 1024 * 1024, 2, 8) == 1);
    CHECK(PerfTools::largeFileMode(9 * 1024 * 1024, 2, 8) == 2);
    CHECK(PerfTools::largeFileMode(100, 0, 0) == 0); // varsayılanlar
}

static void testLowerCache() {
    PerfTools::LowerCache c;
    CHECK(c.lower("Merhaba") == "merhaba");
    CHECK(c.lower("Merhaba") == "merhaba");
    CHECK(c.size() == 1);
    for (int i = 0; i < 600; ++i) c.lower(QString("k%1").arg(i));
    CHECK(c.size() <= 512); // LRU tavan
}

static void testInverted() {
    PerfTools::InvertedIndex idx;
    idx.add(0, "int main fonksiyonu cpp dosyasi");
    idx.add(1, "python def fonksiyon");
    idx.add(2, "hello world");
    CHECK(idx.docCount() == 3);
    const QStringList kws = PerfTools::keywords("fonksiyon main");
    CHECK(kws.size() == 2);
    auto sc = idx.scoreDocs(kws);
    CHECK(sc.value(0) == 2); // ikisi de var (alt-dize: fonksiyon → fonksiyonu)
    CHECK(sc.value(1) == 1);
    CHECK(!sc.contains(2));
    auto none = idx.scoreDocs({"yokboyle"});
    CHECK(none.isEmpty());
}

static void testRagIncremental() {
    QTemporaryDir dir;
    CHECK(dir.isValid());
    QFile a(dir.filePath("a.cpp"));
    CHECK(a.open(QIODevice::WriteOnly));
    a.write("int toplama(int a, int b) {\n    return a + b;\n}\n"
            "// uzun aciklama satiri ile chunk olussun diye dolgu\n"
            "int cikarma(int a, int b) {\n    return a - b;\n}\n");
    a.close();
    QFile b(dir.filePath("b.py"));
    CHECK(b.open(QIODevice::WriteOnly));
    b.write("def merhaba():\n    print('selam dunya')\n"
            "    # dolgu satiri\n    return 42\n");
    b.close();

    RagIndexer rag;
    CHECK(rag.indexProjectIncremental(dir.path()) > 0);
    CHECK(rag.fileCount() == 2);
    CHECK(rag.cacheCount() == 2);
    const int first = rag.chunkCount();

    // Aynı içerikle ikinci kez: önbellekten, aynı sonuç
    CHECK(rag.indexProjectIncremental(dir.path()) == first);
    CHECK(rag.cacheCount() == 2);

    // Sorgu: ters indeksten doğru dosya
    QList<RagChunk> hits = rag.query("cikarma fonksiyonu");
    CHECK(!hits.isEmpty());
    CHECK(hits.first().file.endsWith("a.cpp"));

    // Dosyayı değiştir: artımlı yeniden indeks günceller
    QFile a2(dir.filePath("a.cpp"));
    CHECK(a2.open(QIODevice::WriteOnly | QIODevice::Truncate));
    a2.write("int carpma(int a, int b) {\n    return a * b; // yeni\n}\n"
             "// dolgu satiri yeterince uzun olsun ki chunk olussun\n");
    a2.close();
    rag.indexProjectIncremental(dir.path());
    QList<RagChunk> h2 = rag.query("carpma");
    CHECK(!h2.isEmpty());
    CHECK(h2.first().file.endsWith("a.cpp"));

    // Tam yeniden indeks önbelleği temizler
    rag.indexProject(dir.path());
    CHECK(rag.cacheCount() == 2);
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    testBinary();
    testSkip();
    testLargeMode();
    testLowerCache();
    testInverted();
    testRagIncremental();
    fprintf(stderr, "STAGE32: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
