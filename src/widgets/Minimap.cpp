#include "Minimap.h"
#include "../core/ThemeManager.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSet>
#include <QTextBlock>

Minimap::Minimap(QWidget* parent) : QWidget(parent) {
    setFixedWidth(88);
    setMinimumHeight(100);
    setMouseTracking(true);
}

void Minimap::setDiffMarks(const QMap<int, char>& marks) {
    m_diff = marks;
    update();
}

void Minimap::setSearchMarks(const QList<int>& lines0, int currentLine0) {
    m_search = lines0;
    m_searchCur = currentLine0;
    update();
}

// Stage 17: tanı işaretleri (sağ şerit, aramayla çakışırsa tanı üstte)
void Minimap::setDiagMarks(const QMap<int, int>& marks) {
    m_diags = marks;
    update();
}

void Minimap::setEditor(QPlainTextEdit* editor) {
    if (m_editor) disconnect(m_editor, nullptr, this, nullptr);
    m_editor = editor;
    m_diff.clear();
    m_search.clear();
    m_searchCur = -1;
    m_lensLine = -1;
    if (!m_editor) { update(); return; }
    connect(m_editor->document(), &QTextDocument::contentsChanged, this,
            &Minimap::invalidateBars); // Stage 32: içerik değişince çubuklar tazelenir
    connect(m_editor->verticalScrollBar(), &QScrollBar::valueChanged, this, &Minimap::scheduleUpdate);
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this, &Minimap::scheduleUpdate);
    update();
}

void Minimap::scheduleUpdate() { update(); }

void Minimap::invalidateBars() {
    ++m_barRev;
    update();
}

// Stage 10: satır içeriğinden çubuk rengi (sözdizimi sezgisi)
QColor Minimap::lineColor(const QString& line, const ThemeTokens& tk) {
    const QString s = line.trimmed();
    if (s.isEmpty()) return QColor(); // çağıran varsayılanı kullanır
    if (s.startsWith("//") || s.startsWith("#") || s.startsWith("/*") || s.startsWith("*"))
        return tk.synComment;
    if (s.startsWith("\"") || s.startsWith("'"))
        return tk.synString;
    if (s.startsWith("#include") || s.startsWith("import ") || s.startsWith("from "))
        return tk.synString;
    static const QSet<QString> kws = {
        "if", "else", "for", "while", "return", "class", "struct", "void", "int",
        "float", "double", "bool", "const", "static", "auto", "namespace", "using",
        "public", "private", "protected", "def", "function", "let", "var", "new"};
    const QString first = s.section(QRegularExpression("[\\s(]"), 0, 0);
    if (kws.contains(first)) return tk.synKeyword;
    return QColor();
}

