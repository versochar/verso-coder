#pragma once
#include <QDialog>

class QListWidget;
class QLineEdit;
class QPlainTextEdit;
class QLabel;

// Stage 15: özel istem kitaplığı — kaydet/çalıştır/sil.
// Stage 33: galeri — arama, önizleme/düzenleme, içe/dışa aktarma (JSON).
class PromptLibraryDialog : public QDialog {
    Q_OBJECT
public:
    explicit PromptLibraryDialog(QWidget* parent = nullptr);

signals:
    // taskHint: "!test ..." gibi ipucundan gelen görev (boşsa serbest sohbet)
    void runRequested(const QString& name, const QString& prompt, const QString& taskHint);

private slots:
    void refresh();
    void addNew();
    void removeSelected();
    void runSelected();
    void saveEdits();
    void importJson();
    void exportJson();

private:
    void selectItem(const QString& name);
    QString currentName() const;

    QListWidget* m_list = nullptr;
    QLineEdit* m_search = nullptr;
    QLineEdit* m_name = nullptr;
    QPlainTextEdit* m_text = nullptr;
    QLabel* m_hint = nullptr;
};
