#include "SnippetDialog.h"
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

SnippetDialog::SnippetDialog(const QString& suffix, QWidget* parent)
    : QDialog(parent), m_suffix(suffix) {
    setWindowTitle("Snippet'ler");
    resize(480, 420);
    auto* lay = new QVBoxLayout(this);
    m_list = new QListWidget(this);
    lay->addWidget(m_list, 1);
    auto* row = new QHBoxLayout();
    auto* bIns = new QPushButton("Ekle", this);
    auto* bNew = new QPushButton("Yeni...", this);
    auto* bDel = new QPushButton("Sil", this);
    bIns->setDefault(true);
    row->addWidget(bIns);
    row->addWidget(bNew);
    row->addWidget(bDel);
    row->addStretch(1);
    lay->addLayout(row);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    lay->addWidget(box);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(bIns, &QPushButton::clicked, this, &SnippetDialog::insertSelected);
    connect(bNew, &QPushButton::clicked, this, &SnippetDialog::addCustom);
    connect(bDel, &QPushButton::clicked, this, &SnippetDialog::removeSelected);
    connect(m_list, &QListWidget::itemDoubleClicked, this,
            &SnippetDialog::insertSelected);
    refresh();
}

void SnippetDialog::refresh() {
    m_list->clear();
    m_items = SnippetManager::forLang(m_suffix);
    for (const SnippetDef& s : m_items) {
        auto* it = new QListWidgetItem(
            QString("%1 — %2 [%3]").arg(s.prefix, s.name, s.lang), m_list);
        it->setToolTip(s.body.left(300));
    }
}

void SnippetDialog::addCustom() {
    bool ok = false;
    const QString prefix = QInputDialog::getText(this, "Yeni Snippet", "Önek:",
                                                 QLineEdit::Normal, QString(), &ok);
    if (!ok || prefix.trimmed().isEmpty()) return;
    const QString body = QInputDialog::getMultiLineText(
        this, "Yeni Snippet",
        "Gövde ($1 ${2:varsayılan} $0, $TM_FILENAME ...):", QString(), &ok);
    if (!ok || body.isEmpty()) return;
    SnippetDef s{prefix.trimmed(), prefix.trimmed(), body,
                 SnippetManager::langFor(m_suffix)};
    if (!SnippetManager::saveCustom(s)) return;
    refresh();
}

void SnippetDialog::removeSelected() {
    const int r = m_list->currentRow();
    if (r < 0 || r >= m_items.size()) return;
    const SnippetDef& s = m_items[r];
    // Yalnızca özel olanlar silinebilir
    if (!SnippetManager::removeCustom(s.lang, s.prefix)) return;
    refresh();
}

void SnippetDialog::insertSelected() {
    const int r = m_list->currentRow();
    if (r < 0 || r >= m_items.size()) return;
    emit insertRequested(m_items[r]);
}
