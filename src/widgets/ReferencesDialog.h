#pragma once
#include "../core/LocationSet.h"
#include <QDialog>

class QTreeWidget;
class QTextEdit;

// Stage 13: referanslar — dosyaya göre gruplu ağaç, çift tıkla satıra git.
class ReferencesDialog : public QDialog {
    Q_OBJECT
public:
    explicit ReferencesDialog(const QString& symbol, const QList<LspLocation>& locs,
                              QWidget* parent = nullptr);
    LspLocation selected() const { return m_sel; }

private slots:
    void onJump();
    void onPreview(); // Stage 24: bağlam önizlemesi

private:
    LspLocation m_sel;
    QTreeWidget* m_tree;
    QTextEdit* m_preview = nullptr; // Stage 24
};
