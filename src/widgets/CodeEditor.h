#pragma once
#include "../core/BracketDepth.h"
#include "../core/InlayHintList.h"
#include "../core/LspClient.h"
#include "../core/SearchMarks.h"
#include "../core/SemanticTokens.h"
#include "../core/SpellChecker.h"
#include <QDateTime>
#include <QMap>
#include <QPlainTextEdit>
#include <QSet>
#include <QSyntaxHighlighter>

class CodeHighlighter : public QSyntaxHighlighter {
public:
    explicit CodeHighlighter(QTextDocument* doc);
protected:
    void highlightBlock(const QString& text) override;
};

struct FoldRange {
    int start = 0; // 0-based, başlık satırı (görünür kalır)
    int end = 0;   // 0-based, dahil (gizlenir)
};

// Satır numaralı VS Code tarzı editör.
// Stage 1: harici değişiklik, otomatik parantez, font/sekme.
// Stage 4: diagnostic + büyük dosya. Stage 5: çoklu imleç, katlama,
// akıllı girinti, bracket eşleşme, girinti kılavuzu, satır işlemleri,
// editorconfig, salt-okunur önizleme.
// Stage 11: gökkuşağı parantez, zengin gutter, sticky scroll, arama
// vurguları + overview ruler, boşluk/cetvel, satır flaşı, akıllı seçim.
class CodeEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit CodeEditor(QWidget* parent = nullptr);
    ~CodeEditor() override;
    QString filePath() const { return m_path; }
    void setFilePath(const QString& p) { m_path = p; }
    bool loadFile(const QString& path);
    bool saveFile(const QString& path = QString());
    // Stage 16: uzak belge — içerik ssh ile gelir, yol "ssh://..." biçimindedir
    bool isRemote() const { return m_path.startsWith("ssh://"); }
    // Stage 16: önişlemci içeriği uzaktan gelen belge (kaydetme MainWindow'da)
    void setContent(const QString& path, const QString& text) {
        m_path = path;
        m_preview = false;
        setReadOnly(false);
        setPlainText(text);
        document()->setModified(false);
        m_lastMtime = QDateTime::currentDateTimeUtc();
        m_extra.clear();
        refreshFolds();
    }
    int gutterWidth() const;
    void gutterPaintEvent(QPaintEvent* ev);
    void gutterClick(int y);
    void setGutterHover(int line0); // -1 = yok

    // Stage 1 API
    void applyEditorSettings(int fontSize, int tabWidth);
    void applyLineHeight(double lineHeight); // Stage 9
    void refreshTheme();                     // Stage 9: tema/accent değişimi
    bool checkExternalChanged() const;
    bool reloadFromDisk();
    int cursorOffset() const;
    void setCursorOffset(int pos);
    QString lineEnding() const { return m_crlf ? "CRLF" : "LF"; }

    // Stage 4 API
    void setDiagnostics(const QList<LspDiag>& diags);
    void clearDiagnostics();
    bool largeFileMode() const { return m_large; }
    void gotoLine(int line); // 1-based

    // Stage 5: çoklu imleç
    int extraCursorCount() const { return m_extra.size(); }
    void addCursorAt(int pos);
    bool selectNextOccurrence();
    void selectAllOccurrences();
    void clearExtraCursors();
    void insertTextAtCursors(const QString& t);

    // Stage 5: katlama
    static QList<FoldRange> detectFoldRanges(const QString& text);
    void toggleFoldAtLine(int line0);
    void foldAtCursor();
    void unfoldAtCursor();
    void foldAll();
    void unfoldAll();
    QList<int> foldedStartLines() const;
    void setFoldedLines(const QList<int>& starts);

    // Stage 5: girinti + bracket
    static QString indentForNewLine(const QString& prevLine, int tabWidth, bool useSpaces);
    static int findMatchingBracket(const QString& text, int pos);
    static QString leadingIndent(const QString& line, int tabWidth);

    // Stage 5: satır işlemleri
    void moveLineOrSelection(int dir); // -1 yukarı, +1 aşağı
    void duplicateLineOrSelection();
    void sortSelectedLines();
    void trimTrailingWhitespace();

    // Stage 5: önizleme (çok büyük dosyalar)
    bool loadPreview(const QString& path, int maxLines = 2000);
    bool previewMode() const { return m_preview; }
    bool loadFullPreview();
    SpellChecker* spellChecker();

    // Stage 11: görsel derinlik
    void setGitMarks(const QMap<int, char>& marks); // 1-based satır → a/m/d
    void setShowWhitespace(bool on);
    void setRulerColumn(int col);                   // 0 = kapalı
    void setFindHits(const QList<struct FindHit>& hits, int current);
    void clearFindHits();
    void flashLine(int line0);                      // hedef satır vurgusu
    void expandSelection();                         // akıllı seçim büyüt
    void refreshSticky();                           // yapışkan kapsam şeridi

    // Stage 12: odak (daktilo) modu — imleç satırı dikeyde ortalanır
    void setFocusMode(bool on) { m_focusMode = on; }
    bool focusMode() const { return m_focusMode; }

    // Stage 13: dil zekâsı — semantik token + inlay hint katmanları
    void setSemanticTokens(const QList<struct SemanticToken>& toks,
                           const QStringList& legend);
    void clearSemanticTokens();
    void setInlayHints(const QList<struct InlayHint>& hints);
    void clearInlayHints();
    QString completionPrefix() const; // imleçteki kelime öneki

    // Stage 14: hata ayıklama + kapsama
    void setBreakpoints(const QSet<int>& lines1); // kesme noktası satırları
    void setFrameLine(int line0);                 // -1 = yok (çalışan çerçeve oku)
    void setCoverage(const QMap<int, int>& hits1);// satır → çalıştırma sayısı

    // Stage 15: hayalet tamamlama (Tab kabul, Esc vazgeç)
    void setGhostText(const QString& ghost, const QString& prefix);
    void clearGhost();
    bool hasGhost() const { return !m_ghost.isEmpty(); }
    QString ghostPrefix() const { return m_ghostPrefix; }

    // Stage 17: snippet + sunucu katlamaları + blok seçim + hunk
    bool insertSnippet(const QString& body, const QString& fileName = QString());
    bool snippetActive() const { return !m_snipStops.isEmpty(); }
    void nextSnippetStop(bool forward = true);
    void clearSnippetStops();
    void applyServerFolds(const QList<QPair<int, int>>& ranges);
    QMap<int, char> gitMarks() const { return m_gitMarks; }
    void selectBlockColumn(int lineA, int visA, int lineB, int visB, int tabWidth = 4);
    void addCursorAboveBelow(int dir); // Alt+Shift+Up/Down: sütunu koru

