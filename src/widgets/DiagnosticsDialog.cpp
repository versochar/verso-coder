#include "../core/CrashHandler.h"
#include "DiagnosticsDialog.h"
#include "../core/A11yCheck.h"
#include "../core/BackupManager.h"
#include "../core/GitVersionManager.h"
#include "../core/LanguageManager.h"
#include "../core/PerfMonitor.h"
#include "../core/PluginEngine.h"
#include "../core/ThemeManager.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QtConcurrent>

DiagnosticsDialog::DiagnosticsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Sistem Teşhisi");
    resize(780, 560);

    auto* lay = new QVBoxLayout(this);
    m_tabs = new QTabWidget(this);
    lay->addWidget(m_tabs, 1);

    // --- Hakkında ---
    auto* aboutTab = new QWidget(this);
    auto* aboutLay = new QVBoxLayout(aboutTab);
    m_about = new QTextBrowser(aboutTab);
    m_about->setHtml(AboutInfo::aboutHtml());
    aboutLay->addWidget(m_about, 1);
    // Stage 22: git sürümü (repo içindeyse; örn. geliştirme derlemesi)
    {
        GitVersionManager g;
        g.setRepoRoot(QCoreApplication::applicationDirPath());
        const QString v = g.currentVersion();
        if (!v.isEmpty())
            m_about->append(QString("<p><b>Git:</b> %1</p>").arg(v.toHtmlEscaped()));
    }
    aboutLay->addWidget(new QLabel("Güncelleme manifesti (JSON) yapıştır:", aboutTab));
    m_updateIn = new QTextEdit(aboutTab);
    m_updateIn->setMaximumHeight(70);
    m_updateIn->setPlaceholderText("{\"version\":\"1.1.0\",\"url\":\"https://...\",\"notes\":\"...\"}");
    auto* upBtn = new QPushButton("Güncellemeyi denetle", aboutTab);
    aboutLay->addWidget(m_updateIn);
    aboutLay->addWidget(upBtn);
    connect(upBtn, &QPushButton::clicked, this, &DiagnosticsDialog::checkUpdate);
    m_tabs->addTab(aboutTab, "Hakkında");

    // --- Stage 48: çökme dökümleri (onamlı: görüntüle/temizle) ---
    auto* crashTab = new QWidget(this);
    auto* crashLay = new QVBoxLayout(crashTab);
    m_crash = new QTextEdit(crashTab);
    m_crash->setReadOnly(true);
    m_crash->setFont(QFont("Consolas, monospace", 10));
    auto* crashRow = new QHBoxLayout();
    auto* cRef = new QPushButton("Yenile", crashTab);
    auto* cClear = new QPushButton("Temizle", crashTab);
    crashRow->addWidget(cRef);
    crashRow->addWidget(cClear);
    crashRow->addStretch(1);
    crashLay->addWidget(m_crash, 1);
    crashLay->addLayout(crashRow);
    connect(cRef, &QPushButton::clicked, this, &DiagnosticsDialog::refreshCrashes);
    connect(cClear, &QPushButton::clicked, this, [this]() {
        CrashHandler::clearDumps();
        refreshCrashes();
    });
    m_tabs->addTab(crashTab, "Çökmeler");
    refreshCrashes();

    // --- Araç zinciri ---
    auto* toolsTab = new QWidget(this);
    auto* toolsLay = new QVBoxLayout(toolsTab);
    m_tools = new QTextEdit(toolsTab);
    m_tools->setReadOnly(true);
    m_tools->setFont(QFont("Consolas, monospace", 10));
    m_tools->setPlainText("Araçlar taranıyor...");
    auto* tBtn = new QPushButton("Yeniden tara", toolsTab);
    toolsLay->addWidget(m_tools, 1);
    toolsLay->addWidget(tBtn);
    connect(tBtn, &QPushButton::clicked, this, &DiagnosticsDialog::refreshToolchain);
    m_tabs->addTab(toolsTab, "Araç Zinciri");

    // --- Performans ---
    auto* perfTab = new QWidget(this);
    auto* perfLay = new QVBoxLayout(perfTab);
    m_perf = new QTextEdit(perfTab);
    m_perf->setReadOnly(true);
    m_perf->setFont(QFont("Consolas, monospace", 10));
    auto* pBtn = new QPushButton("Yenile", perfTab);
    perfLay->addWidget(m_perf, 1);
    perfLay->addWidget(pBtn);
    connect(pBtn, &QPushButton::clicked, this, &DiagnosticsDialog::refreshPerformance);
    m_tabs->addTab(perfTab, "Performans");

    // --- Yedekler ---
    auto* bakTab = new QWidget(this);
    auto* bakLay = new QVBoxLayout(bakTab);
    m_backups = new QTreeWidget(bakTab);
    m_backups->setHeaderLabels({"Dosya", "Tarih", "Boyut"});
    m_backups->setRootIsDecorated(false);
    m_backups->header()->setStretchLastSection(false);
    m_backups->setColumnWidth(0, 360);
    m_backups->setColumnWidth(1, 160);
    auto* bakRow = new QHBoxLayout();
    auto* rBtn = new QPushButton("Geri yükle", bakTab);
    auto* dBtn = new QPushButton("Sil", bakTab);
    auto* refBtn = new QPushButton("Yenile", bakTab);
    bakRow->addWidget(rBtn);
    bakRow->addWidget(dBtn);
    bakRow->addWidget(refBtn);
    bakRow->addStretch(1);
    bakLay->addWidget(m_backups, 1);
    bakLay->addLayout(bakRow);
    connect(rBtn, &QPushButton::clicked, this, &DiagnosticsDialog::restoreBackup);
    connect(dBtn, &QPushButton::clicked, this, &DiagnosticsDialog::deleteBackup);
    connect(refBtn, &QPushButton::clicked, this, &DiagnosticsDialog::refreshBackups);
    m_tabs->addTab(bakTab, "Yedekler");

    // --- Stage 22: erişilebilirlik + çeviri kapsama ---
    auto* a11yTab = new QWidget(this);
    auto* a11yLay = new QVBoxLayout(a11yTab);
    m_a11y = new QTextEdit(a11yTab);
    m_a11y->setReadOnly(true);
    m_a11y->setFont(QFont("Consolas, monospace", 10));
    auto* aBtn = new QPushButton("Denetle", a11yTab);
    a11yLay->addWidget(m_a11y, 1);
    a11yLay->addWidget(aBtn);
    connect(aBtn, &QPushButton::clicked, this, &DiagnosticsDialog::refreshA11y);
    m_tabs->addTab(a11yTab, "Erişilebilirlik");

    // --- Stage 29: eklentiler ---
    auto* plugTab = new QWidget(this);
    auto* plugLay = new QVBoxLayout(plugTab);
    m_plugins = new QTextEdit(plugTab);
    m_plugins->setReadOnly(true);
    m_plugins->setFont(QFont("Consolas, monospace", 10));
    auto* plBtn = new QPushButton("Yenile", plugTab);
    plugLay->addWidget(m_plugins, 1);
    plugLay->addWidget(plBtn);
    connect(plBtn, &QPushButton::clicked, this, &DiagnosticsDialog::refreshPlugins);
    m_tabs->addTab(plugTab, "Eklentiler");

    // --- Stage 22: alt şerit — tanı raporu ---
    auto* botRow = new QHBoxLayout();
    auto* eBtn = new QPushButton("Tanı Raporunu Kaydet...", this);
    eBtn->setToolTip("Sürüm + araçlar + performans + erişilebilirlik özeti");
    botRow->addStretch(1);
    botRow->addWidget(eBtn);
    lay->addLayout(botRow);
    connect(eBtn, &QPushButton::clicked, this, &DiagnosticsDialog::exportReport);

    connect(&m_probeWatcher, &QFutureWatcher<QList<ToolInfo>>::finished, this, [this]() {
        if (m_probeWatcher.isCanceled()) return;
        m_tools->setPlainText(ToolchainProbe::report(m_probeWatcher.result()));
    });

    refreshToolchain();
    refreshPerformance();
    refreshBackups();
    refreshA11y();
    refreshPlugins();
}

