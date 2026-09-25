#pragma once
#include "../core/CallHierarchyTree.h"
#include <QDialog>

class QTreeWidget;

// Stage 13: çağrı hiyerarşisi — gelen/giden çağrılar ağacı.
class CallHierarchyDialog : public QDialog {
    Q_OBJECT
public:
    explicit CallHierarchyDialog(const QString& symbol, QWidget* parent = nullptr);
    void setIncoming(const QList<CallNode>& nodes);
    void setOutgoing(const QList<CallNode>& nodes);
    CallNode selected() const { return m_sel; }

signals:
    void jumpRequested(const CallNode& node);
    void expandIncoming(const CallNode& node, class QTreeWidgetItem* item);
    void expandOutgoing(const CallNode& node, class QTreeWidgetItem* item);

private slots:
    void onJump();

private:
    void fillTop(QTreeWidgetItem* top, const QList<CallNode>& nodes);
    CallNode m_sel;
    QTreeWidget* m_tree;
    QTreeWidgetItem* m_inTop;
    QTreeWidgetItem* m_outTop;
};
