#pragma once
#include "../core/ThemeTokens.h"
#include <QColor>
#include <QDialog>
#include <QStringList>

class QGridLayout;
class QHBoxLayout;
class QLabel;
class QComboBox;

// Stage 9: tema kartı — paleti minyatür editör önizlemesi olarak çizer.
class ThemeCard : public QWidget {
    Q_OBJECT
public:
    explicit ThemeCard(const ThemeTokens& t, QWidget* parent = nullptr);
    QString themeName() const { return m_t.name; }
    void setCurrent(bool on);

signals:
    void activated(const QString& name); // çift tık / tık + Enter

protected:
    void paintEvent(QPaintEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;

private:
    ThemeTokens m_t;
    bool m_current = false;
};

// Stage 9: tema galerisi + accent renk seçimi + içe/dışa aktarma.
class ThemeGalleryDialog : public QDialog {
    Q_OBJECT
public:
    explicit ThemeGalleryDialog(const QString& currentTheme,
                                const QColor& currentAccent, // geçersizse "tema rengi"
                                QWidget* parent = nullptr);

signals:
    void themeSelected(const QString& name);
    void accentSelected(const QColor& accent); // geçersiz QColor = tema varsayılanı

private:
    void buildCards();
    // Stage 21: son özel vurgu renkleri
    static QStringList recentAccents();
    void pushRecentAccent(const QColor& c);
    void rebuildRecentAccents();
    QString m_current;
    QColor m_accent;
    QGridLayout* m_grid = nullptr;
    QLabel* m_accentLabel = nullptr;
    QHBoxLayout* m_recentRow = nullptr;
    QComboBox* m_cvdPreview = nullptr; // Stage 23: renk körü önizleme
    QList<ThemeCard*> m_cards;
};
