#include "PluginManagerDialog.h"
#include "../core/PluginEngine.h"
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QTextEdit>
#include <QVBoxLayout>

PluginManagerDialog::PluginManagerDialog(PluginEngine* engine, QWidget* parent)
    : QDialog(parent), m_eng(engine) {
    setWindowTitle("Eklenti Yöneticisi");
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
    auto* bInstall = new QPushButton("Kur (klasör)...", this);
    auto* bDel = new QPushButton("Kaldır", this);
    auto* bToggle = new QPushButton("Aç/Kapa", this);
    auto* bPerms = new QPushButton("İzinler...", this);
    auto* bConf = new QPushButton("Ayarlar...", this);
    auto* bQuar = new QPushButton("Karantinayı kaldır", this);
    auto* bLog = new QPushButton("Günlük", this);
    row->addWidget(bInstall);
    row->addWidget(bDel);
    row->addWidget(bToggle);
    row->addWidget(bPerms);
    row->addWidget(bConf);
    row->addWidget(bQuar);
    row->addWidget(bLog);
    lay->addLayout(row);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(box, &QDialogButtonBox::rejected, this, &PluginManagerDialog::reject);
    lay->addWidget(box);
    connect(bInstall, &QPushButton::clicked, this, &PluginManagerDialog::installFromFolder);
    connect(bDel, &QPushButton::clicked, this, &PluginManagerDialog::removeSelected);
    connect(bToggle, &QPushButton::clicked, this, &PluginManagerDialog::toggleEnabled);
    connect(bPerms, &QPushButton::clicked, this, &PluginManagerDialog::editPermissions);
    connect(bConf, &QPushButton::clicked, this, &PluginManagerDialog::editSettings);
    connect(bQuar, &QPushButton::clicked, this,
            &PluginManagerDialog::clearQuarantineSel);
    connect(bLog, &QPushButton::clicked, this, &PluginManagerDialog::showLog);
    connect(m_list, &QListWidget::currentRowChanged, this,
            &PluginManagerDialog::refreshList);
    refreshList();
}

static QString curId(QListWidget* list) {
    auto* it = list->currentItem();
    return it ? it->data(Qt::UserRole).toString() : QString();
}

void PluginManagerDialog::refreshList() {
    const QString sel = curId(m_list);
    m_list->clear();
    if (!m_eng) return;
    const QStringList enabled =
        QSettings("Verso", "VersoCoder").value("plugin/enabled").toStringList();
    const bool hasSet =
        QSettings("Verso", "VersoCoder").contains("plugin/enabled");
    for (const auto& p : m_eng->plugins()) {
        const bool on = !hasSet || enabled.contains(p.id);
        auto* it = new QListWidgetItem(
            QString("%1%2  v%3").arg(on ? "" : "[kapalı] ", p.name, p.version), m_list);
        it->setData(Qt::UserRole, p.id);
        if (p.id == sel) m_list->setCurrentItem(it);
    }
    // Ayrıntı bölmesi
    QStringList info;
    for (const auto& p : m_eng->plugins()) {
        if (p.id != curId(m_list)) continue;
        info << ("Kimlik: " + p.id);
        info << ("Ad: " + p.name);
        info << ("Sürüm: " + (p.version.isEmpty() ? "-" : p.version));
        info << ("Yol: " + p.path);
        info << ("Durum: " + QString(p.quarantined ? "KARANTİNADA"
                                     : p.loaded   ? "yüklendi"
                                     : "pasif (" + p.error + ")"));
        if (p.loadMs > 2000)
            info << (QString("Yükleme: YAVAŞ (%1 ms)").arg(p.loadMs));
        info << ("İzinler: " + (p.permissions.isEmpty() ? "-" : p.permissions.join(", ")));
        info << ("Komutlar: " +
                 (p.commands.isEmpty() ? "-" : QString::number(p.commands.size())));
    }
    m_info->setPlainText(info.join('\n'));
}

void PluginManagerDialog::installFromFolder() {
    if (!m_eng) return;
    const QString dir = QFileDialog::getExistingDirectory(this, "Eklenti klasörü seç");
    if (dir.isEmpty()) return;
    int n = 0;
    QDir d(dir);
    for (const QFileInfo& fi : d.entryInfoList({"*.js"}, QDir::Files)) {
        const QString dst = m_eng->pluginDir() + "/" + fi.fileName();
        if (QFile::exists(dst)) continue;
        if (QFile::copy(fi.absoluteFilePath(), dst)) ++n;
    }
    if (n == 0) {
        QMessageBox::information(this, "Kur", "Kopyalanacak .js bulunamadı.");
        return;
    }
    // Yeni kurulanı etkin listesine ekle
    QSettings q("Verso", "VersoCoder");
    QStringList en = q.value("plugin/enabled").toStringList();
    for (const QFileInfo& fi : d.entryInfoList({"*.js"}, QDir::Files))
        if (!en.contains(fi.completeBaseName())) en << fi.completeBaseName();
    q.setValue("plugin/enabled", en);
    QMessageBox::information(this, "Kur", QString("%1 eklenti kuruldu. Yeniden yükleniyor...").arg(n));
    m_eng->loadAll(m_eng->pluginDir());
    refreshList();
}

