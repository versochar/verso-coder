#include "CodeEditor.h"
#include "../core/EditorConfig.h"
#include "../core/ScopeChain.h"
#include "../core/SelectionGrow.h"
#include "../core/SettingsManager.h"
#include "../core/AutoPairs.h"
#include "../core/BlockSelect.h"
#include "../core/SnippetEngine.h"
#include <QApplication>
#include <QClipboard>
#include "../core/SpellChecker.h"
#include "../core/Spelling.h"
#include "../core/GitBlame.h"
#include "../core/ThemeManager.h"
#include "../core/Typography.h"
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMap>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QProcess>
#include <QDateTime>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextStream>
#include <QTimer>
#include <QWheelEvent>

// --- Renklendirme ---
CodeHighlighter::CodeHighlighter(QTextDocument* doc) : QSyntaxHighlighter(doc) {}

void CodeHighlighter::highlightBlock(const QString& text) {
    static const QRegularExpression kw(
        "\\b(class|struct|public|private|protected|virtual|override|return|if|else|for|while|"
        "include|import|from|def|void|int|float|double|char|bool|const|static|new|delete|"
        "namespace|using|template|typename|auto|nullptr|true|false|function|let|var)\\b");
    static const QRegularExpression str("\"[^\"]*\"|'[^']*'|`[^`]*`");
    static const QRegularExpression com("(//.*|#.*|/\\*.*\\*/)");
    static const QRegularExpression num("\\b(0x[0-9a-fA-F]+|\\d+\\.?\\d*)\\b");
    const ThemeTokens tk = ThemeManager::instance().tokens();
    QTextCharFormat f;
    f.setForeground(tk.synKeyword);
    for (auto it = kw.globalMatch(text); it.hasNext();) {
        auto m = it.next();
        setFormat(m.capturedStart(), m.capturedLength(), f);
    }
    f.setForeground(tk.synString);
    for (auto it = str.globalMatch(text); it.hasNext();) {
        auto m = it.next();
        setFormat(m.capturedStart(), m.capturedLength(), f);
    }
    f.setForeground(tk.synComment);
    for (auto it = com.globalMatch(text); it.hasNext();) {
        auto m = it.next();
        setFormat(m.capturedStart(), m.capturedLength(), f);
    }
    f.setForeground(tk.synNumber);
    for (auto it = num.globalMatch(text); it.hasNext();) {
        auto m = it.next();
        setFormat(m.capturedStart(), m.capturedLength(), f);
    }
}

// --- Gutter ---
class Gutter : public QWidget {
public:
    explicit Gutter(CodeEditor* e) : QWidget(e), ed(e) { setMouseTracking(true); }
    QSize sizeHint() const override { return QSize(48, 0); }
protected:
    void paintEvent(QPaintEvent* ev) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void leaveEvent(QEvent* e) override;
private:
    int lineAt(int y) const;
    CodeEditor* ed;
};

void Gutter::paintEvent(QPaintEvent* ev) {
    ed->gutterPaintEvent(ev);
}

void Gutter::mousePressEvent(QMouseEvent* e) {
    ed->gutterClick((int)e->position().y());
}

void Gutter::mouseMoveEvent(QMouseEvent* e) {
    ed->setGutterHover(lineAt((int)e->position().y()));
}

void Gutter::leaveEvent(QEvent* e) {
    QWidget::leaveEvent(e);
    ed->setGutterHover(-1);
}

int Gutter::lineAt(int y) const {
    return ed->lineAtY(y);
}

// --- Editör ---
CodeEditor::CodeEditor(QWidget* parent) : QPlainTextEdit(parent) {
    QFont f("JetBrains Mono, Consolas, monospace", 11);
    setFont(f);
    setTabStopDistance(4 * fontMetrics().horizontalAdvance(' '));
    m_gutter = new Gutter(this);
    m_highlighter = new CodeHighlighter(document());
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumbers);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);
    connect(this, &QPlainTextEdit::textChanged, this, &CodeEditor::refreshFolds);
    // Stage 11: gökkuşağı önbelleği + yapışkan şerit
    connect(this, &QPlainTextEdit::textChanged, this, &CodeEditor::refreshRainbow);
    connect(this, &QPlainTextEdit::textChanged, this, &CodeEditor::refreshSticky);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::refreshSticky);
    // Stage 17: snippet duraklarını düzenlemeyle birlikte kaydır
    connect(document(), &QTextDocument::contentsChange, this,
            [this](int pos, int removed, int added) {
                if (m_snipStops.isEmpty()) return;
                const int delta = added - removed;
                for (int i = 0; i < m_snipStops.size(); ++i) {
                    if (m_snipStops[i] > pos + removed)
                        m_snipStops[i] += delta;
                    else if (m_snipStops[i] > pos)
                        m_snipStops[i] = pos; // durağın içi değişti: başa çek
                }
            });
    m_sticky = new QPushButton(this);
    m_sticky->setObjectName("stickyBar");
    m_sticky->setFlat(true);
    // Stage 23: özel imleç yanıp sönme sayacı
    m_blinkTimer = new QTimer(this);
    connect(m_blinkTimer, &QTimer::timeout, this, [this]() {
        m_blinkOn = !m_blinkOn;
        viewport()->update();
    });
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, [this]() {
        m_blinkOn = true;
        if (m_blinkTimer->isActive()) m_blinkTimer->start();
    });
    // Stage 24: blame hayaleti (imleç durunca)
    m_blameTimer = new QTimer(this);
    m_blameTimer->setSingleShot(true);
    connect(m_blameTimer, &QTimer::timeout, this, &CodeEditor::refreshBlame);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, [this]() {
        m_blame.clear();
        m_blameTimer->start(800);
    });
    m_sticky->setCursor(Qt::PointingHandCursor);
    m_sticky->setFocusPolicy(Qt::NoFocus);
    m_sticky->hide();
    connect(m_sticky, &QPushButton::clicked, this, [this]() {
        if (m_stickyLine >= 0) gotoLine(m_stickyLine + 1);
    });
    highlightCurrentLine();
    updateLineNumbers(viewport()->rect(), 0);
}

CodeEditor::~CodeEditor() {
    delete m_spell;
}

bool CodeEditor::loadFile(const QString& path) {
    QFileInfo fi(path);
    AppSettings cfg = SettingsManager::instance().load();
    m_preview = false;
    setReadOnly(false);
    m_large = fi.size() > (qint64)qMax(1, cfg.largeFileMb) * 1024 * 1024;
    m_highlighter->setDocument(m_large ? nullptr : document());
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray raw = f.readAll();
    m_crlf = raw.contains("\r\n");
    m_blame.clear();
    m_blameLine = -1;
    // editorconfig
    m_useSpaces = true;
    m_ecEol.clear();
    m_ecTrim = false;
    m_ecFinalNl = false;
    if (cfg.useEditorConfig) {
        EditorConfig ec = EditorConfigParser::forFile(path);
        if (ec.found) {
            m_useSpaces = ec.useSpaces;
            m_tab = ec.indentSize;
            setTabStopDistance(m_tab * fontMetrics().horizontalAdvance(' '));
            m_ecEol = ec.endOfLine;
            m_ecTrim = ec.trimTrailing;
            m_ecFinalNl = ec.insertFinalNewline;
        }
    }
    setPlainText(QString::fromUtf8(raw));
    m_path = path;
    document()->setModified(false);
    m_lastMtime = QFileInfo(path).lastModified();
    m_extra.clear();
    refreshFolds();
    refreshBlameAges(); // Stage 24: ısı haritası yaşları
    return true;
}

bool CodeEditor::saveFile(const QString& path) {
    QString p = path.isEmpty() ? m_path : path;
    if (p.isEmpty() || m_preview) return false;
    QString text = toPlainText();
    QStringList lines = text.split('\n');
    if (m_ecTrim)
        for (QString& ln : lines) {
            int n = ln.size();
            while (n > 0 && (ln[n - 1] == ' ' || ln[n - 1] == '\t')) --n;
            ln.truncate(n);
        }
    text = lines.join('\n');
    if (m_ecFinalNl && !text.endsWith('\n')) text += '\n';
    QString targetEol = m_ecEol.isEmpty() ? (m_crlf ? "crlf" : "lf") : m_ecEol;
    QByteArray out = text.toUtf8();
    if (targetEol == "crlf") out.replace("\n", "\r\n");
    QFile f(p);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(out);
    f.close();
    m_path = p;
    document()->setModified(false);
    m_lastMtime = QFileInfo(p).lastModified();
    return true;
}

// Önizleme: ilk maxLines satır, salt-okunur
bool CodeEditor::loadPreview(const QString& path, int maxLines) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QStringList lines;
    int total = 0;
    while (!f.atEnd() && total < maxLines) {
        lines << QString::fromUtf8(f.readLine());
        ++total;
    }
    bool more = !f.atEnd();
    while (!f.atEnd()) { f.readLine(); ++total; }
    m_preview = more;
    setReadOnly(more);
    m_large = true;
    m_highlighter->setDocument(nullptr);
    setPlainText(lines.join("") + (more ? QString("\n\n[... önizleme: %1/%2 satır — paletten 'Tamamını Yükle' ...]")
                                                 .arg(total < maxLines ? total : maxLines).arg(total)
                                         : ""));
    m_path = path;
    document()->setModified(false);
    m_extra.clear();
    m_foldStarts.clear();
    m_folded.clear();
    return true;
}

bool CodeEditor::loadFullPreview() {
    if (!m_preview) return false;
    m_preview = false;
    setReadOnly(false);
    return loadFile(m_path);
}

