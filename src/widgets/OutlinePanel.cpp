#include "OutlinePanel.h"
#include "../core/OutlineFallback.h"
#include <QHeaderView>
#include <QLineEdit>
#include <QToolBar>
#include <QVBoxLayout>

OutlinePanel::OutlinePanel(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    auto* bar = new QToolBar(this);
    bar->addAction("Tazele", this, &OutlinePanel::refreshRequested);
    lay->addWidget(bar);
    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText("Sembol süz...");
    m_filter->setClearButtonEnabled(true);
    lay->addWidget(m_filter);
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderHidden(true);
    m_tree->setColumnCount(1);
    lay->addWidget(m_tree, 1);
    connect(m_tree, &QTreeWidget::itemClicked, this, &OutlinePanel::onItemClicked);
    connect(m_filter, &QLineEdit::textChanged, this, &OutlinePanel::onFilterChanged);
}

QString OutlinePanel::iconFor(int kind) {
    // LSP SymbolKind → metin rozeti (ikon teması yok, hafif)
    switch (kind) {
    case 5: return "Ⓒ";  // Class
    case 6: return "Ⓜ";  // Method
    case 12: return "Ⓕ"; // Function
    case 13: return "ⓥ"; // Variable
    case 11: return "Ⓘ"; // Interface
    case 10: return "Ⓔ"; // Enum
    case 4: return "Ⓕ";  // Field
    case 3: return "Ⓝ";  // Namespace
    default: return "•";
    }
}

void OutlinePanel::setLspSymbols(const QList<SymbolNode>& roots) {
    m_roots = roots;
    m_fallback = false;
    fill(roots, m_filter->text());
}

void OutlinePanel::setFallbackText(const QString& text, const QString& suffix) {
    const auto items = OutlineFallback::scan(text, suffix);
    QList<SymbolNode> roots;
    for (const OutlineItem& it : items) {
        SymbolNode n;
        n.name = it.name;
        n.line = it.line0;
        n.kind = (it.kind == "class") ? 5 : (it.kind == "method" ? 6 : 12);
        n.detail = it.kind;
        roots << n;
    }
    m_roots = roots;
    m_fallback = true;
    fill(roots, m_filter->text());
}

void OutlinePanel::clear() {
    m_roots.clear();
    m_tree->clear();
}

void OutlinePanel::fill(const QList<SymbolNode>& roots, const QString& filter) {
    m_tree->clear();
    addNodes(nullptr, roots, filter.trimmed().toLower());
    m_tree->expandAll();
}

void OutlinePanel::addNodes(QTreeWidgetItem* parent, const QList<SymbolNode>& nodes,
                            const QString& filter) {
    for (const SymbolNode& n : nodes) {
        const bool selfOk = filter.isEmpty() || n.name.toLower().contains(filter);
        // Filtrede eşleşen çocuk varsa ebeveyni de göster
        QList<SymbolNode> kids = n.children;
        if (!filter.isEmpty()) {
            // Çocuk eşleşmesi kontrolü: düzleştirilmiş alt ağaçta ara
            bool kidOk = false;
            std::function<void(const QList<SymbolNode>&)> scan =
                [&](const QList<SymbolNode>& ns) {
                    for (const SymbolNode& k : ns) {
                        if (k.name.toLower().contains(filter)) { kidOk = true; return; }
                        scan(k.children);
                    }
                };
            scan(n.children);
            if (!selfOk && !kidOk) continue;
        }
        auto* it = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(m_tree);
        QString label = iconFor(n.kind) + " " + n.name;
        if (!n.detail.isEmpty() && n.detail != n.name)
            label += " — " + n.detail.left(60);
        it->setText(0, label);
        it->setData(0, Qt::UserRole, n.line);
        it->setToolTip(0, QString("Satır %1").arg(n.line + 1));
        addNodes(it, kids, filter);
        if (!filter.isEmpty()) it->setExpanded(true);
    }
}

void OutlinePanel::onItemClicked(QTreeWidgetItem* it, int col) {
    Q_UNUSED(col);
    if (!it) return;
    emit symbolClicked(it->data(0, Qt::UserRole).toInt());
}

void OutlinePanel::onFilterChanged(const QString& t) {
    fill(m_roots, t);
}
