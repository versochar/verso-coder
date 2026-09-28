#include "PluginStoreDialog.h"
#include "../core/AboutInfo.h"
#include "../core/PluginEngine.h"
#include <QApplication>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSplitter>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

PluginStoreDialog::PluginStoreDialog(PluginEngine* engine, QWidget* parent)
    : QDialog(parent), m_eng(engine) {
    setWindowTitle("Eklenti Mağazası");
    resize(680, 460);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);
    m_status = new QLabel("Yenile ile kayıt okunur.", this);
    lay->addWidget(m_status);
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
    m_btnRefresh = new QPushButton("Yenile", this);
    m_btnInstall = new QPushButton("Kur / Güncelle", this);
    m_btnUpdateAll = new QPushButton("Tümünü Güncelle", this);
    auto* bDel = new QPushButton("Kaldır", this);
    row->addWidget(m_btnRefresh);
    row->addWidget(m_btnInstall);
    row->addWidget(m_btnUpdateAll);
    row->addWidget(bDel);
    lay->addLayout(row);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(box, &QDialogButtonBox::rejected, this, &PluginStoreDialog::reject);
    lay->addWidget(box);
    connect(m_btnRefresh, &QPushButton::clicked, this, &PluginStoreDialog::refreshRemote);
    connect(m_btnInstall, &QPushButton::clicked, this,
            &PluginStoreDialog::installSelected);
    connect(m_btnUpdateAll, &QPushButton::clicked, this, &PluginStoreDialog::updateAll);
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

void PluginStoreDialog::setBusy(bool busy, const QString& msg) {
    m_loading = busy;
    if (m_btnRefresh) m_btnRefresh->setEnabled(!busy);
    if (m_btnInstall) m_btnInstall->setEnabled(!busy);
    if (m_btnUpdateAll) m_btnUpdateAll->setEnabled(!busy);
    if (m_status) m_status->setText(msg);
    if (busy) QApplication::setOverrideCursor(Qt::WaitCursor);
    else QApplication::restoreOverrideCursor();
}

void PluginStoreDialog::refreshRemote() {
    if (m_loading) return; // çift tıklama koruması
    if (!m_net) m_net = new QNetworkAccessManager(this);
    setBusy(true, "Kayıt okunuyor...");
    QNetworkReply* reply = m_net->get(QNetworkRequest(QUrl(PluginStore::indexUrl())));
    auto* timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, this, [this, reply, timer]() {
        timer->stop();
        timer->deleteLater();
        QString err;
        if (reply->error() != QNetworkReply::NoError) {
            err = reply->error() == QNetworkReply::OperationCanceledError
                      ? "zaman aşımı"
                      : reply->errorString().left(200);
            m_entries.clear();
        } else {
            m_entries = PluginStore::parseIndex(reply->readAll(), &err);
        }
        reply->deleteLater();
        m_lastError = err;
        if (err.isEmpty()) {
            PluginStore::saveCache(m_entries);
            m_offlineNote.clear();
            setBusy(false,
                    QString("%1 eklenti listelendi.").arg(m_entries.size()));
        } else if (!m_entries.isEmpty()) {
            // Kayıt öldü ama eski liste duruyor: koru
            setBusy(false, "Hata: " + err);
        } else {
            // Çevrimdışı önbellek: son başarılı liste
            QDateTime when;
            const auto cached = PluginStore::loadCache(&when);
            if (!cached.isEmpty()) {
                m_entries = cached;
                m_offlineNote = when.isValid()
                                    ? QString("çevrimdışı (%1 listesi)")
                                          .arg(when.toLocalTime().toString("d MMM HH:mm"))
                                    : QString("çevrimdışı liste");
                setBusy(false, m_offlineNote + " — " + err);
            } else {
                setBusy(false, "Hata: " + err);
            }
        }
        if (!m_lastError.isEmpty() && m_entries.isEmpty())
            QMessageBox::warning(this, "Mağaza", "Kayıt deposuna ulaşılamadı:\n" + m_lastError);
        refreshList();
    });
    timer->start(15000);
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
    if (!m_offlineNote.isEmpty()) info << ("Durum: " + m_offlineNote);
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
    if (!e.valid() || !m_eng || m_loading) return;
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
    // İndirme eşzamansız: arayüz donmaz
    if (!m_net) m_net = new QNetworkAccessManager(this);
    setBusy(true, e.name + " indiriliyor...");
    QNetworkReply* reply = m_net->get(QNetworkRequest(QUrl(PluginStore::pluginUrl(e))));
    auto* timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, this, [this, reply, timer, e, dir]() {
        timer->stop();
        timer->deleteLater();
        QString err;
        QString src;
        if (reply->error() != QNetworkReply::NoError) {
            err = reply->error() == QNetworkReply::OperationCanceledError
                      ? "zaman aşımı"
                      : reply->errorString().left(200);
        } else {
            src = QString::fromUtf8(reply->readAll());
        }
        reply->deleteLater();
        setBusy(false, m_entries.isEmpty() ? QString()
                                           : QString("%1 eklenti listelendi.")
                                                 .arg(m_entries.size()));
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
    });
    timer->start(30000);
}

