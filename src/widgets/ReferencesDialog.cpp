#include "ReferencesDialog.h"
#include <QFileInfo>
#include <QTreeWidget>
#include <QVBoxLayout>

ReferencesDialog::ReferencesDialog(const QString& symbol, const QList<LspLocation>& locs,
                                   QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(QString("Referanslar: %1 (%2)").arg(symbol).arg(locs.size()));
    resize(560, 420);
    auto* lay = new QVBoxLayout(this);
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabels({"Dosya / Satır"});
    m_tree->setRootIsDecorated(true);
    lay->addWidget(m_tree, 1);
    for (const QString& f : LocationSet::files(locs)) {
        auto* top = new QTreeWidgetItem(m_tree);
        const auto inFile = LocationSet::forFile(locs, f);
        top->setText(0, QString("%1 (%2)").arg(QFileInfo(f).fileName()).arg(inFile.size()));
        top->setData(0, Qt::UserRole, f);
        for (const LspLocation& l : inFile) {
            auto* ch = new QTreeWidgetItem(top);
            ch->setText(0, QString("Satır %1, sütun %2").arg(l.line + 1).arg(l.col + 1));
            ch->setData(0, Qt::UserRole, QVariant::fromValue<QString>(l.path));
            ch->setData(0, Qt::UserRole + 1, l.line);
            ch->setData(0, Qt::UserRole + 2, l.col);
        }
        top->setExpanded(true);
    }
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, &ReferencesDialog::onJump);
}

void ReferencesDialog::onJump() {
    auto* it = m_tree->currentItem();
    if (!it) return;
    QString path = it->data(0, Qt::UserRole).toString();
    // Üst düğümse ilk çocuğa in
    if (it->childCount() > 0) it = it->child(0);
    path = it->data(0, Qt::UserRole).toString();
    m_sel.path = path;
    m_sel.line = it->data(0, Qt::UserRole + 1).toInt();
    m_sel.col = it->data(0, Qt::UserRole + 2).toInt();
    if (m_sel.path.isEmpty()) return;
    accept();
}
