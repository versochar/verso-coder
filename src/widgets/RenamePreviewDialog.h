#pragma once
#include "../core/TextEdits.h"
#include <QDialog>

class QLineEdit;
class QTextEdit;

// Stage 13: yeniden adlandırma önizlemesi — yeni ad + etkilenen satırlar.
class RenamePreviewDialog : public QDialog {
    Q_OBJECT
public:
    explicit RenamePreviewDialog(const QString& oldName, const QList<LspTextEdit>& edits,
                                 const QString& text, QWidget* parent = nullptr);
    QString newName() const;
    QList<LspTextEdit> edits() const { return m_edits; }

private slots:
    void onNameChanged(const QString& t);

private:
    QList<LspTextEdit> m_edits;
    QString m_text;
    QLineEdit* m_name;
    QTextEdit* m_preview;
};
