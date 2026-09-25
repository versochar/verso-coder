#pragma once
#include <QDialog>

class QLineEdit;
class QListWidget;

// Ctrl+P hızlı dosya açma: fuzzy filtreli liste.
class QuickOpenDialog : public QDialog {
    Q_OBJECT
public:
    explicit QuickOpenDialog(QWidget* parent = nullptr);
    void setFiles(const QStringList& files);
    QString selected() const { return m_selected; }

private:
    static int fuzzyScore(const QString& pattern, const QString& text);
    void refilter();

    QLineEdit* m_search;
    QListWidget* m_list;
    QStringList m_files;
    QString m_selected;
};