void PluginManagerDialog::removeSelected() {
    const QString id = curId(m_list);
    if (id.isEmpty() || !m_eng) return;
    auto r = QMessageBox::question(this, "Kaldır", id + " silinsin mi?",
                                   QMessageBox::Yes | QMessageBox::Cancel);
    if (r != QMessageBox::Yes) return;
    m_eng->unload(id);
    QFile::remove(m_eng->pluginDir() + "/" + id + ".js");
    QSettings q("Verso", "VersoCoder");
    QStringList en = q.value("plugin/enabled").toStringList();
    en.removeAll(id);
    q.setValue("plugin/enabled", en);
    q.remove("plugin/perms/" + id);
    q.remove("plugin/seal/" + id);
    refreshList();
}

void PluginManagerDialog::toggleEnabled() {
    const QString id = curId(m_list);
    if (id.isEmpty()) return;
    QSettings q("Verso", "VersoCoder");
    QStringList en = q.value("plugin/enabled").toStringList();
    const bool hasSet = q.contains("plugin/enabled");
    // İlk kullanımda tümünü etkin say
    if (!hasSet && m_eng) {
        for (const auto& p : m_eng->plugins()) en << p.id;
    }
    if (en.contains(id)) en.removeAll(id);
    else en << id;
    q.setValue("plugin/enabled", en);
    if (m_eng) m_eng->loadAll(m_eng->pluginDir());
    refreshList();
}

void PluginManagerDialog::editPermissions() {
    const QString id = curId(m_list);
    if (id.isEmpty() || !m_eng) return;
    static const QStringList all = {"fs.read", "fs.write", "events", "ui", "net"};
    QStringList cur = m_eng->permOverride(id);
    bool hasOverride = QSettings("Verso", "VersoCoder").contains("plugin/perms/" + id);
    if (!hasOverride) {
        for (const auto& p : m_eng->plugins())
            if (p.id == id) cur = p.permissions;
    }
    bool ok = false;
    // Çoktan seçmeli: virgüllü liste düzenlet
    const QString v = QInputDialog::getText(
        this, "İzinler — " + id,
        "İzinler (virgüllü; boş = tümü yasak):\nfs.read, fs.write, events, ui",
        QLineEdit::Normal, cur.join(", "), &ok);
    if (!ok) return;
    QStringList out;
    for (const QString& t : v.split(',', Qt::SkipEmptyParts)) {
        const QString s = t.trimmed();
        if (all.contains(s) && !out.contains(s)) out << s;
    }
    m_eng->setPermOverride(id, out);
    m_eng->loadAll(m_eng->pluginDir());
    refreshList();
}

void PluginManagerDialog::clearQuarantineSel() {
    const QString id = curId(m_list);
    if (id.isEmpty() || !m_eng) return;
    m_eng->clearQuarantine(id);
    m_eng->loadAll(m_eng->pluginDir());
    refreshList();
}

void PluginManagerDialog::editSettings() {
    const QString id = curId(m_list);
    if (id.isEmpty()) return;
    // Eklentinin getConfig/setConfig anahtarları
    QSettings q("Verso", "VersoCoder");
    q.beginGroup("plugin/config/" + id);
    const QStringList keys = q.childKeys();
    q.endGroup();
    if (keys.isEmpty()) {
        QMessageBox::information(this, "Ayarlar", "Bu eklentinin ayarı yok.");
        return;
    }
    bool ok = false;
    const QString key = QInputDialog::getItem(this, "Ayarlar — " + id, "Anahtar:",
                                              keys, 0, false, &ok);
    if (!ok) return;
    q.beginGroup("plugin/config/" + id);
    const QString cur = q.value(key).toString();
    q.endGroup();
    const QString v = QInputDialog::getText(this, "Ayarlar — " + id, key + ":",
                                            QLineEdit::Normal, cur, &ok);
    if (!ok) return;
    q.beginGroup("plugin/config/" + id);
    q.setValue(key, v);
    q.endGroup();
}

void PluginManagerDialog::showLog() {
    if (!m_eng) return;
    m_info->setPlainText(m_eng->logLines().join('\n'));
}
