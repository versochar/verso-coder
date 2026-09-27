#pragma once
#include "../core/PatchQueue.h"
#include <QDialog>

class QTreeWidget;
class QTreeWidgetItem;
class QTextEdit;
class QLabel;

// Ajanın önerdiği çoklu dosya düzenlemelerini listeler, diff önizlemesi gösterir
// ve seçilenleri toplu uygulamaya izin verir.
class PatchReviewDialog : public QDialog {
    Q_OBJECT
public:
    explicit PatchReviewDialog(QWidget* parent = nullptr);

    void setEdits(const QList<QueuedEdit>& edits);
    // Stage 39: kuyruk eşiği aşıldıysa başlıkta toplu-onay uyarısı göster
    void setBulkWarning(int threshold);
    QList<QueuedEdit> selectedEdits() const;

private slots:
    void refreshPreview();

private:
    QList<QueuedEdit> m_edits;
    int m_bulkThreshold = -1; // Stage 39: -1 = uyarı yok
    QTreeWidget* m_tree;
    QTextEdit* m_preview;
    QLabel* m_summary;
};