void DiagnosticsDialog::setBackupManager(BackupManager* bm) {
    m_bm = bm;
    refreshBackups();
}

void DiagnosticsDialog::refreshToolchain() {
    if (m_probeWatcher.isRunning()) return;
    m_tools->setPlainText("Araçlar taranıyor...");
    m_probeWatcher.setFuture(QtConcurrent::run([]() { return ToolchainProbe::probeAll(); }));
}

void DiagnosticsDialog::refreshPerformance() {
    m_perf->setPlainText(PerfMonitor::instance().summary());
}

void DiagnosticsDialog::refreshBackups() {
    m_backups->clear();
    if (!m_bm) return;
    for (const BackupEntry& e : m_bm->list()) {
        auto* it = new QTreeWidgetItem(m_backups);
        it->setText(0, QFileInfo(e.original).fileName() + "  —  " + e.original);
        it->setText(1, QDateTime::fromMSecsSinceEpoch(e.whenMs).toString("yyyy-MM-dd HH:mm:ss"));
        it->setText(2, QString("%1 B").arg(e.size));
        it->setData(0, Qt::UserRole, e.backupPath);
        it->setData(0, Qt::UserRole + 1, e.original);
    }
}

void DiagnosticsDialog::checkUpdate() {
    const AboutInfo::Update u = AboutInfo::parseManifest(m_updateIn->toPlainText());
    if (!u.valid) {
        m_updateIn->append("\n[!] Geçersiz manifest.");
        return;
    }
    if (AboutInfo::isNewer(u.version, AboutInfo::version()))
        m_updateIn->append(QString("\n[↑] Yeni sürüm: %1 (mevcut %2)\n%3")
                               .arg(u.version, AboutInfo::version(), u.url));
    else
        m_updateIn->append(QString("\n[✓] Güncel (en son: %1).").arg(u.version));
}

