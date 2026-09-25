#include "SymbolPickerDialog.h"
#include <QFileInfo>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

SymbolPickerDialog::SymbolPickerDialog(const QString& title, const QList<SymbolNode>& roots,
                                       QWidget* parent)
    : QDialog(parent), m_flat(SymbolTree::flatten(roots)) {
    setWindowTitle(title);
    resize(480, 420);
    auto* lay = new QVBoxLayout(this);
    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText("Sembol ara...");
    m_filter->setClearButtonEnabled(true);
    lay->addWidget(m_filter);
    m_list = new QListWidget(this);
    lay->addWidget(m_list, 1);
    connect(m_filter, &QLineEdit::textChanged, this, &SymbolPickerDialog::onFilter);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &SymbolPickerDialog::onAccept);
    connect(m_list, &QListWidget::itemActivated, this, &SymbolPickerDialog::onAccept);
    onFilter(QString());
    m_filter->setFocus();
}

void SymbolPickerDialog::onFilter(const QString& text) {
    m_list->clear();
    m_shown.clear();
    const QString q = text.trimmed().toLower();
    for (const SymbolNode& n : std::as_const(m_flat)) {
        if (!q.isEmpty() && !n.name.toLower().contains(q)
            && !n.container.toLower().contains(q))
            continue;
        QString row = QString("%1  %2").arg(SymbolTree::kindIcon(n.kind), n.name);
        if (!n.container.isEmpty()) row += "  (" + n.container + ")";
        if (!n.path.isEmpty()) row += "  —  " + QFileInfo(n.path).fileName();
        row += QString("  :%1").arg(n.line + 1);
        auto* it = new QListWidgetItem(row, m_list);
        it->setData(Qt::UserRole, m_shown.size());
        m_shown << n;
        if (m_shown.size() >= 300) break;
    }
    if (m_list->count() > 0) m_list->setCurrentRow(0);
}

void SymbolPickerDialog::onAccept() {
    int r = m_list->currentRow();
    QListWidgetItem* it = m_list->currentItem();
    int idx = it ? it->data(Qt::UserRole).toInt() : r;
    if (idx >= 0 && idx < m_shown.size()) m_sel = m_shown[idx];
    else if (!m_shown.isEmpty()) m_sel = m_shown.first();
    accept();
}
