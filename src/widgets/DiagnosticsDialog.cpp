#include "DiagnosticsDialog.h"
#include "../core/BackupManager.h"
#include "../core/PerfMonitor.h"
#include <QDateTime>
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
    aboutLay->addWidget(new QLabel("Güncelleme manifesti (JSON) yapıştır:", aboutTab));
    m_updateIn = new QTextEdit(aboutTab);
    m_updateIn->setMaximumHeight(70);
    m_updateIn->setPlaceholderText("{\"version\":\"1.1.0\",\"url\":\"https://...\",\"notes\":\"...\"}");
    auto* upBtn = new QPushButton("Güncellemeyi denetle", aboutTab);
    aboutLay->addWidget(m_updateIn);
    aboutLay->addWidget(upBtn);
    connect(upBtn, &QPushButton::clicked, this, &DiagnosticsDialog::checkUpdate);
    m_tabs->addTab(aboutTab, "Hakkında");

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

    connect(&m_probeWatcher, &QFutureWatcher<QList<ToolInfo>>::finished, this, [this]() {
        if (m_probeWatcher.isCanceled()) return;
        m_tools->setPlainText(ToolchainProbe::report(m_probeWatcher.result()));
    });

    refreshToolchain();
    refreshPerformance();
    refreshBackups();
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

void DiagnosticsDialog::deleteBackup() {
    QTreeWidgetItem* it = m_backups->currentItem();
    if (!it || !m_bm) return;
    m_bm->remove(it->data(0, Qt::UserRole).toString());
    refreshBackups();
}
