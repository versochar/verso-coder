#pragma once
#include "../core/SymbolTree.h"
#include <QTreeWidget>
#include <QWidget>

class QLineEdit;

// Stage 17: outline paneli — LSP documentSymbol ağacı + regex yedeği.
class OutlinePanel : public QWidget {
    Q_OBJECT
public:
    explicit OutlinePanel(QWidget* parent = nullptr);

    void setLspSymbols(const QList<SymbolNode>& roots);
    void setFallbackText(const QString& text, const QString& suffix);
    void clear();

signals:
    void symbolClicked(int line0); // 0-based satır
    void refreshRequested();       // LSP'den tazele isteği

private slots:
    void onItemClicked(QTreeWidgetItem* it, int col);
    void onFilterChanged(const QString& t);

private:
    void fill(const QList<SymbolNode>& roots, const QString& filter);
    void addNodes(QTreeWidgetItem* parent, const QList<SymbolNode>& nodes,
                  const QString& filter);
    static QString iconFor(int kind);

    QTreeWidget* m_tree;
    QLineEdit* m_filter;
    QList<SymbolNode> m_roots;
    bool m_fallback = false;
};
