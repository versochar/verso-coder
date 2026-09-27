// Stage 50 meta-test: kayıtlı test paketleri dosya sistemiyle tutarlı mı?
// (Kayıtlı ama derlenmeyen / dosyası silinmiş test sessizce kaybolmasın.)
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <cstdio>

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

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    // Kaynak kökü yukarı doğru aranır (build/, makepkg src/ düzeni dahil)
    QString src;
    QDir d = QDir::current();
    for (int i = 0; i < 5 && src.isEmpty(); ++i) {
        const QString cand = d.filePath("CMakeLists.txt");
        if (QFile::exists(cand)) {
            QFile probe(cand);
            if (probe.open(QIODevice::ReadOnly)) {
                const QString head = QString::fromUtf8(probe.read(4096));
                if (head.contains("verso-coder")) src = cand;
            }
        }
        d.cdUp();
    }
    QFile f(src);
    CHECK2(f.open(QIODevice::ReadOnly), "CMakeLists.txt bulunamadı");
    if (!f.isOpen()) {
        fprintf(stderr, "STAGE50: %d passed, %d failed\n", g_pass, g_fail);
        return 1;
    }
    const QString cmake = QString::fromUtf8(f.readAll());
    const QString root = QFileInfo(src).absolutePath();

    // add_test(NAME x COMMAND y) çiftleri
    QRegularExpression re("add_test\\(NAME\\s+(\\S+)\\s+COMMAND\\s+(\\S+)\\)");
    QMap<QString, QString> tests;
    auto it = re.globalMatch(cmake);
    while (it.hasNext()) {
        const auto m = it.next();
        tests[m.captured(1)] = m.captured(2);
    }
    CHECK2(!tests.isEmpty(), "hiç test kaydı yok");
    fprintf(stderr, "NOT: %d test kaydı\n", tests.size());

    // Her COMMAND için add_executable + kaynak dosyası var mı?
    int checked = 0;
    for (auto i = tests.constBegin(); i != tests.constEnd(); ++i) {
        const QString bin = i.value();
        // ui_* ikilileri tests/ui altında
        const bool isUi = bin.startsWith("ui_");
        QRegularExpression reExe(QString("add_executable\\(%1\\s+([^)]+)\\)")
                                     .arg(QRegularExpression::escape(bin)));
        const auto m = reExe.match(cmake);
        CHECK2(m.hasMatch(), qPrintable(QString("add_executable yok: %1").arg(bin)));
        if (!m.hasMatch()) continue;
        const QStringList srcs = m.captured(1).split(QRegularExpression("\\s+"),
                                                     Qt::SkipEmptyParts);
        CHECK2(!srcs.isEmpty(), "kaynak listesi boş");
        for (const QString& s : srcs) {
            if (s.trimmed().startsWith("${")) continue; // cmake değişkeni
            const QString full = QDir(root).filePath(s.trimmed());
            if (!QFile::exists(full))
                fprintf(stderr, "NOT: kayıp kaynak: %s\n", qPrintable(s));
            CHECK2(QFile::exists(full), qPrintable(QString("kayıp: %1").arg(s)));
            ++checked;
        }
    }
    fprintf(stderr, "NOT: %d kaynak denetlendi\n", checked);

    // Hedef başına tekrarlanan kaynak yok (ODR/link kırılması üretir)
    {
        QRegularExpression reExe("add_executable\\((\\w+)[\\s\\S]*?\\\)");
        auto it2 = reExe.globalMatch(cmake);
        while (it2.hasNext()) {
            const QString body = it2.next().captured(0);
            QRegularExpression reSrc("^\\s*(\\S+\.cpp)\\s*\$",
                                     QRegularExpression::MultilineOption);
            QSet<QString> seen;
            auto it3 = reSrc.globalMatch(body);
            while (it3.hasNext()) {
                const QString src = it3.next().captured(1);
                if (src.startsWith("\$")) continue;
                CHECK2(!seen.contains(src),
                       qPrintable(QString("tekrar: %1").arg(src)));
                seen << src;
            }
        }
    }

    // Beklenen paketler: stage19..50 + ui (tam yol haritası)
    QStringList expected = {"ui_widgets", "ui_smoke", "stage50"};
    for (int n = 19; n <= 49; ++n) expected << QString("stage%1").arg(n);
    for (const QString& e : expected)
        CHECK2(tests.contains(e), qPrintable(QString("kayıt yok: %1").arg(e)));

    fprintf(stderr, "STAGE50: %d passed, %d failed\n", g_pass, g_fail);
    fflush(stderr);
    return g_fail == 0 ? 0 : 1;
}
