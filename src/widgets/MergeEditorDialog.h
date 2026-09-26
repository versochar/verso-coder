#pragma once
#include <QDialog>

#include "../core/MergeParse.h"

class QTextEdit;

// Stage 30: 3-yönlü birleştirme — bizimki/onlarınki/her ikisi + sonuç.
class MergeEditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit MergeEditorDialog(const QString& filePath, const QString& text,
                               QWidget* parent = nullptr);
    // Çözülmüş metin (Apply ile); çözümsüz hunk varsa boş + kalan sayı
    QString resolvedText() const { return m_resolved; }
    int remaining() const;

private slots:
    void takeOurs();
    void takeTheirs();
    void takeBoth();
    void applyAll();

private:
    void showCurrent();
    QString m_path;
    QStringList m_lines;
    QList<MergeHunk> m_hunks;
    QList<int> m_choices; // hunk başına 0/1/2 (-1 = kararsız)
    int m_cur = 0;
    QString m_resolved;
    QTextEdit* m_ours = nullptr;
    QTextEdit* m_theirs = nullptr;
    QTextEdit* m_result = nullptr;
};
