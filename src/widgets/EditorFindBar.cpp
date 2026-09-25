#include "EditorFindBar.h"
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

EditorFindBar::EditorFindBar(QWidget* parent) : QWidget(parent) {
    setObjectName("findBar");
    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(8, 4, 8, 4);
    lay->setSpacing(6);

    m_edit = new QLineEdit(this);
    m_edit->setObjectName("findEdit");
    m_edit->setPlaceholderText("Bul... (Enter: sonraki, Shift+Enter: önceki)");
    m_edit->setClearButtonEnabled(true);
    m_edit->setMaximumWidth(320);
    lay->addWidget(m_edit);

    m_count = new QLabel(this);
    m_count->setObjectName("findCount");
    m_count->setMinimumWidth(70);
    lay->addWidget(m_count);

    m_case = new QCheckBox("Aa", this);
    m_case->setToolTip("Büyük/küçük harf duyarlı");
    lay->addWidget(m_case);
    m_word = new QCheckBox("W", this);
    m_word->setToolTip("Tam kelime");
    lay->addWidget(m_word);

    m_prev = new QPushButton("↑", this);
    m_prev->setToolTip("Önceki (Shift+F3)");
    m_prev->setFixedWidth(30);
    lay->addWidget(m_prev);
    m_next = new QPushButton("↓", this);
    m_next->setToolTip("Sonraki (F3)");
    m_next->setFixedWidth(30);
    lay->addWidget(m_next);

    auto* close = new QPushButton("✕", this);
    close->setToolTip("Kapat (Esc)");
    close->setFixedWidth(30);
    lay->addWidget(close);
    lay->addStretch(1);

    connect(m_edit, &QLineEdit::textChanged, this, [this]() { emit queryChanged(); });
    connect(m_case, &QCheckBox::toggled, this, [this]() { emit queryChanged(); });
    connect(m_word, &QCheckBox::toggled, this, [this]() { emit queryChanged(); });
    connect(m_edit, &QLineEdit::returnPressed, this, [this]() { emit navigate(+1); });
    connect(m_next, &QPushButton::clicked, this, [this]() { emit navigate(+1); });
    connect(m_prev, &QPushButton::clicked, this, [this]() { emit navigate(-1); });
    connect(close, &QPushButton::clicked, this, [this]() { emit closed(); });
}

QString EditorFindBar::needle() const { return m_edit->text(); }
bool EditorFindBar::caseSensitive() const { return m_case->isChecked(); }
bool EditorFindBar::wholeWord() const { return m_word->isChecked(); }
void EditorFindBar::setCountText(const QString& t) { m_count->setText(t); }

void EditorFindBar::focusNeedle() {
    show();
    m_edit->setFocus();
    m_edit->selectAll();
}

void EditorFindBar::setNeedle(const QString& s) {
    if (!s.isEmpty() && m_edit->text() != s) m_edit->setText(s);
}