void PluginStoreDialog::updateAll() {
    if (!m_eng || m_loading) return;
    const QList<PluginStore::Entry> bekleyen =
        PluginStore::updatesAvailable(m_entries, m_eng->pluginDir());
    if (bekleyen.isEmpty()) {
        QMessageBox::information(this, "Mağaza", "Tüm eklentiler güncel.");
        return;
    }
    QStringList adlar;
    for (const auto& e : bekleyen) adlar << (e.name + " v" + e.version);
    auto r = QMessageBox::question(
        this, "Tümünü Güncelle",
        QString("%1 eklenti güncellenecek:\n- %2")
            .arg(bekleyen.size())
            .arg(adlar.join("\n- ")),
        QMessageBox::Yes | QMessageBox::Cancel);
    if (r != QMessageBox::Yes) return;
    if (!m_net) m_net = new QNetworkAccessManager(this);
    m_updateQueue = bekleyen;
    m_updateDone = 0;
    m_updateError.clear();
    setBusy(true, "Güncelleniyor (0/" + QString::number(bekleyen.size()) + ")...");
    nextUpdate();
}

void PluginStoreDialog::nextUpdate() {
    if (!m_eng) {
        setBusy(false, QString());
        return;
    }
    if (m_updateQueue.isEmpty()) {
        const QString dir = m_eng->pluginDir();
        m_eng->loadAll(dir);
        emit changed();
        refreshList();
        QString s = QString("%1 eklenti güncellendi.").arg(m_updateDone);
        if (!m_updateError.isEmpty()) s += "\nHata: " + m_updateError;
        setBusy(false, s);
        QMessageBox::information(this, "Mağaza", s);
        return;
    }
    const PluginStore::Entry e = m_updateQueue.takeFirst();
    setBusy(true, "Güncelleniyor: " + e.name + "...");
    QNetworkReply* reply = m_net->get(QNetworkRequest(QUrl(PluginStore::pluginUrl(e))));
    auto* timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, this, [this, reply, timer, e]() {
        timer->stop();
        timer->deleteLater();
        QString err;
        QString src;
        if (reply->error() != QNetworkReply::NoError) {
            err = "indirilemedi";
        } else {
            src = QString::fromUtf8(reply->readAll());
            if (headerVersion(src) != e.version || !PluginStore::install(e, src, m_eng->pluginDir(), &err))
                err = err.isEmpty() ? "bütünlük uyuşmadı" : err;
            else
                ++m_updateDone;
        }
        reply->deleteLater();
        if (!err.isEmpty() && m_updateError.isEmpty()) m_updateError = e.name + ": " + err;
        nextUpdate();
    });
    timer->start(30000);
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
