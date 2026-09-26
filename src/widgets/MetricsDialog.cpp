#include "MetricsDialog.h"
#include "../core/CodeMetrics.h"
#include "../core/DuplicateFinder.h"
#include "../src/core/WorkspaceSymbols.h"
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QRegularExpression>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {
const QStringList kSrcExts = {"c",  "h",  "cpp", "hpp", "cc", "cxx", "py", "js",
                              "jsx", "ts", "tsx", "java", "go",   "rs",  "rb",
                              "php", "cs", "kt",  "swift"};
}

QStringList MetricsDialog::collectSources(const QString& root) {
    QStringList out;
    if (root.isEmpty()) return out;
    QDirIterator it(root, QDir::Files | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext() && out.size() < 1500) {
        const QString p = it.next();
        if (p.contains("/.git/") || p.contains("/build/") ||
            p.contains("/node_modules/") || p.contains("/.venv/"))
            continue;
        if (kSrcExts.contains(QFileInfo(p).suffix().toLower())) out << p;
    }
    return out;
}

QString MetricsDialog::readCapped(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    const QByteArray raw = f.read(512 * 1024);
    if (raw.contains('\0')) return {};
    return QString::fromUtf8(raw);
}

MetricsDialog::MetricsDialog(const QString& root, const QString& currentFile,
                             QWidget* parent)
    : QDialog(parent), m_root(root), m_current(currentFile) {
    setWindowTitle("Proje Radarı");
    resize(680, 480);
    auto* lay = new QVBoxLayout(this);
    auto* tabs = new QTabWidget(this);
    lay->addWidget(tabs, 1);

    const QStringList files = collectSources(root);
    QMap<QString, QString> texts;
    for (const QString& f : files) {
        const QString t = readCapped(f);
        if (!t.isEmpty()) texts[f] = t;
    }
    const QList<WorkspaceSymbol> syms = WorkspaceSymbols::scanFiles(texts);

    // --- Sekme 1: metrikler (etkin dosya) ---
    {
        auto* w = new QWidget(this);
        auto* l = new QVBoxLayout(w);
        auto* tree = new QTreeWidget(w);
        tree->setHeaderLabels({"Ölçü", "Değer"});
        tree->setRootIsDecorated(false);
        const QString cur = texts.value(currentFile, readCapped(currentFile));
        const int fnCount = WorkspaceSymbols::scanFile(currentFile, cur).size();
        const CodeMetrics m = CodeMetrics::analyze(currentFile, cur, fnCount);
        const QStringList rows = {
            QString("Dosya\t%1").arg(QFileInfo(currentFile).fileName()),
            QString("Satır\t%1").arg(m.lines),
            QString("Kod satırı\t%1").arg(m.codeLines),
            QString("Fonksiyon\t%1").arg(m.functions),
            QString("Dallanma\t%1").arg(m.branches),
            QString("Karmaşıklık (1+dal)\t%1").arg(m.complexity()),
        };
        for (const QString& r : rows)
            new QTreeWidgetItem(tree, r.split('\t'));
        l->addWidget(tree);
        tabs->addTab(w, QString("Metrikler (%1 dosya)").arg(files.size()));
    }

    // --- Sekme 2: yinelenen bloklar ---
    {
        auto* w = new QWidget(this);
        auto* l = new QVBoxLayout(w);
        auto* tree = new QTreeWidget(w);
        tree->setHeaderLabels({"Kopya grubu"});
        int groups = 0;
        for (const DupGroup& g : DuplicateFinder::find(texts)) {
            if (groups >= 60) break;
            auto* top = new QTreeWidgetItem(
                tree, {QString("%1 satır × %2 konum").arg(g.lines).arg(g.files.size())});
            for (const QString& loc : g.files) {
                auto* c = new QTreeWidgetItem(top, {loc});
                c->setData(0, Qt::UserRole, loc.section(':', 0, -2));
                c->setData(0, Qt::UserRole + 1,
                           loc.section(':', -1).toInt());
            }
            ++groups;
        }
        if (groups == 0)
            new QTreeWidgetItem(tree, {"Kopya blok yok (≥6 satır)"});
        connect(tree, &QTreeWidget::itemDoubleClicked, this,
                [this](QTreeWidgetItem* it, int) {
                    const QString f = it->data(0, Qt::UserRole).toString();
                    if (!f.isEmpty()) emit fileJumpRequested(f, it->data(0, Qt::UserRole + 1).toInt());
                });
        l->addWidget(tree);
        tabs->addTab(w, QString("Kopyalar (%1)").arg(groups));
    }

    // --- Sekme 3: kullanılmayan fonksiyonlar (etkin dosya) ---
    {
        auto* w = new QWidget(this);
        auto* l = new QVBoxLayout(w);
        auto* tree = new QTreeWidget(w);
        tree->setHeaderLabels({"Kullanılmayan (0 gönderme)"});
        const QString cur = texts.value(currentFile, readCapped(currentFile));
        int n = 0;
        for (const WorkspaceSymbol& s : WorkspaceSymbols::scanFile(currentFile, cur)) {
            if (s.kind != "func" || s.name == "main") continue;
            int refs = 0;
            for (auto it = texts.constBegin(); it != texts.constEnd(); ++it)
                refs += WorkspaceSymbols::refCount(it.value(), s.name,
                                                   it.key() == s.file ? s.line : -1);
            if (refs == 0 && n < 100) {
                new QTreeWidgetItem(tree, {QString("%1  (satır %2)").arg(s.name).arg(s.line)});
                ++n;
            }
        }
        if (n == 0) new QTreeWidgetItem(tree, {"Hepsi kullanılıyor ✓"});
        l->addWidget(tree);
        tabs->addTab(w, QString("Kullanılmayan (%1)").arg(n));
    }

    // --- Sekme 4: bağımlılıklar (include/import) ---
    {
        auto* w = new QWidget(this);
        auto* l = new QVBoxLayout(w);
        auto* tree = new QTreeWidget(w);
        tree->setHeaderLabels({"Dosya → bağımlılık"});
        static QRegularExpression incRe(
            R"(^\s*#\s*include\s+[<"]([^>"]+)[>"]|^\s*(?:import|from)\s+([\w.]+))");
        int n = 0;
        for (auto it = texts.constBegin(); it != texts.constEnd() && n < 300; ++it) {
            QStringList deps;
            for (const QString& ln : it.value().split('\n')) {
                auto m = incRe.match(ln.trimmed());
                if (m.hasMatch() && !m.captured(1).isEmpty()) deps << m.captured(1);
                else if (m.hasMatch() && !m.captured(2).isEmpty())
                    deps << m.captured(2).split('.').first();
            }
            deps.removeDuplicates();
            if (deps.isEmpty()) continue;
            auto* top = new QTreeWidgetItem(tree, {QFileInfo(it.key()).fileName()});
            for (const QString& d : deps) new QTreeWidgetItem(top, {d});
            ++n;
        }
        if (n == 0) new QTreeWidgetItem(tree, {"Bağımlılık bulunamadı"});
        l->addWidget(tree);
        tabs->addTab(w, QString("Bağımlılıklar (%1)").arg(n));
    }
}
