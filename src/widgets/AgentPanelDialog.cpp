#include "AgentPanelDialog.h"
#include "../core/AgentBudget.h"
#include "../core/AgentMemory.h"
#include "../core/AgentPolicy.h"
#include "../core/AgentRunStore.h"
#include "../core/ProjectHealth.h"
#include "../core/SettingsManager.h"
#include "../core/SkillChain.h"
#include "../core/SkillRegistry.h"
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QTabWidget>
#include <QVBoxLayout>

static QString agentDataDir() {
    QString d = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/agent";
    QDir().mkpath(d);
    return d;
}

AgentPanelDialog::AgentPanelDialog(const QString& projectRoot, QWidget* parent)
    : QDialog(parent), m_root(projectRoot) {
    setWindowTitle("Ajan Paneli");
    resize(760, 560);

    const AppSettings s = SettingsManager::instance().load();
    m_tabs = new QTabWidget(this);

    // --- Durum sekmesi ---
    auto* st = new QWidget(m_tabs);
    auto* fl = new QFormLayout(st);
    const AgentPolicy pol = s.agentAutonomous ? AgentPolicy::autonomousReadOnly(4)
                                              : [&] {
                                                    AgentPolicy p = AgentPolicy::safeDefault();
                                                    p.maxSteps = s.agentMaxToolCalls > 0 ? 8 : 5;
                                                    p.allowWrite = true; // ajan modu yazma izni
                                                    p.allowCommand = true;
                                                    p.allowTests = true;
                                                    return p;
                                                }();
    m_policyLabel = new QLabel(pol.summary(), st);
    fl->addRow("Politika:", m_policyLabel);
    m_budgetLabel = new QLabel(AgentBudget().summary(), st);
    fl->addRow("Bütçe sınırları:", m_budgetLabel);
    m_healthLabel = new QLabel("—", st);
    fl->addRow("Proje sağlığı:", m_healthLabel);
    m_healthNotes = new QPlainTextEdit(st);
    m_healthNotes->setReadOnly(true);
    m_healthNotes->setMaximumHeight(140);
    fl->addRow("Öneriler:", m_healthNotes);
    auto* bHealth = new QPushButton("Sağlık taraması çalıştır", st);
    connect(bHealth, &QPushButton::clicked, this, &AgentPanelDialog::runHealth);
    fl->addRow("", bHealth);
    m_tabs->addTab(st, "Durum");

    // --- Beceriler ---
    auto* sk = new QWidget(m_tabs);
    auto* skl = new QVBoxLayout(sk);
    m_skills = new QListWidget(sk);
    m_skills->setToolTip("Hedefe göre otomatik seçilen beceri zinciri (adım sırası)");
    for (const Skill& k : SkillRegistry::all()) {
        auto* it = new QListWidgetItem(QString("%1 — %2").arg(k.name, k.description), m_skills);
        it->setToolTip("adımlar:\n  " + k.steps.join("\n  ") +
                       "\n\naraçlar: " + k.tools.join(", ") +
                       (k.requires.isEmpty() ? QString() : "\n\ngereken: " + k.requires.join(", ")));
    }
    skl->addWidget(m_skills);
    m_tabs->addTab(sk, "Beceriler");

    // --- Bellek ---
    auto* me = new QWidget(m_tabs);
    auto* mel = new QVBoxLayout(me);
    m_memory = new QListWidget(me);
    mel->addWidget(m_memory, 1);
    auto* mrow = new QHBoxLayout();
    auto* bAdd = new QPushButton("Not ekle", me);
    auto* bForget = new QPushButton("Seçili notu sil", me);
    mrow->addWidget(bAdd);
    mrow->addWidget(bForget);
    mrow->addStretch(1);
    mel->addLayout(mrow);
    connect(bAdd, &QPushButton::clicked, this, &AgentPanelDialog::addNote);
    connect(bForget, &QPushButton::clicked, this, &AgentPanelDialog::forgetNote);
    m_tabs->addTab(me, "Bellek");

    // --- Koşu günlüğü ---
    auto* rl = new QWidget(m_tabs);
    auto* rv = new QVBoxLayout(rl);
    m_runs = new QListWidget(rl);
    m_runs->setMaximumHeight(200);
    rv->addWidget(m_runs);
    m_runDetail = new QPlainTextEdit(rl);
    m_runDetail->setReadOnly(true);
    rv->addWidget(m_runDetail, 1);
    auto* rrow = new QHBoxLayout();
    auto* bRevert = new QPushButton("Bu koşuyu geri al", rl);
    auto* bDelete = new QPushButton("Kaydı sil", rl);
    rrow->addWidget(bRevert);
    rrow->addWidget(bDelete);
    rrow->addStretch(1);
    rv->addLayout(rrow);
    connect(m_runs, &QListWidget::itemSelectionChanged, this, &AgentPanelDialog::onRunSelected);
    connect(bRevert, &QPushButton::clicked, this, &AgentPanelDialog::revertRun);
    connect(bDelete, &QPushButton::clicked, this, &AgentPanelDialog::deleteRun);
    m_tabs->addTab(rl, "Koşu Günlüğü");

    auto* lay = new QVBoxLayout(this);
    lay->addWidget(m_tabs, 1);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    lay->addWidget(box);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::accept);

    refreshAll();
}

