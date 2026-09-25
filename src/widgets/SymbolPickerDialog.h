#pragma once
#include "../core/SymbolTree.h"
#include <QDialog>

class QLineEdit;
class QListWidget;

// Stage 13: sembol seçici — Ctrl+Shift+O (belge) ve Ctrl+T (çalışma alanı) ortak.
class SymbolPickerDialog : public QDialog {
    Q_OBJECT
public:
    explicit SymbolPickerDialog(const QString& title, const QList<SymbolNode>& roots,
                                QWidget* parent = nullptr);
    SymbolNode selected() const { return m_sel; }

private slots:
    void onFilter(const QString& text);
    void onAccept();

private:
    QList<SymbolNode> m_flat;
    QList<SymbolNode> m_shown; // filtreli görünüm (satır → sembol)
    SymbolNode m_sel;
    QLineEdit* m_filter;
    QListWidget* m_list;
};
