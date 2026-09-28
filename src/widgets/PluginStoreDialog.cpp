#include "PluginStoreDialog.h"
#include "../core/AboutInfo.h"
#include "../core/PluginEngine.h"
#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSplitter>
#include <QTextEdit>
#include <QVBoxLayout>

PluginStoreDialog::PluginStoreDialog(PluginEngine* engine, QWidget* parent)
    : QDialog(parent), m_eng(engine) {
    setWindowTitle("Eklenti Mağazası");
    resize(680, 460);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);
    auto* split = new QSplitter(Qt::Horizontal, this);
    m_list = new QListWidget(split);
    m_info = new QTextEdit(split);
    m_info->setReadOnly(true);
    m_info->setFontFamily("monospace");
    split->addWidget(m_list);
    split->addWidget(m_info);
    split->setSizes({220, 440});
    lay->addWidget(split, 1);
    auto* row = new QHBoxLayout();
    auto* bRefresh = new QPushButton("Yenile", this);
    auto* bInstall = new QPushButton("Kur / Güncelle", this);
    auto* bDel = new QPushButton("Kaldır", this);
    row->addWidget(bRefresh);
    row->addWidget(bInstall);
    row->addWidget(bDel);
    lay->addLayout(row);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(box, &QDialogButtonBox::rejected, this, &PluginStoreDialog::reject);
    lay->addWidget(box);
    connect(bRefresh, &QPushButton::clicked, this, &PluginStoreDialog::refreshRemote);
    connect(bInstall, &QPushButton::clicked, this, &PluginStoreDialog::installSelected);
    connect(bDel, &QPushButton::clicked, this, &PluginStoreDialog::removeSelected);
    connect(m_list, &QListWidget::currentRowChanged, this,
            &PluginStoreDialog::refreshList);
    refreshList();
}

PluginStore::Entry PluginStoreDialog::curEntry() const {
    auto* it = m_list ? m_list->currentItem() : nullptr;
    if (!it) return {};
    const QString id = it->data(Qt::UserRole).toString();
    for (const auto& e : m_entries)
        if (e.id == id) return e;
    return {};
}

void PluginStoreDialog::refreshRemote() {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString err;
    m_entries = PluginStore::fetchIndex(&err);
    m_lastError = err;
    QApplication::restoreOverrideCursor();
    if (!err.isEmpty() && m_entries.isEmpty())
        QMessageBox::warning(this, "Mağaza", "Kayıt deposuna ulaşılamadı:\n" + err);
    refreshList();
}

void PluginStoreDialog::refreshList() {
    const QString sel = m_list->currentItem()
                            ? m_list->currentItem()->data(Qt::UserRole).toString()
                            : QString();
    m_list->clear();
    const QString dir = m_eng ? m_eng->pluginDir() : QString();
    for (const auto& e : m_entries) {
        const QString inst = dir.isEmpty() ? QString()
                                           : PluginStore::installedVersion(dir, e.id);
        QString tag;
        if (!inst.isEmpty()) {
            const int c = AboutInfo::compareVersions(e.version, inst);
            tag = c > 0 ? QString("  [güncelleme: v%1 → v%2]").arg(inst, e.version)
                        : QString("  [kurulu v%1]").arg(inst);
        }
        auto* it = new QListWidgetItem(e.name + "  v" + e.version + tag, m_list);
        it->setData(Qt::UserRole, e.id);
        if (e.id == sel) m_list->setCurrentItem(it);
    }
    QStringList info;
    if (!m_lastError.isEmpty() && m_entries.isEmpty()) {
        info << ("Durum: " + m_lastError);
        info << "Yenile ile tekrar deneyin.";
    } else if (m_entries.isEmpty()) {
        info << "Liste boş. Mağazayı görmek için Yenile'ye basın.";
    }
    const PluginStore::Entry e = curEntry();
    if (e.valid()) {
        const QString inst =
            dir.isEmpty() ? QString() : PluginStore::installedVersion(dir, e.id);
        info << ("Ad: " + e.name);
        info << ("Kimlik: " + e.id);
        info << ("Kayıt sürümü: v" + e.version);
        info << ("Kurulu: " + (inst.isEmpty() ? "-" : ("v" + inst)));
        info << ("Yazar: " + (e.author.isEmpty() ? "-" : e.author));
        info << ("Açıklama: " + (e.description.isEmpty() ? "-" : e.description));
        info << ("İzinler: " +
                 (e.permissions.isEmpty() ? "yok" : e.permissions.join(", ")));
        if (!e.minApp.isEmpty()) info << ("En düşük uygulama: v" + e.minApp);
    }
    m_info->setPlainText(info.join('\n'));
}

