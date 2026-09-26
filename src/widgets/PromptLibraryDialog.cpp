#include "PromptLibraryDialog.h"
#include "../core/PromptLibrary.h"
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

PromptLibraryDialog::PromptLibraryDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("İstem Galerisi");
    resize(760, 480);
    auto* lay = new QVBoxLayout(this);

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText("Ara...");
    m_search->setClearButtonEnabled(true);
    lay->addWidget(m_search);

    auto* split = new QSplitter(this);
    split->setOrientation(Qt::Horizontal);
    m_list = new QListWidget(split);
    m_list->setMinimumWidth(200);

    auto* right = new QWidget(split);
    auto* rl = new QVBoxLayout(right);
    rl->setContentsMargins(0, 0, 0, 0);
    m_name = new QLineEdit(right);
    m_name->setPlaceholderText("İstem adı");
    m_text = new QPlainTextEdit(right);
    m_text->setPlaceholderText("İstem metni — {{selection}}, {{file}}, {{filename}}, "
                               "{{date}}, {{lang}} değişkenlerini kullanabilirsiniz.");
    rl->addWidget(m_name);
    rl->addWidget(m_text, 1);
    split->addWidget(m_list);
    split->addWidget(right);
    split->setStretchFactor(1, 1);
    lay->addWidget(split, 1);

    m_hint = new QLabel("Çift tık: kullan · Değişkenler çalıştırma anında genişletilir.", this);
    m_hint->setStyleSheet("color:#858585;font-size:11px;");
    lay->addWidget(m_hint);

    auto* row = new QHBoxLayout();
    auto* bRun = new QPushButton("Kullan", this);
    auto* bNew = new QPushButton("Yeni", this);
    auto* bSave = new QPushButton("Kaydet", this);
    auto* bDel = new QPushButton("Sil", this);
    auto* bImport = new QPushButton("İçe Aktar", this);
    auto* bExport = new QPushButton("Dışa Aktar", this);
    row->addWidget(bRun);
    row->addWidget(bNew);
    row->addWidget(bSave);
    row->addWidget(bDel);
    row->addStretch(1);
    row->addWidget(bImport);
    row->addWidget(bExport);
    lay->addLayout(row);

    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    lay->addWidget(box);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(bRun, &QPushButton::clicked, this, &PromptLibraryDialog::runSelected);
    connect(bNew, &QPushButton::clicked, this, &PromptLibraryDialog::addNew);
    connect(bSave, &QPushButton::clicked, this, &PromptLibraryDialog::saveEdits);
    connect(bDel, &QPushButton::clicked, this, &PromptLibraryDialog::removeSelected);
    connect(bImport, &QPushButton::clicked, this, &PromptLibraryDialog::importJson);
    connect(bExport, &QPushButton::clicked, this, &PromptLibraryDialog::exportJson);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &PromptLibraryDialog::runSelected);
    connect(m_list, &QListWidget::currentTextChanged, this, [this](const QString& t) {
        const QString body = PromptLibrary::all().value(t);
        m_name->setText(t);
        m_text->setPlainText(body);
    });
    connect(m_search, &QLineEdit::textChanged, this, [this]() { refresh(); });
    refresh();
}

QString PromptLibraryDialog::currentName() const {
    auto* it = m_list->currentItem();
    return it ? it->text() : QString();
}

void PromptLibraryDialog::refresh() {
    const QString keep = currentName();
    const QString q = m_search->text().trimmed().toLower();
    m_list->clear();
    for (const QString& n : PromptLibrary::names()) {
        const QString body = PromptLibrary::all().value(n);
        if (!q.isEmpty() && !n.toLower().contains(q) && !body.toLower().contains(q)) continue;
        auto* it = new QListWidgetItem(n, m_list);
        it->setToolTip(body.left(200));
    }
    if (!keep.isEmpty()) selectItem(keep);
}

void PromptLibraryDialog::selectItem(const QString& name) {
    for (int i = 0; i < m_list->count(); ++i)
        if (m_list->item(i)->text() == name) {
            m_list->setCurrentRow(i);
            return;
        }
}

void PromptLibraryDialog::addNew() {
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Yeni İstem", "Ad:", QLineEdit::Normal,
                                              QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    PromptLibrary::save(name, "Yeni istem gövdesi...");
    refresh();
    selectItem(PromptLibrary::sanitize(name));
}

void PromptLibraryDialog::saveEdits() {
    const QString name = m_name->text().trimmed();
    if (name.isEmpty() || m_text->toPlainText().trimmed().isEmpty()) return;
    const QString old = currentName();
    PromptLibrary::save(name, m_text->toPlainText());
    if (!old.isEmpty() && old != PromptLibrary::sanitize(name)) PromptLibrary::remove(old);
    refresh();
    selectItem(PromptLibrary::sanitize(name));
    m_hint->setText("Kaydedildi: " + PromptLibrary::sanitize(name));
}

void PromptLibraryDialog::removeSelected() {
    const QString n = currentName();
    if (n.isEmpty()) return;
    PromptLibrary::remove(n);
    m_name->clear();
    m_text->clear();
    refresh();
}

void PromptLibraryDialog::runSelected() {
    const QString n = currentName();
    if (n.isEmpty()) return;
    const QString raw = m_text->toPlainText();
    const QString prompt = PromptLibrary::stripTaskHint(raw);
    if (prompt.isEmpty()) return;
    emit runRequested(n, prompt, PromptLibrary::taskHint(raw));
}

void PromptLibraryDialog::importJson() {
    const QString path = QFileDialog::getOpenFileName(this, "İstemleri İçe Aktar", QString(),
                                                      "JSON (*.json)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
    int n = 0;
    for (auto it = o.begin(); it != o.end(); ++it)
        if (PromptLibrary::save(it.key(), it.value().toString())) ++n;
    refresh();
    m_hint->setText(QString("%1 istem içe aktarıldı.").arg(n));
}

void PromptLibraryDialog::exportJson() {
    const QString path = QFileDialog::getSaveFileName(this, "İstemleri Dışa Aktar", "istemler.json",
                                                      "JSON (*.json)");
    if (path.isEmpty()) return;
    QJsonObject o;
    const auto all = PromptLibrary::all();
    for (auto it = all.begin(); it != all.end(); ++it) o[it.key()] = it.value();
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    m_hint->setText(QString("%1 istem dışa aktarıldı.").arg(all.size()));
}
