#pragma once
#include <QDialog>
#include <QStringList>

class QLineEdit;
class QTreeWidget;

// Stage 25: toplu dosya yeniden adlandırma — desen + önizleme.
// Desen: "*" joker (örn. "test_*.cpp" → "spec_*.cpp").
class BulkRenameDialog : public QDialog {
    Q_OBJECT
public:
    explicit BulkRenameDialog(const QString& dir, QWidget* parent = nullptr);

private slots:
    void refreshPreview();
    void applyAll();

private:
    static QString applyPattern(const QString& name, const QString& from,
                                const QString& to);

    QLineEdit* m_from = nullptr;
    QLineEdit* m_to = nullptr;
    QTreeWidget* m_preview = nullptr;
    QString m_dir;
    QStringList m_files;
};
