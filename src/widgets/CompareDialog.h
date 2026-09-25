#pragma once
#include <QDialog>
#include <QStringList>

class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;
class QCheckBox;

struct CompareRow {
    QString rel;
    QString status; // "sadeceA" | "sadeceB" | "farkli" | "ayni"
};

// İki dizini karşılaştırır (boyut+mtime, gerekirse hash).
class CompareDialog : public QDialog {
    Q_OBJECT
public:
    explicit CompareDialog(QWidget* parent = nullptr);
    void setDirA(const QString& dir);

    static QList<CompareRow> compareDirs(const QString& a, const QString& b);
    static bool filesEqual(const QString& fa, const QString& fb);

signals:
    void diffRequested(const QString& fileA, const QString& fileB, const QString& title);

private slots:
    void run();
    void openDiff(QTreeWidgetItem* it);

private:
    QLineEdit* m_a;
    QLineEdit* m_b;
    QCheckBox* m_hideSame;
    QTreeWidget* m_list;
};
