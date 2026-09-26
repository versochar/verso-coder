#pragma once
#include "../core/TextEdits.h"
#include <QDialog>
#include <QStringList>

class QLineEdit;
class QTextEdit;
class QListWidget;

// Stage 13: yeniden adlandırma önizlemesi — yeni ad + etkilenen satırlar.
// Stage 24: dosya listesi — işareti kaldırılan dosyalar uygulanmaz.
class RenamePreviewDialog : public QDialog {
    Q_OBJECT
public:
    explicit RenamePreviewDialog(const QString& oldName, const QList<LspTextEdit>& edits,
                                 const QString& text, QWidget* parent = nullptr);
    QString newName() const;
    QList<LspTextEdit> edits() const { return m_edits; }
    void setFileList(const QStringList& files); // tüm etkilenen dosyalar
    QStringList excludedFiles() const;          // işareti kaldırılanlar

private slots:
    void onNameChanged(const QString& t);

private:
    QList<LspTextEdit> m_edits;
    QString m_text;
    QLineEdit* m_name;
    QTextEdit* m_preview;
    QListWidget* m_files = nullptr;
};
