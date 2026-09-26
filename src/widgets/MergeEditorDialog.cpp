#include "MergeEditorDialog.h"
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

MergeEditorDialog::MergeEditorDialog(const QString& filePath, const QString& text,
                                     QWidget* parent)
    : QDialog(parent), m_path(filePath) {
    setWindowTitle("Birleştir: " + QFileInfo(filePath).fileName());
    resize(760, 560);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);
    m_lines = text.split('\n');
    m_hunks = MergeParse::find(text);
    m_choices = QList<int>(m_hunks.size(), -1);
    auto* info = new QLabel(this);
    info->setObjectName("mergeInfo");
    info->setText(QString("%1 çakışma").arg(m_hunks.size()));
    lay->addWidget(info);
    auto* mid = new QHBoxLayout();
    m_ours = new QTextEdit(this);
    m_ours->setReadOnly(true);
    m_ours->setFontFamily("monospace");
    m_theirs = new QTextEdit(this);
    m_theirs->setReadOnly(true);
    m_theirs->setFontFamily("monospace");
    mid->addWidget(m_ours, 1);
    mid->addWidget(m_theirs, 1);
    lay->addLayout(mid, 2);
    m_result = new QTextEdit(this);
    m_result->setReadOnly(true);
    m_result->setFontFamily("monospace");
    m_result->setMaximumHeight(140);
    lay->addWidget(m_result, 1);
    auto* row = new QHBoxLayout();
    auto* bOurs = new QPushButton("Bizimki", this);
    auto* bTheirs = new QPushButton("Onlarınki", this);
    auto* bBoth = new QPushButton("Her İkisi", this);
    auto* bAll = new QPushButton("Kalanı Bizimkiyle Bitir", this);
    row->addWidget(bOurs);
    row->addWidget(bTheirs);
    row->addWidget(bBoth);
    row->addWidget(bAll);
    lay->addLayout(row);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                     this);
    box->button(QDialogButtonBox::Ok)->setText("Uygula");
    connect(box, &QDialogButtonBox::accepted, this, &MergeEditorDialog::applyAll);
    connect(box, &QDialogButtonBox::rejected, this, &MergeEditorDialog::reject);
    lay->addWidget(box);
    connect(bOurs, &QPushButton::clicked, this, &MergeEditorDialog::takeOurs);
    connect(bTheirs, &QPushButton::clicked, this, &MergeEditorDialog::takeTheirs);
    connect(bBoth, &QPushButton::clicked, this, &MergeEditorDialog::takeBoth);
    connect(bAll, &QPushButton::clicked, this, [this]() {
        for (int i = 0; i < m_choices.size(); ++i)
            if (m_choices[i] < 0) m_choices[i] = 0;
        showCurrent();
    });
    showCurrent();
}

int MergeEditorDialog::remaining() const {
    return m_choices.count(-1);
}

void MergeEditorDialog::showCurrent() {
    m_cur = m_choices.indexOf(-1);
    if (m_cur < 0) {
        m_ours->setPlainText("(tümü çözüldü — Uygula)");
        m_theirs->clear();
        m_result->setPlainText(MergeParse::applyAll(m_lines.join('\n'), m_choices));
        return;
    }
    const MergeHunk& h = m_hunks[m_cur];
    setWindowTitle(QString("Birleştir (%1/%2)").arg(m_cur + 1).arg(m_hunks.size()));
    m_ours->setPlainText(QString("[%1]\n").arg(h.oursLabel) + h.ours.join('\n'));
    m_theirs->setPlainText(QString("[%1]\n").arg(h.theirsLabel) + h.theirs.join('\n'));
    m_result->setPlainText(MergeParse::applyAll(m_lines.join('\n'), m_choices));
}

void MergeEditorDialog::takeOurs() {
    if (m_cur >= 0) m_choices[m_cur] = 0;
    showCurrent();
}

void MergeEditorDialog::takeTheirs() {
    if (m_cur >= 0) m_choices[m_cur] = 1;
    showCurrent();
}

void MergeEditorDialog::takeBoth() {
    if (m_cur >= 0) m_choices[m_cur] = 2;
    showCurrent();
}

void MergeEditorDialog::applyAll() {
    if (remaining() > 0) {
        // Kararsızları bizimkiyle kapat
        for (int i = 0; i < m_choices.size(); ++i)
            if (m_choices[i] < 0) m_choices[i] = 0;
    }
    m_resolved = MergeParse::applyAll(m_lines.join('\n'), m_choices);
    accept();
}
