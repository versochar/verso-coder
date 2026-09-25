#include "RemoteConnectDialog.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

RemoteConnectDialog::RemoteConnectDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Uzağa Bağlan");
    resize(460, 420);
    auto* lay = new QVBoxLayout(this);
    auto* top = new QHBoxLayout();
    m_list = new QComboBox(this);
    auto* bNew = new QPushButton("Yeni", this);
    auto* bDel = new QPushButton("Sil", this);
    top->addWidget(m_list, 1);
    top->addWidget(bNew);
    top->addWidget(bDel);
    lay->addLayout(top);
    auto* form = new QFormLayout();
    m_name = new QLineEdit(this);
    m_host = new QLineEdit(this);
    m_host->setPlaceholderText("sunucu.ornek.com ya da 192.168.1.10");
    m_port = new QSpinBox(this);
    m_port->setRange(1, 65535);
    m_port->setValue(22);
    m_user = new QLineEdit(this);
    m_user->setPlaceholderText("(boş = yerel kullanıcı)");
    m_key = new QLineEdit(this);
    m_key->setPlaceholderText("~/.ssh/id_ed25519 (boş = agent/varsayılan)");
    auto* bKey = new QPushButton("...", this);
    bKey->setMaximumWidth(36);
    auto* keyRow = new QHBoxLayout();
    keyRow->addWidget(m_key, 1);
    keyRow->addWidget(bKey);
    auto* keyW = new QWidget(this);
    keyW->setLayout(keyRow);
    keyRow->setContentsMargins(0, 0, 0, 0);
    m_jump = new QLineEdit(this);
    m_jump->setPlaceholderText("kullanici@ara-host:22 (opsiyonel)");
    m_root = new QLineEdit(this);
    m_root->setPlaceholderText("/home/kullanici/proje");
    form->addRow("Profil adı:", m_name);
    form->addRow("Host:", m_host);
    form->addRow("Port:", m_port);
    form->addRow("Kullanıcı:", m_user);
    form->addRow("Anahtar:", keyW);
    form->addRow("Jump host:", m_jump);
    form->addRow("Uzak kök:", m_root);
    m_trust = new QCheckBox("Bilinmeyen host anahtarına ilk bağlanışta güven (TOFU)", this);
    form->addRow("", m_trust);
    lay->addLayout(form);
    auto* row = new QHBoxLayout();
    auto* bSave = new QPushButton("Kaydet", this);
    auto* bGo = new QPushButton("Bağlan", this);
    bGo->setDefault(true);
    row->addStretch(1);
    row->addWidget(bSave);
    row->addWidget(bGo);
    lay->addLayout(row);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    lay->addWidget(box);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_list, &QComboBox::currentTextChanged, this, &RemoteConnectDialog::onSelect);
    connect(bNew, &QPushButton::clicked, this, &RemoteConnectDialog::newProfile);
    connect(bDel, &QPushButton::clicked, this, &RemoteConnectDialog::deleteCurrent);
    connect(bSave, &QPushButton::clicked, this, &RemoteConnectDialog::saveCurrent);
    connect(bGo, &QPushButton::clicked, this, &RemoteConnectDialog::doConnect);
    connect(bKey, &QPushButton::clicked, this, [this]() {
        const QString f = QFileDialog::getOpenFileName(this, "SSH Anahtarı", m_key->text());
        if (!f.isEmpty()) m_key->setText(f);
    });
    refreshList();
}

void RemoteConnectDialog::refreshList() {
    const QString keep = m_list->currentText();
    m_list->blockSignals(true);
    m_list->clear();
    m_list->addItems(ConnectionProfiles::names());
    m_list->blockSignals(false);
    const int i = m_list->findText(keep);
    m_list->setCurrentIndex(i >= 0 ? i : 0);
    if (m_list->count() > 0) onSelect(m_list->currentText());
    else newProfile();
}

void RemoteConnectDialog::onSelect(const QString& name) {
    m_current = ConnectionProfiles::get(name);
    loadToUi(m_current);
}

void RemoteConnectDialog::loadToUi(const ConnectionProfile& p) {
    m_name->setText(p.name);
    m_host->setText(p.host);
    m_port->setValue(qBound(1, p.port, 65535));
    m_user->setText(p.user);
    m_key->setText(p.keyPath);
    m_jump->setText(p.jumpHost);
    m_root->setText(p.remoteRoot);
    m_trust->setChecked(p.trustNewHosts);
}

void RemoteConnectDialog::collectFromUi() {
    m_current.name = m_name->text().trimmed();
    m_current.host = m_host->text().trimmed();
    m_current.port = m_port->value();
    m_current.user = m_user->text().trimmed();
    m_current.keyPath = m_key->text().trimmed();
    m_current.jumpHost = m_jump->text().trimmed();
    m_current.remoteRoot = m_root->text().trimmed();
    m_current.trustNewHosts = m_trust->isChecked();
}

void RemoteConnectDialog::saveCurrent() {
    collectFromUi();
    if (!m_current.isValid()) {
        QMessageBox::warning(this, "Profil", "Önce host yazın.");
        return;
    }
    if (m_current.name.isEmpty()) m_current.name = m_current.host;
    if (!ConnectionProfiles::save(m_current)) {
        QMessageBox::warning(this, "Profil", "Kaydedilemedi.");
        return;
    }
    refreshList();
    m_list->setCurrentText(ConnectionProfile::sanitize(m_current.name));
}

void RemoteConnectDialog::deleteCurrent() {
    if (m_list->currentText().isEmpty()) return;
    ConnectionProfiles::remove(m_list->currentText());
    refreshList();
}

void RemoteConnectDialog::newProfile() {
    m_current = ConnectionProfile();
    m_current.port = 22;
    loadToUi(m_current);
    m_host->setFocus();
}

void RemoteConnectDialog::doConnect() {
    collectFromUi();
    if (!m_current.isValid()) {
        QMessageBox::warning(this, "Bağlan", "Önce host yazın.");
        return;
    }
    emit connectRequested(m_current);
    accept();
}