void CodeEditor::applyEditorSettings(int fontSize, int tabWidth) {
    const AppSettings s = SettingsManager::instance().load();
    TypographySettings t = ThemeManager::instance().typography();
    t.editorSize = qBound(8, fontSize, 24);
    setFont(Typography::editorFont(t));
    m_tab = qBound(2, tabWidth, 8);
    setTabStopDistance(m_tab * fontMetrics().horizontalAdvance(' '));
    // Stage 9: imleç genişliği + satır yüksekliği
    setCursorWidth(qBound(1, s.cursorWidth, 6));
    applyLineHeight(s.lineHeight);
    updateLineNumbers(viewport()->rect(), 0);
}

// Stage 9: satır yüksekliği (1.0 = tek satır) — blok formatıyla uygulanır.
void CodeEditor::applyLineHeight(double lh) {
    QTextCursor c(document());
    c.select(QTextCursor::Document);
    QTextBlockFormat fmt;
    fmt.setLineHeight(lh > 1.001 ? int(Typography::clampLineHeight(lh) * 100) : 100,
                      QTextBlockFormat::ProportionalHeight);
    c.mergeBlockFormat(fmt);
}

// Stage 9: tema değişince renkleri tazele.
void CodeEditor::refreshTheme() {
    m_highlighter->rehighlight();
    refreshRainbow(); // Stage 11: gökkuşağı renkleri temaya bağlı
    refreshExtraSelections();
    refreshSticky();
    updateLineNumbers(viewport()->rect(), 0);
    update();
}

bool CodeEditor::checkExternalChanged() const {
    if (m_path.isEmpty() || m_preview) return false;
    QFileInfo fi(m_path);
    if (!fi.exists()) return false;
    return fi.lastModified() > m_lastMtime.addMSecs(500);
}

bool CodeEditor::reloadFromDisk() {
    if (m_path.isEmpty()) return false;
    int pos = cursorOffset();
    if (m_preview) return loadPreview(m_path);
    if (!loadFile(m_path)) return false;
    setCursorOffset(pos);
    return true;
}

int CodeEditor::cursorOffset() const {
    return textCursor().position();
}

void CodeEditor::setCursorOffset(int pos) {
    QTextCursor c = textCursor();
    c.setPosition(qBound(0, pos, document()->characterCount() - 1));
    setTextCursor(c);
}

// ---------- Çoklu imleç ----------
void CodeEditor::addCursorAt(int pos) {
    pos = qBound(0, pos, document()->characterCount() - 1);
    QTextCursor main = textCursor();
    if (main.position() == pos) return;
    for (const auto& c : m_extra)
        if (c.position() == pos && !c.hasSelection()) return;
    QTextCursor c(document());
    c.setPosition(pos);
    m_extra.append(c);
    refreshExtraSelections();
}

static QString wordAt(const QString& text, int pos, int& start) {
    int s = pos, e = pos;
    auto isWord = [](QChar ch) { return ch.isLetterOrNumber() || ch == '_'; };
    while (s > 0 && isWord(text[s - 1])) --s;
    while (e < text.size() && isWord(text[e])) ++e;
    start = s;
    return text.mid(s, e - s);
}

bool CodeEditor::selectNextOccurrence() {
    QString doc = toPlainText();
    QTextCursor main = textCursor();
    QString needle;
    int from = 0;
    if (main.hasSelection()) {
        needle = main.selectedText().replace(QChar(0x2029), "\n");
        from = main.selectionEnd();
    } else {
        int st = 0;
        needle = wordAt(doc, main.position(), st);
        if (needle.isEmpty()) return false;
        from = main.position();
        // Önce mevcut kelimeyi seç
        main.setPosition(st);
        main.setPosition(st + needle.size(), QTextCursor::KeepAnchor);
        setTextCursor(main);
    }
    if (needle.isEmpty()) return false;
    int idx = doc.indexOf(needle, from);
    if (idx < 0) idx = doc.indexOf(needle, 0); // başa sar
    if (idx < 0) return false;
    // Zaten imleç varsa atla
    for (const auto& c : std::as_const(m_extra))
        if (c.hasSelection() && c.selectionStart() == idx) return true;
    if (main.hasSelection() && main.selectionStart() == idx) return true;
    QTextCursor c(document());
    c.setPosition(idx);
    c.setPosition(idx + needle.size(), QTextCursor::KeepAnchor);
    m_extra.append(c);
    refreshExtraSelections();
    return true;
}

void CodeEditor::selectAllOccurrences() {
    QTextCursor main = textCursor();
    QString needle;
    if (main.hasSelection())
        needle = main.selectedText().replace(QChar(0x2029), "\n");
    else {
        int st = 0;
        needle = wordAt(toPlainText(), main.position(), st);
    }
    if (needle.isEmpty()) return;
    QString doc = toPlainText();
    int idx = 0;
    while ((idx = doc.indexOf(needle, idx)) >= 0) {
        QTextCursor c(document());
        c.setPosition(idx);
        c.setPosition(idx + needle.size(), QTextCursor::KeepAnchor);
        bool dup = (main.hasSelection() && main.selectionStart() == idx);
        for (const auto& e : std::as_const(m_extra))
            if (e.selectionStart() == idx) { dup = true; break; }
        if (!dup) m_extra.append(c);
        idx += needle.size();
        if (m_extra.size() > 500) break;
    }
    refreshExtraSelections();
}

void CodeEditor::clearExtraCursors() {
    if (!m_extra.isEmpty()) { m_extra.clear(); refreshExtraSelections(); }
}

void CodeEditor::insertTextAtCursors(const QString& t) {
    if (m_extra.isEmpty()) {
        textCursor().insertText(t);
        return;
    }
    QList<QTextCursor> all;
    all << textCursor() << m_extra;
    QTextCursor mainCur = textCursor();
    // Sıralamadan önce ana imleci işaretle
    QList<QPair<QTextCursor, bool>> flagged;
    for (const QTextCursor& c : std::as_const(all))
        flagged << qMakePair(c, c.position() == mainCur.position() && c.anchor() == mainCur.anchor());
    std::sort(flagged.begin(), flagged.end(),
              [](const auto& a, const auto& b) { return a.first.position() > b.first.position(); });
    QTextCursor undo(document());
    undo.beginEditBlock();
    QTextCursor newMain = mainCur;
    QList<QTextCursor> newExtra;
    for (auto& pr : flagged) {
        pr.first.insertText(t);
        if (pr.second) newMain = pr.first;
        else newExtra << pr.first;
    }
    undo.endEditBlock();
    setTextCursor(newMain);
    m_extra.clear();
    for (const QTextCursor& c : std::as_const(newExtra)) {
        if (c.position() == newMain.position()) continue;
        bool dup = false;
        for (const auto& k : std::as_const(m_extra))
            if (k.position() == c.position()) { dup = true; break; }
        if (!dup) m_extra.append(c);
    }
    refreshExtraSelections();
}

// ---------- Katlama ----------
QList<FoldRange> CodeEditor::detectFoldRanges(const QString& text) {
    QList<FoldRange> out;
    QStringList lines = text.split('\n');
    auto indentOf = [](const QString& ln) {
        int n = 0;
        for (QChar ch : ln) {
            if (ch == ' ') ++n;
            else if (ch == '\t') n += 4;
            else break;
        }
        return n;
    };
    int n = lines.size();
    if (n > 200000) return out; // güvenlik cap'i
    for (int i = 0; i < n; ++i) {
        if (lines[i].trimmed().isEmpty()) continue;
        int j = i + 1;
        while (j < n && lines[j].trimmed().isEmpty()) ++j;
        if (j >= n) break;
        if (indentOf(lines[j]) > indentOf(lines[i])) {
            int k = j;
            int last = j;
            while (k < n) {
                if (!lines[k].trimmed().isEmpty()) {
                    if (indentOf(lines[k]) <= indentOf(lines[i])) break;
                    last = k;
                }
                ++k;
            }
            if (last > i) {
                out.append({i, last});
                if (out.size() > 2000) break;
            }
        }
    }
    return out;
}

// Stage 20: yeni dosya dili — şimdilik renklendirmeyi tazeler
// (CodeHighlighter dil-agnostik; dil çip + kayıt uzantısı + başlıkta yaşar)
void CodeEditor::setLang(const QString& id) {
    if (m_lang == id) return;
    m_lang = id;
    if (m_highlighter) m_highlighter->rehighlight();
}

// Stage 23: görünüm ayarları
void CodeEditor::setCursorStyle(const QString& st) {
    m_cursorStyle = (st == "block" || st == "underline") ? st : "bar";
    if (m_cursorStyle == "bar") {
        if (m_blinkTimer) m_blinkTimer->stop();
        AppSettings s = SettingsManager::instance().load();
        setCursorWidth(qBound(1, s.cursorWidth, 6));
    } else {
        setCursorWidth(0); // yerel imleci gizle, paintEvent çizer
        const int ms = SettingsManager::instance().load().cursorBlink;
        m_blinkTimer->start(ms > 0 ? ms : QApplication::cursorFlashTime());
    }
    viewport()->update();
}

void CodeEditor::setCursorBlinkMs(int ms) {
    if (ms > 0) QApplication::setCursorFlashTime(ms * 2);
    if (m_cursorStyle != "bar" && m_blinkTimer) {
        m_blinkTimer->stop();
        m_blinkTimer->start(ms > 0 ? ms : QApplication::cursorFlashTime());
    }
}

void CodeEditor::setSmoothScroll(bool on) { m_smooth = on; }

void CodeEditor::setBracketStyle(const QString& st) {
    m_bracketStyle = st;
    refreshExtraSelections();
}

void CodeEditor::setFoldGutter(const QString& pos) {
    m_foldGutter = pos;
    m_gutter->update();
}

void CodeEditor::setLineHiOpacity(double o) {
    m_lineHiOpacity = qBound(0.05, o, 1.0);
    refreshExtraSelections();
}

