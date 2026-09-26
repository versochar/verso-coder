#pragma once
#include <QDialog>

#include "../core/WorkspaceSymbols.h"

class QLineEdit;
class QListWidget;

// Stage 24: çalışma alanı sembol araması (Ctrl+T) — adla bul, atla.
class SymbolSearchDialog : public QDialog {
    Q_OBJECT
public:
    explicit SymbolSearchDialog(const QList<WorkspaceSymbol>& index,
                                QWidget* parent = nullptr);
    WorkspaceSymbol selected() const { return m_sel; }
    bool hasSelection() const { return m_ok; }

private slots:
    void refilter();
    void onAccept();

private:
    QLineEdit* m_search = nullptr;
    QListWidget* m_list = nullptr;
    QList<WorkspaceSymbol> m_index;
    WorkspaceSymbol m_sel;
    bool m_ok = false;
};
