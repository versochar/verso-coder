#pragma once
#include <QDialog>

class QTextEdit;

// Ajan düzenlemesi onay ekranı: eski vs önerilen kodu gösterir.
class ApplyEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit ApplyEditDialog(QWidget* parent = nullptr);
    void setTexts(const QString& filePath, const QString& oldText, const QString& newText);
    QString newText() const;

private:
    QTextEdit* m_old;
    QTextEdit* m_new;
};