void CodeEditor::setShowLineEnds(bool on) {
    QTextOption opt = document()->defaultTextOption();
    QTextOption::Flags f = opt.flags();
    if (on) f |= QTextOption::ShowLineAndParagraphSeparators;
    else f &= ~QTextOption::ShowLineAndParagraphSeparators;
    opt.setFlags(f);
    document()->setDefaultTextOption(opt);
}

// Stage 24: imleç satırının git blame'i → satır sonu hayaleti
void CodeEditor::refreshBlame() {
    m_blame.clear();
    if (m_large || m_preview || m_path.isEmpty() || m_path.startsWith("ssh://")) {
        viewport()->update();
        return;
    }
    const int line0 = textCursor().blockNumber();
    const QFileInfo fi(m_path);
    if (m_blameProc && m_blameProc->state() != QProcess::NotRunning) m_blameProc->kill();
    delete m_blameProc;
    m_blameProc = new QProcess(this);
    m_blameLine = line0;
    connect(m_blameProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, line0](int, QProcess::ExitStatus) {
                QProcess* p = m_blameProc;
                m_blameProc = nullptr;
                if (!p) return;
                const QString out = QString::fromUtf8(p->readAllStandardOutput());
                p->deleteLater();
                if (line0 != textCursor().blockNumber()) return; // imleç gitmiş
                const QList<BlameLine> bl = GitBlame::parse(out);
                if (bl.isEmpty()) return;
                const BlameLine& b = bl.first();
                if (b.author.isEmpty() || b.author == "Not Committed Yet") return;
                const QDate d = QDateTime::fromSecsSinceEpoch(b.authorTime).date();
                InlayHint h;
                h.line = line0;
                h.col = 0;
                h.label = QString("%1 • %2").arg(b.author.left(24)).arg(d.toString("yyyy-MM-dd"));
                h.kind = 0;
                m_blame = {h};
                viewport()->update();
            });
    m_blameProc->setWorkingDirectory(fi.absolutePath());
    m_blameProc->start("git",
                       {"blame", "-L", QString("%1,%1").arg(line0 + 1), "--line-porcelain",
                        "--", fi.fileName()});
}

// Stage 24: tüm dosyanın satır yaşları (ısı haritası için, tek seferlik)
void CodeEditor::refreshBlameAges() {
    m_blameAges.clear();
    if (m_large || m_preview || m_path.isEmpty() || m_path.startsWith("ssh://")) return;
    const QFileInfo fi(m_path);
    QProcess* p = new QProcess(this);
    connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, p](int code, QProcess::ExitStatus) {
                if (code == 0) {
                    const qint64 now = QDateTime::currentSecsSinceEpoch();
                    Q_UNUSED(now);
                    for (const BlameLine& b : GitBlame::parse(
                             QString::fromUtf8(p->readAllStandardOutput())))
                        if (b.line > 0) m_blameAges[b.line] = b.authorTime;
                    m_gutter->update();
                }
                p->deleteLater();
            });
    p->setWorkingDirectory(fi.absolutePath());
    p->start("git", {"blame", "--line-porcelain", "--", fi.fileName()});
}

// Stage 27: code lens + renk kutuları
void CodeEditor::setCodeLens(const QList<InlayHint>& lens) {
    m_lens = lens;
    viewport()->update();
}

void CodeEditor::setColorBoxes(const QList<ColorBox>& boxes) {
    m_colorBoxes = boxes;
    m_gutter->update();
}

// Stage 28: yer imleri
void CodeEditor::setBookmarks(const QList<int>& lines1) {
    m_bookmarks = lines1;
    m_gutter->update();
}

void CodeEditor::wheelEvent(QWheelEvent* e) {
    if (m_smooth && e->angleDelta().x() == 0 && e->modifiers() == Qt::NoModifier) {
        auto* sb = verticalScrollBar();
        int dy = -e->pixelDelta().y();
        if (dy == 0 && !e->angleDelta().isNull())
            dy = -e->angleDelta().y() / 120 * qMax(12, fontMetrics().height() * 3);
        if (dy != 0) {
            if (!m_smoothAnim) {
                m_smoothAnim = new QPropertyAnimation(this);
                m_smoothAnim->setTargetObject(sb);
                m_smoothAnim->setPropertyName("value");
                m_smoothAnim->setDuration(90);
                m_smoothAnim->setEasingCurve(QEasingCurve::OutCubic);
            }
            m_smoothAnim->stop();
            m_smoothAnim->setStartValue(sb->value());
            m_smoothAnim->setEndValue(qBound(sb->minimum(), sb->value() + dy, sb->maximum()));
            m_smoothAnim->start();
            e->accept();
            return;
        }
    }
    QPlainTextEdit::wheelEvent(e);
}

// Stage 21: satır sonu dönüşümü (LF ↔ CRLF), geri-al tek adımda
void CodeEditor::setCrlf(bool on) {
    if (m_crlf == on) return;
    QString t = toPlainText();
    t.replace("\r\n", "\n");
    if (on) t.replace("\n", "\r\n");
    QTextCursor c(document());
    c.beginEditBlock();
    c.select(QTextCursor::Document);
    c.insertText(t);
    c.endEditBlock();
    m_crlf = on;
}
void CodeEditor::refreshFolds() {    if (m_large || m_preview) { m_foldStarts.clear(); return; }
    m_foldStarts.clear();
    m_foldEnds.clear();
    if (blockCount() > 50000 || document()->characterCount() > 300000) return; // yazarken tarama
    for (const auto& r : detectFoldRanges(toPlainText())) {
        m_foldStarts.insert(r.start);
        m_foldEnds[r.start] = r.end;
    }
    // Stage 17: LSP sunucu aralıkları (girintiyle çakışmıyorsa eklenir)
    for (const auto& pr : m_serverFolds) {
        if (!m_foldStarts.contains(pr.first) && pr.second > pr.first) {
            m_foldStarts.insert(pr.first);
            m_foldEnds[pr.first] = pr.second;
        }
    }
    // Geçersiz katlı satırları temizle
    QList<int> drop;
    for (int s : std::as_const(m_folded))
        if (!m_foldStarts.contains(s)) drop << s;
    for (int s : drop) m_folded.remove(s);
    applyFoldVisibility();
}

void CodeEditor::applyFoldVisibility() {
    QTextDocument* doc = document();
    // Önce hepsini göster
    for (QTextBlock b = doc->begin(); b.isValid(); b = b.next())
        if (!b.isVisible()) b.setVisible(true);
    if (m_folded.isEmpty()) {
        doc->markContentsDirty(0, doc->characterCount());
        viewport()->update();
        m_gutter->update();
        return;
    }
    // Katlı aralıkları gizle (girinti algısı + Stage 17 LSP aralıkları)
    for (auto it = m_foldEnds.begin(); it != m_foldEnds.end(); ++it) {
        if (!m_folded.contains(it.key())) continue;
        QTextBlock b = doc->findBlockByNumber(it.key() + 1);
        for (int l = it.key() + 1; l <= it.value() && b.isValid(); ++l, b = b.next())
            b.setVisible(false);
    }
    // İmleç gizli bölgedeyse başlığa çek
    QTextCursor c = textCursor();
    if (!c.block().isVisible()) {
        QTextBlock b = c.block();
        while (b.isValid() && !b.isVisible()) b = b.previous();
        if (b.isValid()) { c.setPosition(b.position()); setTextCursor(c); }
    }
    doc->markContentsDirty(0, doc->characterCount());
    viewport()->update();
    m_gutter->update();
    updateLineNumbers(viewport()->rect(), 0);
}

void CodeEditor::toggleFoldAtLine(int line0) {
    if (!m_foldStarts.contains(line0)) return;
    if (m_folded.contains(line0)) m_folded.remove(line0);
    else m_folded.insert(line0);
    applyFoldVisibility();
}

void CodeEditor::foldAtCursor() {
    toggleFoldAtLine(textCursor().blockNumber());
}

void CodeEditor::unfoldAtCursor() {
    int ln = textCursor().blockNumber();
    // İmleç katlı bölgedeyse en yakın katlı başlığı aç
    int best = -1;
    for (int s : std::as_const(m_folded))
        if (s <= ln && s > best) best = s;
    if (best >= 0) { m_folded.remove(best); applyFoldVisibility(); }
    else if (m_folded.contains(ln)) { m_folded.remove(ln); applyFoldVisibility(); }
}

void CodeEditor::foldAll() {
    m_folded = m_foldStarts;
    // Tek satırlık aralıkları atla (gizlenecek satır yoksa)
    applyFoldVisibility();
}

void CodeEditor::unfoldAll() {
    if (!m_folded.isEmpty()) { m_folded.clear(); applyFoldVisibility(); }
}

QList<int> CodeEditor::foldedStartLines() const {
    return QList<int>(m_folded.begin(), m_folded.end());
}

void CodeEditor::setFoldedLines(const QList<int>& starts) {
    m_folded.clear();
    for (int s : starts) m_folded.insert(s);
    applyFoldVisibility();
}

// ---------- Girinti + bracket ----------
QString CodeEditor::leadingIndent(const QString& line, int tabWidth) {
    QString out;
    for (QChar ch : line) {
        if (ch == ' ' || ch == '\t') out += ch;
        else break;
    }
    Q_UNUSED(tabWidth);
    return out;
}

QString CodeEditor::indentForNewLine(const QString& prevLine, int tabWidth, bool useSpaces) {
    QString base = leadingIndent(prevLine, tabWidth);
    QString t = prevLine;
    while (!t.isEmpty() && (t.back() == ' ' || t.back() == '\t')) t.chop(1);
    bool extra = t.endsWith('{') || t.endsWith(':');
    // Kapanış satırından sonra ekstra girinti yok
    if (t == "}" || t == "}") extra = false;
    if (!extra) return base;
    return base + (useSpaces ? QString(tabWidth, ' ') : "\t");
}