void Minimap::paintEvent(QPaintEvent*) {
    QPainter p(this);
    const ThemeTokens tk = ThemeManager::instance().tokens();
    p.fillRect(rect(), tk.gutterBg);
    if (!m_editor || !m_editor->document()) return;
    auto* doc = m_editor->document();
    int n = doc->blockCount();
    if (n <= 0) return;
    int step = n > 15000 ? (n / 8000 + 1) : 1; // büyük dosyada örnekle
    double h = (double)height() / qMax(1, n);
    double barH = qMax(1.5, h * 0.8 * step);
    int curLine = m_editor->textCursor().blockNumber();
    // Stage 32: çubuk katmanını önbelleğe al — kaydırma/cursor sadece üstte çizer
    if (m_barCache.isNull() || m_barSize != size() || m_barDrawn != m_barRev ||
        m_barTheme != tk.gutterBg.rgb()) {
        m_barCache = QPixmap(size());
        m_barCache.fill(tk.gutterBg);
        QPainter bp(&m_barCache);
        for (int i = 0; i < n; i += step) {
            QTextBlock b = doc->findBlockByNumber(i);
            QString t = b.text();
            QString s = t.trimmed();
            double y = i * h;
            int lead = 0;
            while (lead < t.size() && t[lead].isSpace()) ++lead;
            int indent = lead / 4;
            double x = 4 + qMin(30, indent * 5);
            double w = qMin((double)width() - 8 - x, (double)qMax(6, s.size()));
            QColor c = lineColor(t, tk);
            if (!c.isValid()) c = ThemeTokens::withAlphaF(tk.text, tk.dark ? 0.55 : 0.70);
            bp.fillRect(QRectF(x, y, w, barH), c);
        }
        m_barSize = size();
        m_barDrawn = m_barRev;
        m_barTheme = tk.gutterBg.rgb();
    }
    p.drawPixmap(0, 0, m_barCache);
    // Geçerli satır vurgusu (dinamik)
    p.fillRect(QRectF(0, curLine * h, width(), barH),
               ThemeTokens::withAlphaF(tk.textStrong, 0.30));
    // viewport
    auto* sb = m_editor->verticalScrollBar();
    double total = sb->maximum() + sb->pageStep();
    if (total > 0) {
        double y = (double)sb->value() / total * height();
        double vh = (double)sb->pageStep() / total * height();
        p.setPen(tk.accent);
        p.setBrush(ThemeTokens::withAlphaF(tk.accent, 0.16));
        p.drawRect(QRectF(1, y, width() - 2, qMax(12.0, vh)));
    }
    // Stage 11: git diff işaretleri (sol kenar şeridi)
    if (!m_diff.isEmpty()) {
        for (auto it = m_diff.begin(); it != m_diff.end(); ++it) {
            const int ln = it.key() - 1;
            if (ln < 0 || ln >= n) continue;
            QColor mc = (it.value() == 'a') ? tk.success
                        : (it.value() == 'm') ? tk.accent : tk.error;
            p.fillRect(QRectF(0, ln * h, 3, qMax(1.5, barH)), mc);
        }
    }
    // Stage 11: arama isabetleri (sağ kenar şeridi)
    if (!m_search.isEmpty()) {
        for (int ln : m_search) {
            if (ln < 0 || ln >= n) continue;
            p.fillRect(QRectF(width() - 4, ln * h, 3, qMax(1.5, barH)),
                       ln == m_searchCur ? tk.textStrong
                                         : ThemeTokens::withAlphaF(tk.accent, 0.8));
        }
    }
    // Stage 17: tanı işaretleri (sağ şerit, aramanın üstüne)
    if (!m_diags.isEmpty()) {
        for (auto it = m_diags.begin(); it != m_diags.end(); ++it) {
            const int ln = it.key() - 1;
            if (ln < 0 || ln >= n) continue;
            p.fillRect(QRectF(width() - 4, ln * h, 3, qMax(1.5, barH)),
                       it.value() == 1 ? tk.error : tk.warning);
        }
    }
    // Stage 11: hover büyüteci — gerçek metinle ~7 satırlık önizleme
    if (m_lensLine >= 0 && m_lensLine < n) {
        QFont lf("Consolas, monospace", 9);
        p.setFont(lf);
        QFontMetrics lfm(lf);
        const int lo = qMax(0, m_lensLine - 3);
        const int hi = qMin(n - 1, m_lensLine + 3);
        const int lw = width() + 150; // sola taşan kutu
        const int lx = qMax(0, width() - lw);
        int ly = (int)(m_lensLine * h) - (m_lensLine - lo + 1) * (lfm.height() + 2) - 6;
        ly = qBound(0, ly, qMax(0, height() - (hi - lo + 1) * (lfm.height() + 2) - 12));
        const int lh = (hi - lo + 1) * (lfm.height() + 2) + 12;
        p.fillRect(lx, ly, lw, lh, tk.surface);
        p.setPen(tk.border);
        p.drawRect(lx, ly, lw, lh);
        int ty = ly + 8;
        for (int ln = lo; ln <= hi; ++ln) {
            QTextBlock lb = doc->findBlockByNumber(ln);
            p.setPen(ln == m_lensLine ? tk.accent : tk.text);
            p.drawText(lx + 8, ty, lw - 40, lfm.height() + 2, Qt::AlignLeft,
                       QString("%1  %2").arg(ln + 1).arg(lb.text().left(60)));
            ty += lfm.height() + 2;
        }
    }
}

int Minimap::lineAtY(int y) const {
    if (!m_editor || !m_editor->document()) return -1;
    const int n = m_editor->document()->blockCount();
    if (n <= 0 || height() <= 0) return -1;
    const double h = (double)height() / qMax(1, n);
    return qBound(0, int(y / h), n - 1);
}

void Minimap::mousePressEvent(QMouseEvent* e) {
    scrollToLine(lineAtY((int)e->position().y()));
}

void Minimap::mouseMoveEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton) scrollToLine(lineAtY((int)e->position().y()));
    else {
        const int ln = lineAtY((int)e->position().y());
        if (ln != m_lensLine) { m_lensLine = ln; update(); }
    }
}
void Minimap::leaveEvent(QEvent* e) {
    QWidget::leaveEvent(e);
    if (m_lensLine >= 0) { m_lensLine = -1; update(); }
}
void Minimap::scrollTo(double frac) {
    if (!m_editor) return;
    auto* sb = m_editor->verticalScrollBar();
    sb->setValue(int(frac * (sb->maximum() + sb->pageStep()) - sb->pageStep() / 2));
}

// Stage 21: satır-merkezli atlama — tıklanan satır görünümün ortasına gelir
void Minimap::scrollToLine(int line0) {
    if (!m_editor || line0 < 0) return;
    QTextBlock b = m_editor->document()->findBlockByNumber(line0);
    if (!b.isValid()) return;
    const int y = m_editor->cursorRect(QTextCursor(b)).top();
    auto* sb = m_editor->verticalScrollBar();
    sb->setValue(y - sb->pageStep() / 2);
}

void Minimap::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    scheduleUpdate(); // Stage 21: boyut değişince şeritleri tazele
}
