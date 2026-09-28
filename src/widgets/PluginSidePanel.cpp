#include "PluginSidePanel.h"
#include "../core/PluginEngine.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

PluginSidePanel::PluginSidePanel(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(8, 8, 8, 8);
    lay->setSpacing(6);
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText("Eklenti ara...");
    m_search->setClearButtonEnabled(true);
    lay->addWidget(m_search);
    auto* l1 = new QLabel("KURULU", this);
    l1->setStyleSheet("font-weight:bold;font-size:11px;");
    lay->addWidget(l1);
    m_list = new QListWidget(this);
    lay->addWidget(m_list, 3);
    auto* l2 = new QLabel("GÖRÜNÜMLER", this);
    l2->setStyleSheet("font-weight:bold;font-size:11px;");
    lay->addWidget(l2);
    m_viewList = new QListWidget(this);
    lay->addWidget(m_viewList, 1);
    auto* row = new QHBoxLayout();
    auto* bStore = new QPushButton("Mağaza...", this);
    auto* bManage = new QPushButton("Yönetici...", this);
    auto* bFolder = new QPushButton("Klasör", this);
    row->addWidget(bStore);
    row->addWidget(bManage);
    row->addWidget(bFolder);
    lay->addLayout(row);
    connect(m_search, &QLineEdit::textChanged, this,
            [this](const QString&) { refresh(); });
    connect(m_list, &QListWidget::itemChanged, this,
            &PluginSidePanel::onItemChanged);
    connect(m_viewList, &QListWidget::itemDoubleClicked, this,
            &PluginSidePanel::onViewActivated);
    connect(bStore, &QPushButton::clicked, this, &PluginSidePanel::storeRequested);
    connect(bManage, &QPushButton::clicked, this, &PluginSidePanel::manageRequested);
    connect(bFolder, &QPushButton::clicked, this, &PluginSidePanel::folderRequested);
    refresh();
}

void PluginSidePanel::refresh() {
    if (!m_list) return;
    m_guard = true;
    m_list->clear();
    const QString q = m_search ? m_search->text().trimmed().toLower() : QString();
    if (m_eng) {
        QSettings st("Verso", "VersoCoder");
        const bool hasSet = st.contains("plugin/enabled");
        const QStringList enabled = st.value("plugin/enabled").toStringList();
        for (const auto& p : m_eng->plugins()) {
            if (!q.isEmpty() &&
                !p.name.toLower().contains(q) && !p.id.contains(q))
                continue;
            QString t = p.name;
            if (!p.version.isEmpty()) t += "  v" + p.version;
            if (p.quarantined) t += "  [karantina]";
            else if (!p.loaded) t += "  [pasif]";
            else if (hasSet && !enabled.contains(p.id)) t += "  [kapalı]";
            auto* it = new QListWidgetItem(t, m_list);
            it->setData(Qt::UserRole, p.id);
            it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
            const bool on = p.loaded && (!hasSet || enabled.contains(p.id));
            it->setCheckState(on ? Qt::Checked : Qt::Unchecked);
            if (!p.loaded)
                it->setFlags(it->flags() & ~Qt::ItemIsUserCheckable);
        }
    }
    if (m_list->count() == 0)
        m_list->addItem(m_eng ? "(Mağaza'dan eklenti kurun)" : "(motor hazır değil)");
    m_viewList->clear();
    for (auto it = m_views.constBegin(); it != m_views.constEnd(); ++it) {
        auto* v = new QListWidgetItem(it.value(), m_viewList);
        v->setData(Qt::UserRole, it.key());
    }
    if (m_viewList->count() == 0) m_viewList->addItem("(görünüm yok)");
    m_guard = false;
}

void PluginSidePanel::onItemChanged(QListWidgetItem* it) {
    if (m_guard || !it || !m_eng) return;
    const QString id = it->data(Qt::UserRole).toString();
    if (id.isEmpty()) return;
    QSettings q("Verso", "VersoCoder");
    QStringList en = q.value("plugin/enabled").toStringList();
    if (!q.contains("plugin/enabled")) {
        // İlk kullanım: mevcutları koru (yöneticiyle aynı kural)
        for (const auto& p : m_eng->plugins())
            if (!en.contains(p.id)) en << p.id;
    }
    if (it->checkState() == Qt::Checked) {
        if (!en.contains(id)) en << id;
    } else {
        en.removeAll(id);
    }
    q.setValue("plugin/enabled", en);
    m_eng->loadAll(m_eng->pluginDir());
    emit pluginsChanged();
    refresh();
}

void PluginSidePanel::onViewActivated(QListWidgetItem* it) {
    if (!it) return;
    const QString cmd = it->data(Qt::UserRole).toString();
    if (!cmd.isEmpty()) emit viewRequested(cmd);
}