int CodeEditor::findMatchingBracket(const QString& text, int pos) {
    if (pos < 0 || pos >= text.size() || text.size() > 1000000) return -1;
    static const QMap<QChar, QChar> open2close = {{'(', ')'}, {'{', '}'}, {'[', ']'}};
    static const QMap<QChar, QChar> close2open = {{')', '('}, {'}', '{'}, {']', '['}};
    QChar ch = text[pos];
    QChar mate;
    int dir = 0;
    if (open2close.contains(ch)) { mate = open2close[ch]; dir = 1; }
    else if (close2open.contains(ch)) { mate = close2open[ch]; dir = -1; }
    else return -1;
    int depth = 0;
    for (int i = pos; i >= 0 && i < text.size(); i += dir) {
        if (text[i] == ch) ++depth;
        else if (text[i] == mate) {
            --depth;
            if (depth == 0) return i;
        }
    }
    return -1;
}

// ---------- Satır işlemleri ----------
QPair<int, int> CodeEditor::selectedLineRange() const {
    QTextCursor c = textCursor();
    int a = c.selectionStart(), b = c.selectionEnd();
    QTextCursor ca(document()), cb(document());
    ca.setPosition(a);
    cb.setPosition(b);
    int first = ca.blockNumber();
    int last = cb.blockNumber();
    // Seçim blok sonunda bitiyorsa (sütun 0) son satırı dahil etme
    if (c.hasSelection() && cb.positionInBlock() == 0 && last > first) --last;
    return {first, last};
}

void CodeEditor::moveLineOrSelection(int dir) {
    if (m_preview) return;
    auto [first, last] = selectedLineRange();
    int count = blockCount();
    if ((dir < 0 && first == 0) || (dir > 0 && last >= count - 1)) return;
    QStringList lines = toPlainText().split('\n');
    // Son satır boşluğu ( trailing \n ) koruması: split parçası olarak ele al
    QStringList moving;
    for (int i = first; i <= last; ++i) moving << lines[i];
    for (int i = 0; i <= last - first; ++i) lines.removeAt(first);
    int insertAt = first + dir;
    for (int i = 0; i < moving.size(); ++i) lines.insert(insertAt + i, moving[i]);
    int col = textCursor().columnNumber();
    bool hadSel = textCursor().hasSelection();
    setPlainText(lines.join('\n'));
    QTextCursor c(document());
    c.movePosition(QTextCursor::Start);
    c.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, insertAt);
    if (hadSel || first != last) {
        c.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, last - first);
        c.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    } else {
        c.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, qMin(col, c.block().length() - 1));
    }
    setTextCursor(c);
}

void CodeEditor::duplicateLineOrSelection() {
    if (m_preview) return;
    auto [first, last] = selectedLineRange();
    QStringList lines = toPlainText().split('\n');
    QStringList copy;
    for (int i = first; i <= last; ++i) copy << lines[i];
    for (int i = 0; i < copy.size(); ++i) lines.insert(last + 1 + i, copy[i]);
    setPlainText(lines.join('\n'));
    QTextCursor c(document());
    c.movePosition(QTextCursor::Start);
    c.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, last + 1);
    setTextCursor(c);
}

void CodeEditor::sortSelectedLines() {
    if (m_preview) return;
    auto [first, last] = selectedLineRange();
    if (last <= first) return;
    QStringList lines = toPlainText().split('\n');
    QStringList part;
    for (int i = first; i <= last; ++i) part << lines[i];
    std::sort(part.begin(), part.end(),
              [](const QString& a, const QString& b) { return a.toLower() < b.toLower(); });
    for (int i = first; i <= last; ++i) lines[i] = part[i - first];
    int pos = cursorOffset();
    setPlainText(lines.join('\n'));
    setCursorOffset(qMin(pos, document()->characterCount() - 1));
}

void CodeEditor::trimTrailingWhitespace() {
    if (m_preview) return;
    QStringList lines = toPlainText().split('\n');
    for (QString& ln : lines) {
        int n = ln.size();
        while (n > 0 && (ln[n - 1] == ' ' || ln[n - 1] == '\t')) --n;
        ln.truncate(n);
    }
    int pos = cursorOffset();
    setPlainText(lines.join('\n'));
    setCursorOffset(qMin(pos, document()->characterCount() - 1));
}

// ---------- Olaylar ----------
void CodeEditor::resizeEvent(QResizeEvent* e) {
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    m_gutter->setGeometry(QRect(cr.left(), cr.top(), gutterWidth(), cr.height()));
    // Stage 11: yapışkan şerit en üstte tam genişlikte
    if (m_sticky) m_sticky->setGeometry(0, 0, width(), 24);
}

int CodeEditor::gutterWidth() const {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) { max /= 10; ++digits; }
    return 26 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditor::gutterPaintEvent(QPaintEvent* ev) {
    QPainter p(m_gutter);
    const ThemeTokens tk = ThemeManager::instance().tokens();
    p.fillRect(ev->rect(), tk.gutterBg);
    const int curLine = textCursor().blockNumber();
    const qint64 nowSecs = QDateTime::currentSecsSinceEpoch(); // Stage 24: ısı
    QTextBlock b = firstVisibleBlock();
    int num = b.blockNumber();
    int top = (int)blockBoundingGeometry(b).translated(contentOffset()).top();
    int bottom = top + (int)blockBoundingRect(b).height();
    QFontMetrics fm = fontMetrics();
    QFont numFont = font();
    const int gw = m_gutter->width();
    while (b.isValid() && top <= ev->rect().bottom()) {
        if (b.isVisible() && bottom >= ev->rect().top()) {
            const int h = bottom - top;
            // Stage 24: ısı haritası — yeni satırlar vurgulu, eskiler silik
            if (!m_blameAges.isEmpty()) {
                BlameLine tmp;
                tmp.authorTime = m_blameAges.value(num + 1, 0);
                const double heat = GitBlame::heat(tmp, nowSecs);
                if (heat < 0.999) {
                    QColor hc = tk.accent;
                    hc.setAlphaF(0.03 + 0.20 * (1.0 - heat));
                    p.fillRect(0, top, gw, h, hc);
                }
            }
            // Stage 11: hover satırı zemin vurgusu
            if (num == m_hoverLine) {
                p.fillRect(0, top, gw, h, ThemeTokens::withAlphaF(tk.text, 0.06));
            }
            // Stage 14: çalışan çerçeve satırı (sarı zemin + ok)
            if (num == m_frameLine) {
                p.fillRect(0, top, gw, h, ThemeTokens::withAlphaF(tk.warning, 0.30));
            }
            // Stage 28: yer imi (sol kenar şeridi)
            if (m_bookmarks.contains(num + 1)) {
                p.fillRect(0, top, 3, h, QColor("#4ec9b0"));
            }
            // Stage 14: kesme noktası (kırmızı nokta; çerçevedeyse okla birlikte)
            if (m_breakpoints.contains(num + 1)) {
                p.setBrush(tk.error);
                p.setPen(Qt::NoPen);
                const int cy = top + h / 2;
                p.drawEllipse(3, cy - 5, 10, 10);
                p.setBrush(Qt::NoBrush);
            } else if (num == m_frameLine) {
                p.setPen(tk.warning);
                p.drawText(2, top, 14, fm.height(), Qt::AlignLeft, "➤");
            }
            // Stage 27: renk kutuları (sağ kenar, git şeridinin solu)
            for (const ColorBox& cb : std::as_const(m_colorBoxes)) {
                if (cb.line0 != num || !cb.color.isValid()) continue;
                p.fillRect(gw - 12, top + 2, 6, qMax(4, h - 4), cb.color);
                p.setPen(tk.border);
                p.drawRect(gw - 12, top + 2, 6, qMax(4, h - 4));
            }
            // Stage 11: git diff işareti (sağ kenarda 3px şerit)
            if (m_gitMarks.contains(num + 1)) {
                const char st = m_gitMarks[num + 1];
                QColor mc = (st == 'a') ? tk.success : (st == 'm') ? tk.accent : tk.error;
                p.fillRect(gw - 4, top, 3, h, mc);
            }
            // Katlama işareti (hover'da belirgin; konum ayarlanabilir)
            if (m_foldStarts.contains(num) && m_foldGutter != "gizli") {
                bool folded = m_folded.contains(num);
                const bool hov = (num == m_hoverLine);
                // Stage 23: sol | sağ
                const int fx = (m_foldGutter == "sag") ? gw - 16 : 2;
                p.setPen(hov ? tk.textStrong : (num == curLine ? tk.gutterActive : tk.gutterText));
                if (hov) p.fillRect(fx - 2, top, 16, h, ThemeTokens::withAlphaF(tk.accent, 0.25));
                p.drawText(fx, top, 14, fm.height(), Qt::AlignLeft,
                           folded ? "▸" : "▾");
            }
            // Satır numarası (aktif satır kalın)
            QFont f = numFont;
            if (num == curLine) { f.setBold(true); }
            p.setFont(f);
            p.setPen(num == curLine ? tk.gutterActive : tk.gutterText);
            p.drawText(16, top, gw - 24, fm.height(),
                       Qt::AlignRight, QString::number(num + 1));
            p.setFont(numFont);
        }
        b = b.next();
        top = bottom;
        bottom = top + (int)blockBoundingRect(b).height();
        ++num;
    }
}

int CodeEditor::lineAtY(int y) const {
    QTextBlock b = firstVisibleBlock();
    int top = (int)blockBoundingGeometry(b).translated(contentOffset()).top();
    while (b.isValid()) {
        int bottom = top + (int)blockBoundingRect(b).height();
        if (y >= top && y < bottom) return b.blockNumber();
        b = b.next();
        top = bottom;
    }
    return -1;
}

void CodeEditor::setGutterHover(int line0) {
    if (m_hoverLine == line0) return;
    m_hoverLine = line0;
    m_gutter->setCursor(line0 >= 0 && m_foldStarts.contains(line0)
                            ? Qt::PointingHandCursor : Qt::ArrowCursor);
    m_gutter->update();
}

