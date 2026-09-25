#pragma once
#include <QDialog>

class QListWidget;

// Stage 15: özel istem kitaplığı — kaydet/çalıştır/sil.
class PromptLibraryDialog : public QDialog {
    Q_OBJECT
public:
    explicit PromptLibraryDialog(QWidget* parent = nullptr);

signals:
    void runRequested(const QString& name, const QString& prompt);

private slots:
    void refresh();
    void addNew();
    void removeSelected();
    void runSelected();

private:
    QListWidget* m_list;
};
