#include "SymbolSearchDialog.h"
#include "CommandPalette.h"
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>
#include <algorithm>

SymbolSearchDialog::SymbolSearchDialog(const QList<WorkspaceSymbol>& index,
                                       QWidget* parent)
    : QDialog(parent), m_index(index) {
    setWindowTitle("Sembol Ara (Ctrl+T)");
    resize(560, 420);
    auto* lay = new QVBoxLayout(this);
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText("fonksiyon/sınıf adı…");
    m_search->setClearButtonEnabled(true);
    m_list = new QListWidget(this);
    lay->addWidget(m_search);
    lay->addWidget(m_list, 1);
    connect(m_search, &QLineEdit::textChanged, this, &SymbolSearchDialog::refilter);
    connect(m_list, &QListWidget::itemDoubleClicked, this,
            &SymbolSearchDialog::onAccept);
    connect(m_search, &QLineEdit::returnPressed, this, &SymbolSearchDialog::onAccept);
    refilter();
    m_search->setFocus();
}

static QString kindGlyph(const QString& kind) {
    // Stage 27: tür simgeleri
    if (kind == "func") return "ƒ";
    if (kind == "class") return "C";
    return "•";
}

void SymbolSearchDialog::refilter() {
    m_list->clear();
    auto hits = WorkspaceSymbols::query(m_index, m_search->text(), 400);
    // Stage 27: skor sıralaması (önek/kelime-başı bonuslu)
    const QString pat = m_search->text();
    std::sort(hits.begin(), hits.end(), [&](const WorkspaceSymbol& a,
                                            const WorkspaceSymbol& b) {
        return CommandPalette::matchScore(pat, a.name) >
               CommandPalette::matchScore(pat, b.name);
    });
    if (hits.size() > 120) hits = hits.mid(0, 120);
    for (const WorkspaceSymbol& s : hits) {
        auto* it = new QListWidgetItem(
            QString("%1 %2  %3:%4").arg(kindGlyph(s.kind), s.name,
                                        QFileInfo(s.file).fileName()).arg(s.line),
            m_list);
        it->setData(Qt::UserRole, QVariant::fromValue<int>(m_list->count()));
        it->setData(Qt::UserRole + 1, s.file);
        it->setData(Qt::UserRole + 2, s.line);
        it->setData(Qt::UserRole + 3, s.name);
        it->setData(Qt::UserRole + 4, s.kind);
    }
    if (m_list->count() > 0) m_list->setCurrentRow(0);
}

void SymbolSearchDialog::onAccept() {
    auto* it = m_list->currentItem();
    if (!it) return;
    m_sel.file = it->data(Qt::UserRole + 1).toString();
    m_sel.line = it->data(Qt::UserRole + 2).toInt();
    m_sel.name = it->data(Qt::UserRole + 3).toString();
    m_sel.kind = it->data(Qt::UserRole + 4).toString();
    m_ok = true;
    accept();
}