void CodeEditor::gutterClick(int y) {
    int ln = lineAtY(y);
    if (ln < 0) return;
    // Stage 14: sol 16px'teki katlama oku katlar; gutter'ın gerisi kesme noktası
    const int x = m_gutter->mapFromGlobal(QCursor::pos()).x();
    if (x < 16 && m_foldStarts.contains(ln)) { toggleFoldAtLine(ln); return; }
    // Stage 27: renk kutusu tıklaması
    if (x >= m_gutter->width() - 12) {
        for (const ColorBox& cb : std::as_const(m_colorBoxes)) {
            if (cb.line0 == ln) {
                emit colorBoxClicked(ln);
                return;
            }
        }
    }
    emit breakpointToggleRequested(ln + 1);
}

void CodeEditor::setGitMarks(const QMap<int, char>& marks) {
    m_gitMarks = marks;
    m_gutter->update();
}

void CodeEditor::updateViewportMargins() {
    const int top = (m_sticky && m_sticky->isVisible()) ? m_sticky->height() : 0;
    setViewportMargins(gutterWidth(), top, 0, 0);
}

void CodeEditor::updateLineNumbers(const QRect& rect, int dy) {
    if (dy) m_gutter->scroll(0, dy);
    else m_gutter->update(0, 0, gutterWidth(), height());
    if (rect.contains(viewport()->rect())) { /* noop */ }
    updateViewportMargins();
    refreshSticky(); // Stage 11: kaydırınca kapsam şeridini tazele
}

void CodeEditor::focusInEvent(QFocusEvent* e) {
    QPlainTextEdit::focusInEvent(e);
    if (checkExternalChanged())
        emit externalChangeDetected(this);
}

void CodeEditor::mousePressEvent(QMouseEvent* e) {
    // Stage 17: Alt+sürükle → dikdörtgen blok seçim
    if ((e->modifiers() & Qt::AltModifier) && e->button() == Qt::LeftButton) {
        QTextCursor cur = cursorForPosition(e->pos());
        const QString line = cur.block().text();
        m_blockAnchorLine = cur.blockNumber();
        m_blockAnchorVis = BlockSelect::visualCol(line, cur.positionInBlock(), m_tab);
        m_blockSel = true;
        clearExtraCursors();
        setTextCursor(cur);
        e->accept();
        return;
    }
    if (e->modifiers() & Qt::AltModifier) {
        addCursorAt(cursorForPosition(e->pos()).position());
        e->accept();
        return;
    }
    if (e->button() == Qt::LeftButton) clearExtraCursors();
    QPlainTextEdit::mousePressEvent(e);
}

void CodeEditor::mouseMoveEvent(QMouseEvent* e) {
    if (m_blockSel && (e->buttons() & Qt::LeftButton)) {
        QTextCursor cur = cursorForPosition(e->pos());
        const QString line = cur.block().text();
        selectBlockColumn(m_blockAnchorLine, m_blockAnchorVis, cur.blockNumber(),
                          BlockSelect::visualCol(line, cur.positionInBlock(), m_tab),
                          m_tab);
        e->accept();
        return;
    }
    QPlainTextEdit::mouseMoveEvent(e);
}

void CodeEditor::mouseReleaseEvent(QMouseEvent* e) {
    m_blockSel = false;
    QPlainTextEdit::mouseReleaseEvent(e);
}

void CodeEditor::keyPressEvent(QKeyEvent* e) {
    // Stage 17: snippet durakları — Tab/Shift+Tab gezinir (en öncelikli)
    if (!m_snipStops.isEmpty()) {
        if (e->key() == Qt::Key_Tab && e->modifiers() == Qt::NoModifier) {
            nextSnippetStop(true);
            e->accept();
            return;
        }
        if (e->key() == Qt::Key_Backtab
            || (e->key() == Qt::Key_Tab && (e->modifiers() & Qt::ShiftModifier))) {
            nextSnippetStop(false);
            e->accept();
            return;
        }
        if (e->key() == Qt::Key_Escape) {
            clearSnippetStops();
            e->accept();
            return;
        }
    }
    // Stage 17: Alt+Shift+Yukarı/Aşağı — sütunu koruyarak imleç ekle
    if ((e->modifiers() & Qt::AltModifier) && (e->modifiers() & Qt::ShiftModifier)
        && (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down)) {
        addCursorAboveBelow(e->key() == Qt::Key_Up ? -1 : 1);
        e->accept();
        return;
    }
    // Stage 15: hayalet metin — düz Tab kabul, Esc vazgeç (öncelikli)
    if (!m_ghost.isEmpty()) {
        if (e->key() == Qt::Key_Tab && e->modifiers() == Qt::NoModifier) {
            QTextCursor c = textCursor();
            c.insertText(m_ghost);
            setTextCursor(c);
            clearGhost();
            e->accept();
            return;
        }
        if (e->key() == Qt::Key_Escape) {
            clearGhost();
            e->accept();
            return;
        }
    }
    // Escape: önce ek imleçleri temizle
    if (e->key() == Qt::Key_Escape && !m_extra.isEmpty()) {
        clearExtraCursors();
        e->accept();
        return;
    }
    if (m_preview && e->key() != Qt::Key_Escape) {
        QPlainTextEdit::keyPressEvent(e); // readOnly zaten engeller
        return;
    }
    // Tab / Shift+Tab: seçim girintile/çıkar, yoksa sekme/boşluk
    if (e->key() == Qt::Key_Tab && !(e->modifiers() & Qt::ControlModifier)) {
        QTextCursor c = textCursor();
        if (c.hasSelection()) {
            auto [first, last] = selectedLineRange();
            QStringList lines = toPlainText().split('\n');
            QString pad = m_useSpaces ? QString(m_tab, ' ') : "\t";
            for (int i = first; i <= last; ++i) lines[i].prepend(pad);
            int pos = cursorOffset();
            setPlainText(lines.join('\n'));
            setCursorOffset(pos + (last - first + 1) * pad.size());
            e->accept();
            return;
        }
        insertTextAtCursors(m_useSpaces ? QString(m_tab, ' ') : "\t");
        e->accept();
        return;
    }
    if (e->key() == Qt::Key_Backtab) {
        auto [first, last] = selectedLineRange();
        QStringList lines = toPlainText().split('\n');
        for (int i = first; i <= last; ++i) {
            if (lines[i].startsWith('\t')) lines[i].remove(0, 1);
            else {
                int n = 0;
                while (n < m_tab && n < lines[i].size() && lines[i][n] == ' ') ++n;
                lines[i].remove(0, n);
            }
        }
        int pos = cursorOffset();
        setPlainText(lines.join('\n'));
        setCursorOffset(qMax(0, pos - m_tab));
        e->accept();
        return;
    }
    // Enter: akıllı girinti
    if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) &&
        !(e->modifiers() & ~Qt::ShiftModifier)) {
        QTextCursor c = textCursor();
        QString prevLine = c.block().text().left(c.positionInBlock());
        QString indent = indentForNewLine(prevLine, m_tab, m_useSpaces);
        // Stage 17: kapanış parantezinden önce Enter → araya girintili satır
        const QString after = c.block().text().mid(c.positionInBlock());
        if (!m_extra.isEmpty()) {
            insertTextAtCursors("\n" + indent);
            e->accept();
            return;
        }
        const QString nl = AutoPairs::enterIndent(prevLine, after, indent, m_useSpaces,
                                                  m_tab);
        QPlainTextEdit::keyPressEvent(e);
        textCursor().insertText(nl.mid(1)); // baştaki \n zaten eklendi
        e->accept();
        return;
    }
    // Stage 17: otomatik çift kapatma + kapanıştan atlama + seçim sarma
    if (e->modifiers() == Qt::NoModifier && e->text().size() == 1 && m_extra.isEmpty()
        && !m_preview && !m_large) {
        AppSettings cfg17 = SettingsManager::instance().load();
        const QChar ch = e->text()[0];
        QTextCursor c = textCursor();
        const QString before = c.block().text().left(c.positionInBlock());
        const QString after = c.block().text().mid(c.positionInBlock());
        // Dize/yorum sezgisi (yaklaşık)
        const int quotes = before.count('"') - before.count("\\\"");
        const bool inStr = (quotes % 2) != 0;
        const int cmt = before.indexOf("//");
        const bool inCmt = cmt >= 0 && before.left(cmt).count('"') % 2 == 0;
        if (AutoPairs::isOpener(ch)) {
            if (c.hasSelection()) {
                const QString sel = c.selectedText();
                c.insertText(QString(ch) + sel + QString(AutoPairs::matching(ch)));
                e->accept();
                return;
            }
            AutoPairs::KeyResult r = AutoPairs::onOpen(
                ch, AutoPairs::matching(ch), before, after, inStr, inCmt,
                cfg17.autoClose);
            if (r.handled) {
                c.insertText(r.insert);
                for (int i = 0; i < r.cursorBack; ++i)
                    c.movePosition(QTextCursor::Left);
                setTextCursor(c);
                e->accept();
                return;
            }
        } else if ((ch == ')' || ch == ']' || ch == '}' || ch == '"' || ch == '\'')
                   && cfg17.autoClose && AutoPairs::shouldSkip(ch, after)) {
            c.movePosition(QTextCursor::Right);
            setTextCursor(c);
            e->accept();
            return;
        }
    }
    // '}' otomatik geri-girinti: satırda sadece boşluk varsa bir seviye sil
    if (e->text() == "}" && m_useSpaces) {
        QTextCursor c = textCursor();
        QString before = c.block().text().left(c.positionInBlock());
        if (!before.isEmpty() && before.trimmed().isEmpty() && before.size() >= m_tab) {
            c.movePosition(QTextCursor::StartOfBlock);
            for (int i = 0; i < m_tab; ++i) c.deleteChar();
            setTextCursor(c);
        }
    }
    // Yazdırılabilir metin + ek imleç: hepsine yaz
    if (!m_extra.isEmpty() && !e->text().isEmpty() && e->modifiers() == Qt::NoModifier) {
        insertTextAtCursors(e->text());
        e->accept();
        return;
    }
    if (!m_extra.isEmpty() &&
        (e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete)) {
        QList<QTextCursor> all;
        all << textCursor() << m_extra;
        std::sort(all.begin(), all.end(),
                  [](const QTextCursor& a, const QTextCursor& b) { return a.position() > b.position(); });
        textCursor().beginEditBlock();
        for (QTextCursor c : all) {
            setTextCursor(c);
            if (e->key() == Qt::Key_Backspace) c.deletePreviousChar();
            else c.deleteChar();
        }
        textCursor().endEditBlock();
        refreshExtraSelections();
        e->accept();
        return;
    }
    // Basit otomatik kapatma: ( { [ " '
    static const QMap<QString, QString> pairs = {
        {"(", ")"}, {"{", "}"}, {"[", "]"}, {"\"", "\""}, {"'", "'"}};
    QPlainTextEdit::keyPressEvent(e);
    if (e->text().isEmpty()) return;
    QString t = e->text();
    if (pairs.contains(t) && !(t == "\"" || t == "'")) {
        QTextCursor c = textCursor();
        if (c.hasSelection()) {
            QString sel = c.selectedText();
            c.insertText(t + sel + pairs[t]);
        } else {
            c.insertText(pairs[t]);
            c.movePosition(QTextCursor::Left);
            setTextCursor(c);
        }
        clearExtraCursors();
    }
}

