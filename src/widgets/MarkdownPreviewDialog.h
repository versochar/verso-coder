#pragma once
#include <QDialog>

class QTextBrowser;
class QTimer;
class CodeEditor;

// Stage 28: Markdown önizleme — işlenmiş görünüm + kaydırma senkronu.
class MarkdownPreviewDialog : public QDialog {
    Q_OBJECT
public:
    explicit MarkdownPreviewDialog(CodeEditor* editor, QWidget* parent = nullptr);

private slots:
    void refresh();
    void syncScroll();

private:
    CodeEditor* m_editor;
    QTextBrowser* m_view = nullptr;
    QTimer* m_timer = nullptr;
};
