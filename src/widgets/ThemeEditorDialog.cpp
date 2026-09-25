#include "ThemeEditorDialog.h"
#include "../core/OllamaClient.h"
#include "../core/SettingsManager.h"
#include "../core/ThemeManager.h"
#include "../core/ThemeStore.h"
#include "../core/ThemeValidator.h"
#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>

static const QPair<QString, QString> kFields[] = {
    {"bg", "Arka plan"}, {"surface", "Yüzey"}, {"surfaceAlt", "Yüzey (alt)"},
    {"border", "Kenarlık"}, {"text", "Metin"}, {"textStrong", "Metin (güçlü)"},
    {"textDim", "Metin (soluk)"}, {"accent", "Vurgu"}, {"success", "Başarı"},
    {"warning", "Uyarı"}, {"error", "Hata"}, {"selection", "Seçim"},
    {"lineHighlight", "Aktif satır"}, {"gutterBg", "Gutter zemin"},
    {"gutterText", "Gutter metin"}, {"gutterActive", "Gutter (aktif)"},
    {"indentGuide", "Girinti kılavuzu"}, {"bracket", "Parantez"},
    {"cursor", "İmleç"}, {"scrollbar", "Kaydırma çubuğu"},
    {"synKeyword", "Anahtar kelime"}, {"synString", "Dize"}, {"synComment", "Yorum"},
    {"synNumber", "Sayı"}, {"synFunc", "Fonksiyon"}, {"synType", "Tür"},
};

ThemeEditorDialog::ThemeEditorDialog(const QString& baseTheme, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Tema Düzenleyici");
    resize(560, 640);

    m_tokens = ThemeStore::instance().theme(baseTheme);
    m_tokens.name = baseTheme + "-kopya";

    auto* lay = new QVBoxLayout(this);
    auto* tabs = new QTabWidget(this);

    // --- Renkler sekmesi ---
    auto* colors = new QWidget(this);
    auto* clay = new QVBoxLayout(colors);
    auto* top = new QFormLayout();
    m_name = new QLineEdit(m_tokens.name, colors);
    m_name->setPlaceholderText("tema-adı (küçük harf, tire)");
    connect(m_name, &QLineEdit::textChanged, this, &ThemeEditorDialog::onNameChanged);
    top->addRow("Tema adı:", m_name);
    m_dark = new QCheckBox("Koyu tema", colors);
    m_dark->setChecked(m_tokens.dark);
    connect(m_dark, &QCheckBox::toggled, this, &ThemeEditorDialog::onDarkToggled);
    top->addRow(m_dark);
    clay->addLayout(top);

    auto* scroll = new QScrollArea(colors);
    scroll->setWidgetResizable(true);
    auto* host = new QWidget(scroll);
    buildColorRows(host);
    scroll->setWidget(host);
    clay->addWidget(scroll, 1);
    tabs->addTab(colors, "Renkler");

    // --- AI sekmesi ---
    auto* ai = new QWidget(this);
    auto* alay = new QVBoxLayout(ai);
    auto* info = new QLabel(
        "Ollama'ya tarifinizi yazın (örn. \"gece mavisi, yumuşak vurgulu\");\n"
        "üretim doğrulanır, önizlenir ve düzenleyiciye yüklenir.", ai);
    info->setWordWrap(true);
    alay->addWidget(info);
    m_prompt = new QLineEdit(ai);
    m_prompt->setPlaceholderText("Tema tarifi...");
    alay->addWidget(m_prompt);
    auto* mrow = new QHBoxLayout();
    m_model = new QComboBox(ai);
    m_model->setEditable(true);
    mrow->addWidget(m_model, 1);
    auto* bModels = new QPushButton("Modelleri Yenile", ai);
    connect(bModels, &QPushButton::clicked, this, &ThemeEditorDialog::refreshModels);
    mrow->addWidget(bModels);
    alay->addLayout(mrow);
    m_generate = new QPushButton("AI ile Üret", ai);
    connect(m_generate, &QPushButton::clicked, this, &ThemeEditorDialog::generateWithAi);
    alay->addWidget(m_generate);
    m_aiStatus = new QLabel(ai);
    m_aiStatus->setWordWrap(true);
    alay->addWidget(m_aiStatus);
    alay->addStretch(1);
    tabs->addTab(ai, "AI ile Üret");
    lay->addWidget(tabs, 1);

    auto* buttons = new QHBoxLayout();
    auto* bSave = new QPushButton("Kaydet", this);
    bSave->setDefault(true);
    auto* bCancel = new QPushButton("Vazgeç", this);
    buttons->addStretch(1);
    buttons->addWidget(bCancel);
    buttons->addWidget(bSave);
    lay->addLayout(buttons);
    connect(bSave, &QPushButton::clicked, this, &ThemeEditorDialog::saveTheme);
    connect(bCancel, &QPushButton::clicked, this, [this]() {
        ThemeManager::instance().clearPreview();
        reject();
    });

    // Model listesi + istemci
    m_client = new OllamaClient(this);
    AppSettings s = SettingsManager::instance().load();
    m_client->setHost(s.ollamaHost);
    m_model->addItem(s.ollamaModel);
    connect(m_client, &OllamaClient::modelsReady, this, [this](const QStringList& ms) {
        m_model->clear();
        m_model->addItems(ms);
        m_aiStatus->setText(QString("%1 model bulundu.").arg(ms.size()));
    });

    preview();
}