void CodeEditor::paintEvent(QPaintEvent* e) {
    QPlainTextEdit::paintEvent(e);
    // Stage 23: blok / alt çizgi imleç (yerel çizim)
    if (m_cursorStyle != "bar" && hasFocus() && !textCursor().hasSelection() && m_blinkOn
        && !m_large && !m_preview) {
        const QRect cr = cursorRect();
        QPainter p(viewport());
        const QColor c = ThemeManager::instance().tokens().textStrong;
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        if (m_cursorStyle == "block") {
            const int w = qMax(4, fontMetrics().horizontalAdvance(QLatin1Char('M')));
            p.drawRect(cr.x(), cr.y(), w, cr.height());
        } else { // underline
            p.drawRect(cr.x(), cr.y() + cr.height() - 2, qMax(4, cr.width()), 2);
        }
    }
    if (m_large) return; // büyük dosyada kılavuz çizme
    const ThemeTokens tk = ThemeManager::instance().tokens();
    QPainter p(viewport());
    QFontMetrics fm = fontMetrics();
    const int tabPx = fm.horizontalAdvance(' ') * m_tab;
    if (tabPx <= 0) return;
    const double x0 = contentOffset().x() + document()->documentMargin();

    // Stage 11: imleç satırının girinti seviyesi (aktif kılavuz için)
    int activeLevel = 0;
    {
        QString ct = textCursor().block().text();
        int sp = 0;
        for (QChar ch : ct) {
            if (ch == ' ') ++sp;
            else if (ch == '\t') sp += m_tab;
            else break;
        }
        activeLevel = sp / qMax(1, m_tab);
    }

    QTextBlock b = firstVisibleBlock();
    int guard = 0;
    while (b.isValid() && guard++ < 500) {
        QRectF r = blockBoundingGeometry(b).translated(contentOffset());
        if (r.top() > e->rect().bottom()) break;
        if (b.isVisible() && r.bottom() >= e->rect().top()) {
            QString t = b.text();
            int spaces = 0;
            for (QChar ch : t) {
                if (ch == ' ') ++spaces;
                else if (ch == '\t') spaces += m_tab;
                else break;
            }
            int levels = spaces / qMax(1, m_tab);
            for (int l = 1; l <= levels; ++l) {
                const int x = (int)(x0 + l * tabPx);
                // Stage 11: aktif seviye vurgulu kılavuz
                p.setPen(l == activeLevel && activeLevel > 0
                             ? ThemeTokens::withAlphaF(tk.text, 0.28)
                             : tk.indentGuide);
                p.drawLine(x, (int)r.top(), x, (int)r.bottom());
            }
            // Stage 11: CRLF dosyalarında satır sonu işareti
            if (m_crlf && !t.isEmpty()) {
                QTextCursor ec(b);
                ec.movePosition(QTextCursor::EndOfBlock);
                const int ex = cursorRect(ec).right() + 2;
                p.setPen(tk.textDim);
                p.drawText(ex, (int)r.top(), 20, fm.height(), Qt::AlignLeft, "␍");
            }
        }
        b = b.next();
    }

    // Stage 11: sütun cetveli
    if (m_rulerCol > 0) {
        const int rx = (int)(x0 + m_rulerCol * fm.horizontalAdvance(' '));
        p.setPen(ThemeTokens::withAlphaF(tk.textDim, 0.55));
        p.drawLine(rx, 0, rx, viewport()->height());
    }

    // Stage 11: overview ruler — arama isabetleri (sağ kenar şeridi)
    if (!m_searchHits.isEmpty()) {
        const int rw = 8;
        const int rx = viewport()->width() - rw - 2;
        p.fillRect(rx - 1, 0, rw + 2, viewport()->height(),
                   ThemeTokens::withAlphaF(tk.text, 0.05));
        for (int i = 0; i < m_searchHits.size(); ++i) {
            const FindHit& h = m_searchHits[i];
            QTextBlock bb = document()->findBlockByNumber(h.line0);
            if (!bb.isValid()) continue;
            const double y = blockBoundingGeometry(bb).translated(contentOffset()).top();
            if (y < -4 || y > viewport()->height() + 4) continue;
            p.fillRect(rx, (int)y, rw, 3,
                       i == m_searchCur ? tk.textStrong
                                        : ThemeTokens::withAlphaF(tk.accent, 0.75));
        }
    }

    // Stage 15: hayalet tamamlama (imleç sonrası soluk metin)
    if (!m_large && !m_ghost.isEmpty() && hasFocus()) {
        const QRect cr = cursorRect(textCursor());
        p.setPen(ThemeTokens::withAlphaF(tk.textDim, 0.85));
        QFont gf = font();
        gf.setItalic(true);
        p.setFont(gf);
        const int lh = fm.height();
        int y = cr.top();
        const QStringList gl = m_ghost.split('\n');
        for (int i = 0; i < gl.size() && i < 3; ++i) {
            const int x = (i == 0) ? cr.left() : (int)(contentOffset().x() + 4);
            p.drawText(x, y, viewport()->width() - x - 8, lh,
                       Qt::AlignLeft, gl[i].left(120));
            y += lh;
        }
    }

    // Stage 13: inlay hints — satır sonu / konum etiketleri (soluk metin)
    if (!m_large && !m_inlay.isEmpty()) {
        p.setPen(ThemeTokens::withAlphaF(tk.textDim, 0.9));
        QFont hf = font();
        hf.setPointSize(qMax(7, hf.pointSize() - 1));
        hf.setItalic(true);
        p.setFont(hf);
        QTextBlock b2 = firstVisibleBlock();
        int guard2 = 0;
        while (b2.isValid() && guard2++ < 300) {
            QRectF r = blockBoundingGeometry(b2).translated(contentOffset());
            if (r.top() > e->rect().bottom()) break;
            if (b2.isVisible() && r.bottom() >= e->rect().top()) {
                QStringList tags;
                for (const InlayHint& h : std::as_const(m_inlay)) {
                    if (h.line != b2.blockNumber()) continue;
                    tags << h.label.left(24);
                    if (tags.size() >= 4) break;
                }
                // Stage 24: blame hayaleti (imleç satırında, en sonda)
                for (const InlayHint& h : std::as_const(m_blame)) {
                    if (h.line != b2.blockNumber()) continue;
                    tags << ("◷ " + h.label.left(40));
                }
                // Stage 27: code lens (gönderme sayısı)
                for (const InlayHint& h : std::as_const(m_lens)) {
                    if (h.line != b2.blockNumber()) continue;
                    tags << ("↗" + h.label.left(24));
                    break;
                }
                if (!tags.isEmpty()) {
                    const QString txt = QString("  %1").arg(tags.join("  "));
                    p.drawText((int)(r.left() + r.width()) - 220, (int)r.top(),
                               220, fm.height(), Qt::AlignRight, txt.left(60));
                }
            }
            b2 = b2.next();
        }
    }
}

void CodeEditor::contextMenuEvent(QContextMenuEvent* e) {
    QMenu* menu = createStandardContextMenu();
    QTextCursor c = cursorForPosition(e->pos());
    int st = 0;
    QString w = wordAt(toPlainText(), c.position(), st);
    if (!w.isEmpty() && spellChecker()) {
        // İmleçteki kelime yorum/string içinde ve yanlışsa öner
        bool inside = false;
        for (const auto& sp : Spelling::commentStringSpans(toPlainText()))
            if (c.position() >= sp.first && c.position() <= sp.first + sp.second) {
                inside = true;
                break;
            }
        if (inside && !m_spell->check(w) && !m_spell->check(w.toLower())) {
            QStringList sug = m_spell->suggest(w, 5);
            if (!sug.isEmpty()) {
                menu->addSeparator();
                for (const QString& s : sug) {
                    QAction* a = menu->addAction(s);
                    int at = st, len = w.size();
                    connect(a, &QAction::triggered, this, [this, at, len, s]() {
                        QTextCursor cc(document());
                        cc.setPosition(at);
                        cc.setPosition(at + len, QTextCursor::KeepAnchor);
                        cc.insertText(s);
                    });
                }
            }
        }
    }
    menu->addSeparator();
    menu->addAction("Katla/Aç", this, [this, c]() {
        setTextCursor(c);
        toggleFoldAtLine(c.blockNumber());
    });
    menu->addAction("Sonraki Eşleşmeyi Seç (Ctrl+D)", this, [this]() { selectNextOccurrence(); });
    menu->exec(e->globalPos());
    delete menu;
}

