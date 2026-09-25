#include "PromptLibraryDialog.h"
#include "../core/PromptLibrary.h"
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

PromptLibraryDialog::PromptLibraryDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("İstem Kitaplığı");
    resize(440, 380);
    auto* lay = new QVBoxLayout(this);
    m_list = new QListWidget(this);
    lay->addWidget(m_list, 1);
    auto* row = new QHBoxLayout();
    auto* bRun = new QPushButton("Çalıştır", this);
    auto* bAdd = new QPushButton("Yeni...", this);
    auto* bDel = new QPushButton("Sil", this);
    row->addWidget(bRun);
    row->addWidget(bAdd);
    row->addWidget(bDel);
    row->addStretch(1);
    lay->addLayout(row);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    lay->addWidget(box);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(bRun, &QPushButton::clicked, this, &PromptLibraryDialog::runSelected);
    connect(bAdd, &QPushButton::clicked, this, &PromptLibraryDialog::addNew);
    connect(bDel, &QPushButton::clicked, this, &PromptLibraryDialog::removeSelected);
    connect(m_list, &QListWidget::itemDoubleClicked, this,
            &PromptLibraryDialog::runSelected);
    refresh();
}

void PromptLibraryDialog::refresh() {
    m_list->clear();
    for (const QString& n : PromptLibrary::names()) {
        auto* it = new QListWidgetItem(n, m_list);
        it->setToolTip(PromptLibrary::all().value(n).left(200));
    }
}

void PromptLibraryDialog::addNew() {
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Yeni İstem", "Ad:", QLineEdit::Normal,
                                              QString(), &ok);
    if (!ok) return;
    const QString prompt = QInputDialog::getMultiLineText(this, "Yeni İstem",
        "İstem metni (seçim/dosya bağlamı otomatik eklenir):", QString(), &ok);
    if (!ok) return;
    if (!PromptLibrary::save(name, prompt)) return;
    refresh();
}

void PromptLibraryDialog::removeSelected() {
    auto* it = m_list->currentItem();
    if (!it) return;
    PromptLibrary::remove(it->text());
    refresh();
}

void PromptLibraryDialog::runSelected() {
    auto* it = m_list->currentItem();
    if (!it) return;
    const QString prompt = PromptLibrary::all().value(it->text());
    if (!prompt.isEmpty()) emit runRequested(it->text(), prompt);
}
