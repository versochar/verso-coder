#include "FirstRunDialog.h"
#include "../core/KeymapPresets.h"
#include "../core/LanguageManager.h"
#include "../core/SettingsManager.h"
#include "../core/ThemeManager.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

FirstRunDialog::FirstRunDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Verso Coder'a Hoş Geldiniz");
    setMinimumWidth(380);
    AppSettings s = SettingsManager::instance().load();
    auto* lay = new QVBoxLayout(this);
    auto* head = new QLabel("Başlamadan önce birkaç tercih:", this);
    head->setObjectName("welcomeSection");
    lay->addWidget(head);
    auto* form = new QFormLayout();
    m_lang = new QComboBox(this);
    m_lang->addItems({"Türkçe", "English"});
    m_lang->setCurrentIndex(s.language == "en" ? 1 : 0);
    form->addRow("Dil:", m_lang);
    m_theme = new QComboBox(this);
    const QStringList themes = ThemeManager::instance().availableThemes();
    m_theme->addItems(themes.isEmpty() ? QStringList{"dark"} : themes);
    const int ti = m_theme->findText(s.theme);
    m_theme->setCurrentIndex(ti < 0 ? 0 : ti);
    form->addRow("Tema:", m_theme);
    m_keys = new QComboBox(this);
    m_keys->addItems(KeymapPresets::names());
    form->addRow("Kısayol profili:", m_keys);
    m_font = new QSpinBox(this);
    m_font->setRange(8, 24);
    m_font->setValue(s.fontSize);
    form->addRow("Editör yazı boyutu:", m_font);
    lay->addLayout(form);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                     this);
    box->button(QDialogButtonBox::Ok)->setText("Başla");
    connect(box, &QDialogButtonBox::accepted, this, &FirstRunDialog::onAccept);
    connect(box, &QDialogButtonBox::rejected, this, &FirstRunDialog::reject);
    lay->addWidget(box);
}

void FirstRunDialog::onAccept() {
    AppSettings s = SettingsManager::instance().load();
    s.language = (m_lang->currentIndex() == 1) ? "en" : "tr";
    s.theme = m_theme->currentText();
    s.fontSize = m_font->value();
    const QMap<QString, QString> p = KeymapPresets::preset(m_keys->currentText());
    for (auto it = p.constBegin(); it != p.constEnd(); ++it)
        s.shortcuts[it.key()] = it.value();
    s.m_firstRun = false;
    SettingsManager::instance().save(s);
    LanguageManager::instance().setLanguage(s.language);
    ThemeManager::instance().apply(s.theme);
    accept();
}