SpellChecker* CodeEditor::spellChecker() {
    if (m_spell) return m_spell->isOk() ? m_spell : nullptr;
    AppSettings cfg = SettingsManager::instance().load();
    if (cfg.spellLang == "off") return nullptr;
    m_spell = new SpellChecker();
    QStringList langs;
    // Stage 21: dile göre sözlük — düz metin/belge → ayar; kod → önce İngilizce
    const QString suf = QFileInfo(m_path).suffix().toLower();
    const bool prose = suf == "md" || suf == "markdown" || suf == "txt" || m_path.isEmpty();
    auto cfgLangs = [&] {
        if (cfg.spellLang == "auto") return QStringList{"tr_TR", "en_US"};
        return QStringList{cfg.spellLang};
    };
    if (prose) langs = cfgLangs();
    else {
        langs << "en_US";
        for (const QString& l : cfgLangs())
            if (!langs.contains(l)) langs << l;
    }
    for (const QString& l : langs)
        if (m_spell->load(l)) return m_spell;
    return nullptr;
}

// ---------- Seçim vurguları (aktif satır + diagnostic + bracket + ek imleç) ----------
void CodeEditor::highlightCurrentLine() {
    refreshExtraSelections();
    if (m_focusMode && hasFocus()) centerCursor(); // Stage 12: daktilo modu
}

void CodeEditor::refreshExtraSelections() {
    QList<QTextEdit::ExtraSelection> sel;
    const ThemeTokens tk = ThemeManager::instance().tokens();
    const AppSettings as = SettingsManager::instance().load();
    if (!m_large && as.lineHighlightOn) {
        QTextEdit::ExtraSelection s;
        // Stage 23: satır vurgusu opaklığı
        QColor bg = tk.lineHighlight;
        bg.setAlphaF(qBound(0.05, bg.alphaF() * m_lineHiOpacity, 1.0));
        s.format.setBackground(bg);
        s.format.setProperty(QTextFormat::FullWidthSelection, true);
        s.cursor = textCursor();
        s.cursor.clearSelection();
        sel.append(s);
    }
    QTextCharFormat err, warn;
    err.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    err.setUnderlineColor(tk.error);
    warn.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    warn.setUnderlineColor(tk.warning);
    for (const LspDiag& d : m_diags) {
        QTextBlock b = document()->findBlockByNumber(d.line);
        if (!b.isValid()) continue;
        QTextCursor c(b);
        int len = b.length() - 1;
        int sc = qBound(0, d.col, qMax(0, len));
        int ec = (d.endLine == d.line) ? qBound(sc + 1, d.endCol, qMax(1, len)) : len;
        c.setPosition(b.position() + sc);
        c.setPosition(b.position() + qMax(sc + 1, ec), QTextCursor::KeepAnchor);
        QTextEdit::ExtraSelection s;
        s.format = (d.severity == 1) ? err : warn;
        s.cursor = c;
        sel.append(s);
    }
    // Bracket eşleşme
    if (!m_large) {
        QString doc = toPlainText();
        QTextCursor cur = textCursor();
        int cand[2] = {cur.position() - 1, cur.position()};
        for (int k = 0; k < 2; ++k) {
            int mate = findMatchingBracket(doc, cand[k]);
            if (mate >= 0) {
                const QColor cols[3] = {tk.bracket, tk.synType, tk.synKeyword};
                // Derinlik: eşleşen çiftin açılışına kadar say
                int open = qMin(cand[k], mate);
                QChar o = doc[open];
                int depth = 0;
                for (int i = 0; i <= open && i < 200000; ++i)
                    if (doc[i] == o) ++depth;
                QTextCharFormat bf;
                bf.setBackground(ThemeTokens::withAlphaF(cols[depth % 3], 0.30));
                bf.setForeground(cols[depth % 3]);
                for (int pp : {cand[k], mate}) {
                    QTextCursor bc(document());
                    bc.setPosition(pp);
                    bc.setPosition(pp + 1, QTextCursor::KeepAnchor);
                    QTextEdit::ExtraSelection s;
                    s.format = bf;
                    s.cursor = bc;
                    sel.append(s);
                }
                break;
            }
        }
    }
    // Ek imleçler: accent renkli seçim
    QTextCharFormat mc;
    mc.setBackground(tk.accent);
    mc.setForeground(tk.dark ? tk.textStrong : tk.bg);
    for (const QTextCursor& ec : std::as_const(m_extra)) {
        QTextCursor c = ec;
        if (!c.hasSelection()) c.setPosition(c.position() + 1, QTextCursor::KeepAnchor);
        if (c.position() > document()->characterCount() - 1) continue;
        QTextEdit::ExtraSelection s;
        s.format = mc;
        s.cursor = c;
        sel.append(s);
    }
    // Stage 11: gökkuşağı parantezler (önbellekten, cap'li)
    if (!m_large && !m_bracketMarks.isEmpty()) {
        int added = 0;
        for (const BracketMark& mk : std::as_const(m_bracketMarks)) {
            if (mk.pos < 0 || mk.pos >= document()->characterCount()) continue;
            QTextCharFormat bf;
            // Stage 23: parantez vurgu stili
            if (m_bracketStyle == "zemin") {
                QColor bg = tk.accent;
                bg.setAlphaF(0.30);
                bf.setBackground(bg);
            } else if (m_bracketStyle == "altcizgi") {
                bf.setUnderlineStyle(QTextCharFormat::SingleUnderline);
                bf.setUnderlineColor(tk.accent);
            } else {
                bf.setForeground(BracketDepth::colorFor(mk.depth, tk.dark));
                QFont bfFont = font();
                bfFont.setBold(true);
                bf.setFont(bfFont);
            }
            QTextCursor bc(document());
            bc.setPosition(mk.pos);
            bc.setPosition(mk.pos + 1, QTextCursor::KeepAnchor);
            QTextEdit::ExtraSelection s;
            s.format = bf;
            s.cursor = bc;
            sel.append(s);
            if (++added >= 1500) break;
        }
    }
    // Stage 11: arama isabetleri (tümü + aktif)
    if (!m_searchHits.isEmpty()) {
        for (int i = 0; i < m_searchHits.size(); ++i) {
            const FindHit& h = m_searchHits[i];
            if (h.start < 0 || h.end > document()->characterCount()) continue;
            QTextCharFormat sf;
            if (i == m_searchCur) {
                sf.setBackground(tk.accent);
                sf.setForeground(tk.dark ? tk.textStrong : tk.bg);
            } else {
                sf.setBackground(ThemeTokens::withAlphaF(tk.accent, 0.30));
            }
            QTextCursor hc(document());
            hc.setPosition(h.start);
            hc.setPosition(h.end, QTextCursor::KeepAnchor);
            QTextEdit::ExtraSelection s;
            s.format = sf;
            s.cursor = hc;
            sel.append(s);
        }
    }
    // Stage 11: hedef satır flaşı
    if (m_flashLine >= 0) {
        QTextBlock fb = document()->findBlockByNumber(m_flashLine);
        if (fb.isValid()) {
            QTextEdit::ExtraSelection s;
            s.format.setBackground(ThemeTokens::withAlphaF(tk.accent, 0.35));
            s.format.setProperty(QTextFormat::FullWidthSelection, true);
            QTextCursor fc(fb);
            s.cursor = fc;
            sel.append(s);
        }
    }
    // Stage 13: semantik token vurguları (legend → rol → tema rengi)
    if (!m_large && as.semanticHighlight && !m_semantic.isEmpty()) {
        int added = 0;
        for (const SemanticToken& t : std::as_const(m_semantic)) {
            QTextBlock bb = document()->findBlockByNumber(t.line);
            if (!bb.isValid()) continue;
            const int blen = bb.length() - 1;
            if (t.col < 0 || t.col >= blen) continue;
            const QString tname = (t.type >= 0 && t.type < m_semLegend.size())
                ? m_semLegend[t.type] : QString();
            const QString role = SemanticTokens::roleFor(tname);
            QColor fg;
            if (role == "keyword") fg = tk.synKeyword;
            else if (role == "string") fg = tk.synString;
            else if (role == "comment") fg = tk.synComment;
            else if (role == "number") fg = tk.synNumber;
            else if (role == "func") fg = tk.synFunc;
            else if (role == "type") fg = tk.synType;
            else continue; // var/other: varsayılan renklendirme yeterli
            QTextCursor sc(document());
            sc.setPosition(bb.position() + t.col);
            sc.setPosition(bb.position() + qMin(blen, t.col + t.len),
                           QTextCursor::KeepAnchor);
            QTextEdit::ExtraSelection s;
            QTextCharFormat sf;
            sf.setForeground(fg);
            s.format = sf;
            s.cursor = sc;
            sel.append(s);
            if (++added >= 2000) break;
        }
    }
    // Stage 14: kapsama ısı haritası (yeşil=çalıştı, kırmızı=kapsanmıyor)
    if (!m_large && !m_coverage.isEmpty()) {
        for (auto it = m_coverage.begin(); it != m_coverage.end(); ++it) {
            QTextBlock bb = document()->findBlockByNumber(it.key() - 1);
            if (!bb.isValid()) continue;
            QTextEdit::ExtraSelection s;
            s.format.setBackground(it.value() > 0
                ? ThemeTokens::withAlphaF(tk.success, 0.18)
                : ThemeTokens::withAlphaF(tk.error, 0.22));
            s.format.setProperty(QTextFormat::FullWidthSelection, true);
            QTextCursor cc(bb);
            s.cursor = cc;
            sel.append(s);
        }
    }
    setExtraSelections(sel);
}