static QString headerVersion(const QString& src) {
    static QRegularExpression re(R"(^\s*//\s*@version\s+(.+)$)",
                                 QRegularExpression::MultilineOption);
    auto m = re.match(src);
    return m.hasMatch() ? m.captured(1).trimmed().left(40) : QString();
}

void PluginStoreDialog::installSelected() {
    const PluginStore::Entry e = curEntry();
    if (!e.valid() || !m_eng) return;
    const QString dir = m_eng->pluginDir();
    const QString inst = PluginStore::installedVersion(dir, e.id);
    if (!inst.isEmpty() && AboutInfo::compareVersions(e.version, inst) <= 0) {
        QMessageBox::information(this, "Mağaza", e.name + " zaten güncel (v" + inst + ").");
        return;
    }
    QString confirm = QString("%1 v%2 kurulacak.").arg(e.name, e.version);
    if (!inst.isEmpty()) confirm = QString("%1 v%2 → v%3 güncellenecek.").arg(e.name, inst, e.version);
    confirm += QString("\n\nİstediği izinler: %1")
                   .arg(e.permissions.isEmpty() ? "yok" : e.permissions.join(", "));
    if (!e.minApp.isEmpty() && AboutInfo::isNewer(e.minApp, AboutInfo::version()))
        confirm += QString("\n\nUYARI: bu eklenti uygulama v%1 istiyor (sizde v%2). "
                           "Çalışmayabilir.")
                       .arg(e.minApp, AboutInfo::version());
    auto r = QMessageBox::question(this, "Kur", confirm,
                                   QMessageBox::Yes | QMessageBox::Cancel);
    if (r != QMessageBox::Yes) return;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString err;
    const QString src =
        QString::fromUtf8(PluginStore::fetchUrl(PluginStore::pluginUrl(e), &err));
    QApplication::restoreOverrideCursor();
    if (!err.isEmpty()) {
        QMessageBox::warning(this, "Mağaza", "İndirilemedi:\n" + err);
        return;
    }
    // Kayıt bütünlüğü: inen dosyanın başlığı listedeki sürümle uyuşmalı
    if (headerVersion(src) != e.version) {
        QMessageBox::warning(this, "Mağaza",
                             "Kayıt tutarsız: inen dosya v" + headerVersion(src) +
                                 ", listede v" + e.version + ". Kurulum durduruldu.");
        return;
    }
    if (!PluginStore::install(e, src, dir, &err)) {
        QMessageBox::warning(this, "Mağaza", "Kurulamadı:\n" + err);
        return;
    }
    m_eng->loadAll(dir);
    emit changed();
    refreshList();
    QMessageBox::information(this, "Mağaza", e.name + " v" + e.version + " kuruldu.");
}

void PluginStoreDialog::removeSelected() {
    const PluginStore::Entry e = curEntry();
    if (!e.valid() || !m_eng) return;
    if (PluginStore::installedVersion(m_eng->pluginDir(), e.id).isEmpty()) {
        QMessageBox::information(this, "Kaldır", e.name + " kurulu değil.");
        return;
    }
    auto r = QMessageBox::question(this, "Kaldır", e.name + " silinsin mi?",
                                   QMessageBox::Yes | QMessageBox::Cancel);
    if (r != QMessageBox::Yes) return;
    m_eng->unload(e.id);
    QFile::remove(m_eng->pluginDir() + "/" + e.id + ".js");
    QSettings q("Verso", "VersoCoder");
    QStringList en = q.value("plugin/enabled").toStringList();
    en.removeAll(e.id);
    q.setValue("plugin/enabled", en);
    q.remove("plugin/perms/" + e.id);
    q.remove("plugin/seal/" + e.id);
    m_eng->loadAll(m_eng->pluginDir());
    emit changed();
    refreshList();
}