void ThemeEditorDialog::buildColorRows(QWidget* parent) {
    auto* form = qobject_cast<QFormLayout*>(parent->layout());
    if (!form) {
        form = new QFormLayout(parent);
    }
    for (const auto& f : kFields) addColorRow(form, f.first, f.second);
}

void ThemeEditorDialog::addColorRow(QFormLayout* form, const QString& key,
                                    const QString& label) {
    auto* b = new QPushButton(this);
    b->setFixedHeight(26);
    b->setCursor(Qt::PointingHandCursor);
    b->setToolTip("Değiştirmek için tıkla");
    connect(b, &QPushButton::clicked, this, [this, key]() { pickColor(key); });
    m_buttons[key] = b;
    form->addRow(label + ":", b);
    syncButtons();
}

QColor ThemeEditorDialog::tokenColor(const QString& key) const {
    const ThemeTokens& t = m_tokens;
    if (key == "bg") return t.bg;
    if (key == "surface") return t.surface;
    if (key == "surfaceAlt") return t.surfaceAlt;
    if (key == "border") return t.border;
    if (key == "text") return t.text;
    if (key == "textStrong") return t.textStrong;
    if (key == "textDim") return t.textDim;
    if (key == "accent") return t.accent;
    if (key == "success") return t.success;
    if (key == "warning") return t.warning;
    if (key == "error") return t.error;
    if (key == "selection") return t.selection;
    if (key == "lineHighlight") return t.lineHighlight;
    if (key == "gutterBg") return t.gutterBg;
    if (key == "gutterText") return t.gutterText;
    if (key == "gutterActive") return t.gutterActive;
    if (key == "indentGuide") return t.indentGuide;
    if (key == "bracket") return t.bracket;
    if (key == "cursor") return t.cursor;
    if (key == "scrollbar") return t.scrollbar;
    if (key == "synKeyword") return t.synKeyword;
    if (key == "synString") return t.synString;
    if (key == "synComment") return t.synComment;
    if (key == "synNumber") return t.synNumber;
    if (key == "synFunc") return t.synFunc;
    if (key == "synType") return t.synType;
    return QColor();
}

void ThemeEditorDialog::setTokenColor(const QString& key, const QColor& c) {
    ThemeTokens& t = m_tokens;
    if (key == "bg") t.bg = c;
    else if (key == "surface") t.surface = c;
    else if (key == "surfaceAlt") t.surfaceAlt = c;
    else if (key == "border") t.border = c;
    else if (key == "text") t.text = c;
    else if (key == "textStrong") t.textStrong = c;
    else if (key == "textDim") t.textDim = c;
    else if (key == "accent") t.accent = c;
    else if (key == "success") t.success = c;
    else if (key == "warning") t.warning = c;
    else if (key == "error") t.error = c;
    else if (key == "selection") t.selection = c;
    else if (key == "lineHighlight") t.lineHighlight = c;
    else if (key == "gutterBg") t.gutterBg = c;
    else if (key == "gutterText") t.gutterText = c;
    else if (key == "gutterActive") t.gutterActive = c;
    else if (key == "indentGuide") t.indentGuide = c;
    else if (key == "bracket") t.bracket = c;
    else if (key == "cursor") t.cursor = c;
    else if (key == "scrollbar") t.scrollbar = c;
    else if (key == "synKeyword") t.synKeyword = c;
    else if (key == "synString") t.synString = c;
    else if (key == "synComment") t.synComment = c;
    else if (key == "synNumber") t.synNumber = c;
    else if (key == "synFunc") t.synFunc = c;
    else if (key == "synType") t.synType = c;
    else return;
}

void ThemeEditorDialog::syncButtons() {
    for (auto it = m_buttons.begin(); it != m_buttons.end(); ++it) {
        const QColor c = tokenColor(it.key());
        it.value()->setText(c.name());
        it.value()->setStyleSheet(
            QString("text-align:left;padding:2px 8px;border:1px solid %1;"
                    "border-left:10px solid %1;background:transparent;")
                .arg(c.name()));
    }
}