void CodeEditor::setDiagnostics(const QList<LspDiag>& diags) {
    m_diags = diags;
    refreshExtraSelections();
}

void CodeEditor::clearDiagnostics() {
    if (!m_diags.isEmpty()) { m_diags.clear(); refreshExtraSelections(); }
}

void CodeEditor::gotoLine(int line) {
    QTextCursor c = textCursor();
    c.movePosition(QTextCursor::Start);
    if (line > 1) c.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, line - 1);
    setTextCursor(c);
    centerCursor();
    setFocus();
    flashLine(line - 1); // Stage 11: hedef satır vurgusu
}

// ---------- Stage 11: görsel derinlik ----------

void CodeEditor::refreshRainbow() {
    m_bracketMarks.clear();
    if (m_large || m_preview) return;
    if (document()->characterCount() > 150000) return; // büyük belgede kapalı
    m_bracketMarks = BracketDepth::marks(toPlainText());
    refreshExtraSelections();
}

void CodeEditor::setShowWhitespace(bool on) {
    QTextOption opt = document()->defaultTextOption();
    QTextOption::Flags f = opt.flags();
    if (on) f |= QTextOption::ShowTabsAndSpaces;
    else f &= ~QTextOption::ShowTabsAndSpaces;
    opt.setFlags(f);
    document()->setDefaultTextOption(opt);
}

void CodeEditor::setRulerColumn(int col) {
    m_rulerCol = qMax(0, col);
    viewport()->update();
}

void CodeEditor::setFindHits(const QList<FindHit>& hits, int current) {
    m_searchHits = hits;
    m_searchCur = current;
    refreshExtraSelections();
    viewport()->update();
}

void CodeEditor::clearFindHits() {
    if (m_searchHits.isEmpty() && m_searchCur < 0) return;
    m_searchHits.clear();
    m_searchCur = -1;
    refreshExtraSelections();
    viewport()->update();
}

void CodeEditor::flashLine(int line0) {
    m_flashLine = line0;
    refreshExtraSelections();
    QTimer::singleShot(900, this, [this, line0]() {
        if (m_flashLine == line0) {
            m_flashLine = -1;
            refreshExtraSelections();
        }
    });
}

// ---------- Stage 13: dil zekâsı katmanları ----------

void CodeEditor::setSemanticTokens(const QList<SemanticToken>& toks,
                                   const QStringList& legend) {
    m_semantic = toks;
    m_semLegend = legend;
    refreshExtraSelections();
}

void CodeEditor::clearSemanticTokens() {
    if (m_semantic.isEmpty()) return;
    m_semantic.clear();
    refreshExtraSelections();
}

void CodeEditor::setInlayHints(const QList<InlayHint>& hints) {
    m_inlay = hints;
    viewport()->update();
}

void CodeEditor::clearInlayHints() {
    if (m_inlay.isEmpty()) return;
    m_inlay.clear();
    viewport()->update();
}

// ---------- Stage 14: hata ayıklama + kapsama ----------

void CodeEditor::setBreakpoints(const QSet<int>& lines1) {
    m_breakpoints = lines1;
    m_gutter->update();
}

void CodeEditor::setFrameLine(int line0) {
    m_frameLine = line0;
    refreshExtraSelections();
    m_gutter->update();
    if (line0 >= 0) {
        // Çerçeveyi görünür yap
        QTextBlock b = document()->findBlockByNumber(line0);
        if (b.isValid()) {
            QTextCursor c(b);
            setTextCursor(c);
            centerCursor();
            // İmleci geri alma: seçim yok, sadece kaydırma istenirdi — imleç
            // hareketi kabul edilebilir (VS Code da satıra gider)
        }
    }
}

void CodeEditor::setCoverage(const QMap<int, int>& hits1) {
    m_coverage = hits1;
    refreshExtraSelections();
}

void CodeEditor::setGhostText(const QString& ghost, const QString& prefix) {
    m_ghost = ghost;
    m_ghostPrefix = prefix;
    viewport()->update();
}

void CodeEditor::clearGhost() {
    if (m_ghost.isEmpty()) return;
    m_ghost.clear();
    m_ghostPrefix.clear();
    viewport()->update();
}

QString CodeEditor::completionPrefix() const {
    QTextCursor c = textCursor();
    const QString line = c.block().text();
    int i = c.positionInBlock() - 1;
    QString pre;
    while (i >= 0 && (line[i].isLetterOrNumber() || line[i] == '_')) {
        pre.prepend(line[i]);
        --i;
    }
    return pre;
}

void CodeEditor::expandSelection() {
    QTextCursor c = textCursor();
    auto [ns, ne] = SelectionGrow::grow(toPlainText(), c.selectionStart(), c.selectionEnd());
    c.setPosition(ns);
    c.setPosition(ne, QTextCursor::KeepAnchor);
    setTextCursor(c);
}

void CodeEditor::refreshSticky() {
    if (!m_sticky) return;
    const AppSettings as = SettingsManager::instance().load();
    const bool want = as.stickyScroll && !m_large && !m_preview && blockCount() <= 20000;
    if (!want) {
        if (m_sticky->isVisible()) {
            m_sticky->hide();
            updateViewportMargins();
        }
        return;
    }
    const int curLine = textCursor().blockNumber();
    const QList<ScopeFrame> frames =
        ScopeChain::chain(toPlainText().split('\n'), curLine, m_tab);
    const int firstVis = firstVisibleBlock().blockNumber();
    if (frames.isEmpty() || frames.first().line0 >= firstVis) {
        if (m_sticky->isVisible()) {
            m_sticky->hide();
            updateViewportMargins();
        }
        return;
    }
    m_stickyLine = frames.last().line0; // en iç kapsam
    const QString label = ScopeChain::label(frames);
    if (m_sticky->text() != label) m_sticky->setText(label);
    m_sticky->setToolTip(QString("Satır %1: %2").arg(m_stickyLine + 1).arg(label));
    QFont sf = font();
    sf.setPointSize(qMax(8, font().pointSize() - 1));
    m_sticky->setFont(sf);
    if (!m_sticky->isVisible()) {
        m_sticky->show();
        updateViewportMargins();
    }
}

// ---------- Stage 17: snippet + blok + sunucu katlamaları ----------


bool CodeEditor::insertSnippet(const QString& body, const QString& fileName) {
    QTextCursor c = textCursor();
    clearSnippetStops();
    // Seçili önek kelimeyi sil (palet çağrısında imleç kelime sonundadır)
    QTextCursor w = c;
    w.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
    const QString word = w.selectedText();
    if (!word.isEmpty() && word[0].isLetter()) {
        // Snippet önekiyle bitiyorsa tamamını sil
        c.setPosition(w.selectionStart());
        setTextCursor(c);
    }
    QTextCursor sel = textCursor();
    sel.select(QTextCursor::WordUnderCursor);
    const QString fn = fileName.isEmpty() ? QFileInfo(m_path).fileName() : fileName;
    QString clip;
    if (auto* cb = QApplication::clipboard()) clip = cb->text().left(2000);
    QString work = SnippetEngine::expandVars(body, fn, sel.selectedText());
    SnippetExpand ex = SnippetEngine::expand(work, c.position(), clip);
    c.insertText(ex.text);
    setTextCursor(c);
    if (!ex.stopOffsets.isEmpty()) {
        m_snipStops = ex.stopOffsets;
        m_snipIndex = 0;
        QTextCursor s(document());
        s.setPosition(m_snipStops[0]);
        // Yer tutucu metni seçili getir (üzerine yazılsın)
        if (!ex.stopNames.isEmpty() && !ex.stopNames[0].isEmpty())
            s.setPosition(m_snipStops[0] + ex.stopNames[0].size(),
                          QTextCursor::KeepAnchor);
        setTextCursor(s);
    }
    return true;
}

void CodeEditor::nextSnippetStop(bool forward) {
    if (m_snipStops.isEmpty()) return;
    m_snipIndex = forward ? (m_snipIndex + 1) : (m_snipIndex - 1);
    if (m_snipIndex >= m_snipStops.size() || m_snipIndex < 0) {
        clearSnippetStops();
        return;
    }
    QTextCursor s(document());
    s.setPosition(m_snipStops[m_snipIndex]);
    setTextCursor(s);
    ensureCursorVisible();
}

void CodeEditor::clearSnippetStops() {
    m_snipStops.clear();
    m_snipIndex = -1;
}

void CodeEditor::applyServerFolds(const QList<QPair<int, int>>& ranges) {
    m_serverFolds = ranges;
    refreshFolds();
}

void CodeEditor::selectBlockColumn(int lineA, int visA, int lineB, int visB,
                                   int tabWidth) {
    const auto spans = BlockSelect::spans(lineA, visA, lineB, visB);
    if (spans.isEmpty()) return;
    m_extra.clear();
    bool first = true;
    for (const BlockSpan& s : spans) {
        QTextBlock b = document()->findBlockByNumber(s.line0);
        if (!b.isValid()) continue;
        const int off = BlockSelect::charCol(b.text(), s.colB, tabWidth);
        QTextCursor c(b);
        c.setPosition(b.position() + off);
        if (first) { setTextCursor(c); first = false; }
        else m_extra.append(c);
    }
    refreshExtraSelections();
}

void CodeEditor::addCursorAboveBelow(int dir) {
    QTextCursor c = textCursor();
    const int line = c.blockNumber() + dir;
    if (line < 0 || line >= blockCount()) return;
    QTextBlock b = document()->findBlockByNumber(line);
    const int wantVis = BlockSelect::visualCol(c.block().text(),
                                               c.positionInBlock(), m_tab);
    const int off = BlockSelect::charCol(b.text(), wantVis, m_tab);
    QTextCursor n(b);
    n.setPosition(b.position() + off);
    m_extra.append(n);
    setTextCursor(n);
    refreshExtraSelections();
}
