#pragma once
#include "../core/ThemeTokens.h"
#include <QList>
#include <QMap>
#include <QPixmap>
#include <QWidget>

class QPlainTextEdit;

// Hafif minimap: satırları çubuk olarak çizer, viewport dikdörtgeni gösterir.
// Stage 10: çubuklar satır içeriğine göre sözdizimi renklerinde.
// Stage 11: git diff işaretleri + arama isabetleri + hover büyüteci (lens).
class Minimap : public QWidget {
    Q_OBJECT
public:
    explicit Minimap(QWidget* parent = nullptr);
    void setEditor(QPlainTextEdit* editor);

    // Satır metninden çubuk rengi (test edilebilir saf mantık)
    static QColor lineColor(const QString& line, const ThemeTokens& tk);

    // Stage 11: 1-based satır → a/m/d + arama satırları
    void setDiffMarks(const QMap<int, char>& marks);
    void setSearchMarks(const QList<int>& lines0, int currentLine0);
    // Stage 17: tanı işaretleri — 1-based satır → severity (1 hata, 2 uyarı)
    void setDiagMarks(const QMap<int, int>& marks);

protected:
    void paintEvent(QPaintEvent* ev) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void leaveEvent(QEvent* e) override;
    void resizeEvent(QResizeEvent* e) override; // Stage 21

private slots:
    void scheduleUpdate();
    void invalidateBars(); // Stage 32: çubuk önbelleğini geçersiz kıl

private:
    void scrollTo(double frac);
    void scrollToLine(int line0); // Stage 21: satır-merkezli atlama
    int lineAtY(int y) const;
    QPlainTextEdit* m_editor = nullptr;
    QMap<int, char> m_diff;     // 1-based
    QList<int> m_search;        // 0-based satırlar
    int m_searchCur = -1;
    QMap<int, int> m_diags;     // Stage 17: 1-based satır → severity
    int m_lensLine = -1;        // hover büyüteci satırı (0-based)
    QPixmap m_barCache;         // Stage 32: çubuk katmanı önbelleği
    quint64 m_barRev = 0;
    quint64 m_barDrawn = ~0ull;
    QSize m_barSize;
    QRgb m_barTheme = 0;
};