void ThemeEditorDialog::preview() const {
    ThemeTokens tk = m_tokens;
    ThemeManager::instance().previewTokens(tk);
}

void ThemeEditorDialog::pickColor(const QString& key) {
    const QColor c = QColorDialog::getColor(tokenColor(key), this, "Renk seç");
    if (!c.isValid()) return;
    setTokenColor(key, c);
    syncButtons();
    preview();
}

void ThemeEditorDialog::onNameChanged() {
    m_tokens.name = m_name->text().trimmed();
    preview();
}

void ThemeEditorDialog::onDarkToggled(bool on) {
    m_tokens.dark = on;
    preview();
}

void ThemeEditorDialog::loadTokens(const ThemeTokens& tk) {
    m_tokens = tk;
    m_name->setText(tk.name);
    m_dark->setChecked(tk.dark);
    syncButtons();
    preview();
}

void ThemeEditorDialog::saveTheme() {
    QString name = m_name->text().trimmed().toLower();
    name.replace(' ', '-');
    QString clean;
    for (QChar ch : name)
        if (ch.isLetterOrNumber() || ch == '-' || ch == '_') clean += ch;
    if (clean.isEmpty()) {
        QMessageBox::warning(this, "Tema", "Geçerli bir tema adı yazın.");
        return;
    }
    m_tokens.name = clean;
    const QString json = m_tokens.toJsonString();
    QString err;
    if (!ThemeValidator::validate(json, &err)) {
        QMessageBox::warning(this, "Tema", "Tema geçersiz:\n" + err);
        return;
    }
    QDir().mkpath(ThemeStore::customThemesDir());
    const QString path = ThemeStore::customThemesDir() + "/" + clean + ".json";
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::warning(this, "Tema", "Dosya yazılamadı:\n" + path);
        return;
    }
    f.write(json.toUtf8());
    f.close();
    ThemeManager::instance().clearPreview();
    m_saved = clean;
    accept();
}

void ThemeEditorDialog::refreshModels() {
    m_aiStatus->setText("Modeller alınıyor...");
    m_client->fetchModels();
}

QString ThemeEditorDialog::aiSystemPrompt() const {
    return "Sen bir kod editörü tema tasarımcısısın. SADECE geçerli bir JSON nesnesi döndür, "
           "başka hiçbir metin yazma. Şema: {\"name\": \"kısa-ad\", \"dark\": true/false, "
           "\"colors\": {\"bg\", \"surface\", \"surfaceAlt\", \"border\", \"text\", "
           "\"textStrong\", \"textDim\", \"accent\", \"success\", \"warning\", \"error\", "
           "\"selection\", \"lineHighlight\", \"gutterBg\", \"gutterText\", \"gutterActive\", "
           "\"indentGuide\", \"bracket\", \"cursor\", \"scrollbar\", "
           "\"syntax\": {\"keyword\", \"string\", \"comment\", \"number\", \"func\", \"type\"}}} "
           "Tüm renkler \"#rrggbb\" formatında olmalı. Okunabilir kontrastı koru.";
}

void ThemeEditorDialog::generateWithAi() {
    const QString desc = m_prompt->text().trimmed();
    if (desc.isEmpty()) {
        m_aiStatus->setText("Önce bir tarif yazın.");
        return;
    }
    m_generate->setEnabled(false);
    m_aiStatus->setText("Üretiliyor... (bu biraz sürebilir)");
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();
    QString err;
    const QString reply = m_client->chatSync(
        m_model->currentText().trimmed(), aiSystemPrompt(),
        "Şu tarifte bir editör teması üret: " + desc,
        QJsonObject{{"temperature", 0.7}}, err, 120000);
    QApplication::restoreOverrideCursor();
    m_generate->setEnabled(true);
    if (!err.isEmpty()) {
        m_aiStatus->setText("Hata: " + err);
        return;
    }
    const QString json = ThemeValidator::extractJson(reply);
    QString verr;
    if (!ThemeValidator::validate(json, &verr)) {
        m_aiStatus->setText("Üretim geçersiz (" + verr + "). Farklı tarif deneyin.");
        return;
    }
    QString perr;
    ThemeTokens tk = ThemeTokens::fromJson(json, &perr);
    if (!perr.isEmpty()) {
        m_aiStatus->setText("Ayrıştırma hatası: " + perr);
        return;
    }
    loadTokens(tk);
    m_aiStatus->setText("Üretildi ve önizleniyor — Renkler sekmesinden ince ayar yapın.");
}