signals:
    void externalChangeDetected(CodeEditor* editor);
    void breakpointToggleRequested(int line1); // Stage 14: gutter tıklaması

protected:
    void resizeEvent(QResizeEvent* e) override;
    void focusInEvent(QFocusEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void paintEvent(QPaintEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* e) override;

private slots:
    void updateLineNumbers(const QRect& rect, int dy);
    void highlightCurrentLine();
    void refreshExtraSelections();
    void refreshFolds();
    void refreshRainbow(); // Stage 11: gökkuşağı parantez önbelleği

private:
    QPair<int, int> selectedLineRange() const;
    void applyFoldVisibility();
    void updateViewportMargins(); // Stage 11: gutter + sticky üst boşluğu

public: // Gutter sınıfı erişir
    int lineAtY(int y) const;     // gutter hover için
private:

    QWidget* m_gutter;
    QString m_path;
    CodeHighlighter* m_highlighter;
    QDateTime m_lastMtime;
    bool m_crlf = false;
    bool m_large = false;
    QList<LspDiag> m_diags;

    // Stage 5
    QList<QTextCursor> m_extra;
    QSet<int> m_foldStarts;   // katlanabilir başlıklar (0-based)
    QMap<int, int> m_foldEnds;  // Stage 17: başlık → bitiş (girinti ya da LSP)
    QSet<int> m_folded;       // katlı başlıklar
    int m_tab = 4;
    bool m_useSpaces = true;
    bool m_preview = false;
    SpellChecker* m_spell = nullptr;
    QString m_ecEol;          // editorconfig hedefi ("" = dosyadaki)
    bool m_ecTrim = false;
    bool m_ecFinalNl = false;

    // Stage 11
    QMap<int, char> m_gitMarks;              // 1-based satır → a/m/d
    int m_hoverLine = -1;                    // gutter hover satırı (0-based)
    QList<struct FindHit> m_searchHits;
    int m_searchCur = -1;
    QList<struct BracketMark> m_bracketMarks;
    int m_flashLine = -1;                    // flaşlanan satır (0-based)
    int m_rulerCol = 0;
    class QPushButton* m_sticky = nullptr;   // yapışkan kapsam şeridi
    int m_stickyLine = -1;
    bool m_stickyOn = true;
    bool m_focusMode = false; // Stage 12: daktilo modu
    // Stage 13
    QList<struct SemanticToken> m_semantic;
    QStringList m_semLegend;
    QList<struct InlayHint> m_inlay;
    // Stage 14
    QSet<int> m_breakpoints;   // 1-based kesme satırları
    int m_frameLine = -1;      // 0-based çalışan çerçeve
    QMap<int, int> m_coverage; // 1-based satır → hits
    // Stage 15
    QString m_ghost;       // hayalet öneri (imleç sonrası)
    QString m_ghostPrefix; // istek anındaki önek (geçerlilik için)
    // Stage 17
    QList<int> m_snipStops;   // belge mutlak konumlu snippet durakları
    int m_snipIndex = -1;
    bool m_blockSel = false;  // Alt+sürükle dikdörtgen seçim sürüyor
    int m_blockAnchorLine = 0;
    int m_blockAnchorVis = 0;
    QList<QPair<int, int>> m_serverFolds; // LSP foldingRange (başlık→bitiş, 0-based)
};