void AgentPanelDialog::setRunCount(int n) {
    m_budgetLabel->setText(m_budgetLabel->text() + QString("  ·  kayıtlı koşu: %1").arg(n));
}

void AgentPanelDialog::runHealth() {
    const ProjectHealth h = ProjectHealth::scan(m_root);
    m_healthLabel->setText(h.summary());
    QStringList notes = h.notes;
    notes << QString("uzun dosya: %1 · dev dosya: %2").arg(h.longFiles).arg(h.hugeFiles);
    if (h.maxFileLines > 0)
        notes << QString("en uzun: %1 (%2 satır)").arg(h.maxFilePath).arg(h.maxFileLines);
    m_healthNotes->setPlainText(notes.join("\n"));
}

void AgentPanelDialog::addNote() {
    bool ok = false;
    const QString text = QInputDialog::getMultiLineText(
        this, "Belleğe not ekle", "Not (kurallar, yollar, komutlar):", QString(), &ok);
    if (!ok || text.trimmed().isEmpty()) return;
    QStringList tagChoices = AgentMemory::tags();
    const QString fallback = tagChoices.last();
    tagChoices.removeLast();
    const QString tag =
        QInputDialog::getItem(this, "Not türü", "Tür:", tagChoices, 0, false, &ok);
    if (!ok) return;
    AgentMemory mem(agentDataDir() + "/memory.json");
    mem.add(text.trimmed(), tag.isEmpty() ? fallback : tag);
    refreshAll();
}

void AgentPanelDialog::forgetNote() {
    auto* it = m_memory->currentItem();
    if (!it) return;
    const QString id = it->data(Qt::UserRole).toString();
    AgentMemory mem(agentDataDir() + "/memory.json");
    mem.remove(id);
    refreshAll();
}

void AgentPanelDialog::onRunSelected() {
    auto* it = m_runs->currentItem();
    if (!it) { m_runDetail->clear(); return; }
    const QString id = it->data(Qt::UserRole).toString();
    AgentRunStore store(agentDataDir() + "/runs");
    const AgentRun r = store.load(id);
    if (r.id.isEmpty()) { m_runDetail->clear(); return; }
    QString out = QString("Görev: %1\nDurum: %2 · %3 adım · %4 araç · %5 dosya\n\n")
                      .arg(r.task.left(400), r.statusLabel())
                      .arg(r.steps)
                      .arg(r.toolCalls)
                      .arg(r.changedFiles.size());
    out += AgentRunStore::diffSummary(r);
    if (!r.changedFiles.isEmpty()) {
        out += "\n\nGeri alınabilir dosyalar:\n";
        for (const RunFile& f : r.changedFiles)
            out += QString("- %1%2\n").arg(f.path, f.created ? " (yeni oluşturulmuş)" : "");
    }
    m_runDetail->setPlainText(out);
}

void AgentPanelDialog::revertRun() {
    auto* it = m_runs->currentItem();
    if (!it) return;
    const QString id = it->data(Qt::UserRole).toString();
    AgentRunStore store(agentDataDir() + "/runs");
    const AgentRun r = store.load(id);
    if (r.id.isEmpty() || r.changedFiles.isEmpty()) {
        QMessageBox::information(this, "Geri al", "Bu koşunun geri alınabilir dosya değişikliği yok.");
        return;
    }
    const auto ans = QMessageBox::question(
        this, "Koşuyu geri al",
        QString("%1 koşusunun %2 dosyadaki değişikliği geri alınsın mı?\n\n"
                "Sonradan elle değiştirilmiş dosyalar atlanır.")
            .arg(id)
            .arg(r.changedFiles.size()),
        QMessageBox::Yes | QMessageBox::Cancel);
    if (ans != QMessageBox::Yes) return;
    int reverted = 0;
    QStringList skipped;
    const QString msg = AgentRunStore::revert(r, &reverted, &skipped);
    QMessageBox::information(this, "Geri al", msg);
    onRunSelected();
}

void AgentPanelDialog::deleteRun() {
    auto* it = m_runs->currentItem();
    if (!it) return;
    const QString id = it->data(Qt::UserRole).toString();
    AgentRunStore store(agentDataDir() + "/runs");
    if (store.remove(id)) refreshAll();
    m_runDetail->clear();
}

void AgentPanelDialog::refreshAll() {
    AgentMemory mem(agentDataDir() + "/memory.json");
    m_memory->clear();
    for (const MemoryNote& n : mem.notes()) {
        auto* it = new QListWidgetItem(QString("[%1] %2").arg(n.tag, n.text.left(160)), m_memory);
        it->setData(Qt::UserRole, n.id);
        it->setToolTip(n.text);
    }

    AgentRunStore store(agentDataDir() + "/runs");
    m_runs->clear();
    for (const AgentRun& r : store.list(50)) {
        auto* it = new QListWidgetItem(r.oneLine(), m_runs);
        it->setData(Qt::UserRole, r.id);
        it->setToolTip(r.task.left(400));
    }
    m_budgetLabel->setText(AgentBudget().summary());
}
