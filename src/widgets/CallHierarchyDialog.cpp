#include "CallHierarchyDialog.h"
#include <QFileInfo>
#include <QTreeWidget>
#include <QVBoxLayout>

CallHierarchyDialog::CallHierarchyDialog(const QString& symbol, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(QString("Çağrı Hiyerarşisi: %1").arg(symbol));
    resize(560, 440);
    auto* lay = new QVBoxLayout(this);
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabels({"Çağrılar"});
    lay->addWidget(m_tree, 1);
    m_inTop = new QTreeWidgetItem(m_tree, QStringList("Gelen çağrılar (kim çağırıyor)"));
    m_outTop = new QTreeWidgetItem(m_tree, QStringList("Giden çağrılar (neyi çağırıyor)"));
    m_inTop->setExpanded(true);
    m_outTop->setExpanded(true);
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, &CallHierarchyDialog::onJump);
    connect(m_tree, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem* it) {
        if (it == m_inTop || it == m_outTop) return;
        // Yalnızca "…" yer tutuculu düğümler genişletilir (Stage 24: derinlik takibi)
        if (it->childCount() != 1 || it->child(0)->text(0) != "…") return;
        CallNode n;
        n.name = it->data(0, Qt::UserRole).toString();
        n.uri = it->data(0, Qt::UserRole + 1).toString();
        n.line = it->data(0, Qt::UserRole + 2).toInt();
        // Stage 24: derinlik sınırı (4) + döngü koruması (ata zincirinde aynı düğüm)
        const int depth = it->data(0, Qt::UserRole + 4).toInt();
        if (depth >= 4) {
            it->addChild(new QTreeWidgetItem(it, QStringList("(derinlik sınırı)")));
            return;
        }
        for (QTreeWidgetItem* a = it->parent(); a; a = a->parent()) {
            if (a->data(0, Qt::UserRole + 1).toString() == n.uri &&
                a->data(0, Qt::UserRole).toString() == n.name) {
                it->addChild(new QTreeWidgetItem(it, QStringList("(döngü)")));
                return;
            }
        }
        if (it->parent() == m_inTop) emit expandIncoming(n, it);
        else emit expandOutgoing(n, it);
    });
}

static void addNodes(QTreeWidgetItem* top, const QList<CallNode>& nodes, int depth = 1) {
    for (const CallNode& n : nodes) {
        auto* it = new QTreeWidgetItem(top);
        QString label = n.name;
        if (!n.rangeSummary.isEmpty()) label += "  (" + n.rangeSummary + ")";
        if (!n.uri.isEmpty()) label += "  —  " + QFileInfo(n.uri).fileName();
        it->setText(0, label);
        it->setData(0, Qt::UserRole, n.name);
        it->setData(0, Qt::UserRole + 1, n.uri);
        it->setData(0, Qt::UserRole + 2, n.line);
        it->setData(0, Qt::UserRole + 3, n.detail);
        it->setData(0, Qt::UserRole + 4, depth); // Stage 24: derinlik
        // Lazy: genişletilince alt seviye yüklenir (boş çocuk işareti)
        it->addChild(new QTreeWidgetItem(it, QStringList("…")));
    }
}

void CallHierarchyDialog::setIncoming(const QList<CallNode>& nodes) {
    qDeleteAll(m_inTop->takeChildren());
    if (nodes.isEmpty())
        m_inTop->addChild(new QTreeWidgetItem(m_inTop, QStringList("(yok)")));
    else addNodes(m_inTop, nodes);
}

void CallHierarchyDialog::setOutgoing(const QList<CallNode>& nodes) {
    qDeleteAll(m_outTop->takeChildren());
    if (nodes.isEmpty())
        m_outTop->addChild(new QTreeWidgetItem(m_outTop, QStringList("(yok)")));
    else addNodes(m_outTop, nodes);
}

void CallHierarchyDialog::fillTop(QTreeWidgetItem* top, const QList<CallNode>& nodes) {
    addNodes(top, nodes);
}

void CallHierarchyDialog::onJump() {
    auto* it = m_tree->currentItem();
    if (!it || it == m_inTop || it == m_outTop) return;
    m_sel.name = it->data(0, Qt::UserRole).toString();
    QString uri = it->data(0, Qt::UserRole + 1).toString();
    m_sel.line = it->data(0, Qt::UserRole + 2).toInt();
    // uri "file://" olabilir ya da düz yol
    m_sel.uri = uri.startsWith("file:") ? uri : uri;
    m_sel.detail = it->data(0, Qt::UserRole + 3).toString();
    if (m_sel.name.isEmpty()) return;
    emit jumpRequested(m_sel);
}
