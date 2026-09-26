#include "MarkdownPreviewDialog.h"
#include "CodeEditor.h"
#include <QScrollBar>
#include <QTextBrowser>
#include <QTimer>
#include <QVBoxLayout>

MarkdownPreviewDialog::MarkdownPreviewDialog(CodeEditor* editor, QWidget* parent)
    : QDialog(parent), m_editor(editor) {
    setWindowTitle("Markdown Önizleme");
    resize(640, 520);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    m_view = new QTextBrowser(this);
    m_view->setOpenExternalLinks(true);
    lay->addWidget(m_view, 1);
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setInterval(400);
    connect(m_timer, &QTimer::timeout, this, &MarkdownPreviewDialog::refresh);
    if (m_editor) {
        connect(m_editor, &QPlainTextEdit::textChanged, m_timer,
                QOverload<>::of(&QTimer::start));
        connect(m_editor->verticalScrollBar(), &QScrollBar::valueChanged, this,
                &MarkdownPreviewDialog::syncScroll);
    }
    refresh();
}

void MarkdownPreviewDialog::refresh() {
    if (!m_editor) return;
    m_view->setMarkdown(m_editor->toPlainText());
    syncScroll();
}

void MarkdownPreviewDialog::syncScroll() {
    if (!m_editor || !m_view) return;
    auto* esb = m_editor->verticalScrollBar();
    auto* vsb = m_view->verticalScrollBar();
    const int span = esb->maximum() - esb->minimum();
    const double frac = span > 0 ? double(esb->value() - esb->minimum()) / span : 0.0;
    vsb->setValue(vsb->minimum() + int(frac * (vsb->maximum() - vsb->minimum())));
}