void DiagnosticsDialog::restoreBackup() {
    QTreeWidgetItem* it = m_backups->currentItem();
    if (!it || !m_bm) return;
    const QString path = it->data(0, Qt::UserRole).toString();
    const QString original = it->data(0, Qt::UserRole + 1).toString();
    if (m_bm->restore(path)) emit backupRestored(original);
}

void DiagnosticsDialog::setPluginEngine(PluginEngine* eng) {
    m_pluginEng = eng;
    refreshPlugins();
}

void DiagnosticsDialog::refreshPlugins() {
    if (!m_pluginEng) {
        m_plugins->setPlainText("(eklenti motoru yok)");
        return;
    }
    QStringList out;
    for (const auto& p : m_pluginEng->plugins())
        out << QString("%1 v%2 — %3").arg(p.name, p.version.isEmpty() ? "?" : p.version,
                                          p.quarantined ? "KARANTİNADA"
                                          : p.loaded    ? "yüklendi"
                                                        : "pasif (" + p.error + ")");
    out << "";
    out << "== Günlük ==";
    out << m_pluginEng->logLines();
    m_plugins->setPlainText(out.join("\n"));
}

void DiagnosticsDialog::deleteBackup() {
    QTreeWidgetItem* it = m_backups->currentItem();
    if (!it || !m_bm) return;
    m_bm->remove(it->data(0, Qt::UserRole).toString());
    refreshBackups();
}

// Stage 22: erişilebilirlik + çeviri kapsama denetimi
void DiagnosticsDialog::refreshA11y() {
    QStringList out;
    out << "== Tema kontrastı (" + ThemeManager::instance().current() + ") ==";
    out << A11yCheck::checkTheme(ThemeManager::instance().tokens());
    out << "";
    out << "== Çeviri kapsama (tr/en) ==";
    const QStringList keys = LanguageManager::instance().allKeys();
    QStringList missing;
    for (const QString& k : keys) {
        if (!LanguageManager::instance().hasTranslation(k, "tr")) missing << k + " [tr]";
        if (!LanguageManager::instance().hasTranslation(k, "en")) missing << k + " [en]";
    }
    out << (missing.isEmpty() ? QString("%1 anahtar — tamamı çevrili ✓").arg(keys.size())
                              : "Eksik:\n" + missing.join("\n"));
    m_a11y->setPlainText(out.join("\n"));
}

// Stage 22: tanı raporunu metin dosyası olarak kaydet
void DiagnosticsDialog::exportReport() {
    const QString p = QFileDialog::getSaveFileName(this, "Tanı Raporunu Kaydet",
                                                   "verso-tani-raporu.txt",
                                                   "Metin (*.txt)");
    if (p.isEmpty()) return;
    QString r;
    r += "Verso Coder tanı raporu — " +
         QDateTime::currentDateTime().toString(Qt::ISODate) + "\n\n";
    r += "== Hakkında ==\n" + AboutInfo::appName() + " " + AboutInfo::version() + "\n" +
         AboutInfo::buildInfo() + "\n\n";
    r += "== Araç Zinciri ==\n" + m_tools->toPlainText() + "\n\n";
    r += "== Performans ==\n" + m_perf->toPlainText() + "\n\n";
    r += "== Erişilebilirlik ==\n" + m_a11y->toPlainText() + "\n";
    QFile f(p);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) f.write(r.toUtf8());
}

void DiagnosticsDialog::refreshCrashes() {
    if (!m_crash) return;
    const QStringList dumps = CrashHandler::pendingDumps();
    if (dumps.isEmpty()) {
        m_crash->setPlainText(QStringLiteral("Çökme dökümü yok."));
        return;
    }
    QStringList lines;
    for (const QString& d : dumps) lines << CrashHandler::crashDir() + "/" + d;
    m_crash->setPlainText(QString("%1 döküm:\n").arg(dumps.size()) + lines.join("\n"));
}