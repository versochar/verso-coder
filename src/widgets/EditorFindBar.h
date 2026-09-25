#pragma once
#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

// Stage 11: editör-içi bulma çubuğu (Ctrl+F).
// Tüm eşleşmeler editörde + overview ruler'da + minimap'te işaretlenir.
class EditorFindBar : public QWidget {
    Q_OBJECT
public:
    explicit EditorFindBar(QWidget* parent = nullptr);

    QString needle() const;
    bool caseSensitive() const;
    bool wholeWord() const;
    void setCountText(const QString& t);
    void focusNeedle();
    // Geçerli editördeki seçimden sorguyu doldur
    void setNeedle(const QString& s);

signals:
    void queryChanged();
    void navigate(int dir); // +1 sonraki, -1 önceki
    void closed();

private:
    QLineEdit* m_edit;
    QLabel* m_count;
    QCheckBox* m_case;
    QCheckBox* m_word;
    QPushButton* m_prev;
    QPushButton* m_next;
};
