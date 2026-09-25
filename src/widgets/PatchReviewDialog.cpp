#include "PatchReviewDialog.h"
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>

PatchReviewDialog::PatchReviewDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Ajan düzenlemeleri — incele & uygula");
    resize(900, 560);

    auto* lay = new QVBoxLayout(this);
    m_summary = new QLabel(this);
    m_summary->setStyleSheet("color:#858585;font-size:11px;");
    lay->addWidget(m_summary);

    auto* split = new QSplitter(Qt::Horizontal, this);
    m_tree = new QTreeWidget(split);
    m_tree->setHeaderLabels({"Dosya", "Durum"});
    m_tree->setRootIsDecorated(false);
    m_tree->setColumnWidth(0, 320);
    m_preview = new QTextEdit(split);
    m_preview->setReadOnly(true);
    m_preview->setFont(QFont("Consolas, monospace", 10));
    split->addWidget(m_tree);
    split->addWidget(m_preview);
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);
    lay->addWidget(split, 1);

    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    box->button(QDialogButtonBox::Ok)->setText("Seçilenleri uygula");
    box->button(QDialogButtonBox::Cancel)->setText("Reddet");
    lay->addWidget(box);
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_tree, &QTreeWidget::currentItemChanged, this, [this]() { refreshPreview(); });
    connect(m_tree, &QTreeWidget::itemChanged, this, [this]() { refreshPreview(); });
}

void PatchReviewDialog::setEdits(const QList<QueuedEdit>& edits) {
    m_edits = edits;
    m_tree->clear();
    for (const QueuedEdit& e : edits) {
        auto* it = new QTreeWidgetItem(m_tree);
        it->setText(0, QFileInfo(e.path).fileName());
        it->setToolTip(0, e.path);
        it->setText(1, e.oldText.isEmpty() ? "yeni dosya" : "değişiklik");
        it->setCheckState(0, Qt::Checked);
        it->setData(0, Qt::UserRole, e.path);
    }
    m_summary->setText(QString("%1 dosya düzenlemesi önerildi. Uygulamak istediklerini seç.")
                           .arg(edits.size()));
    if (m_tree->topLevelItemCount() > 0) m_tree->setCurrentItem(m_tree->topLevelItem(0));
    refreshPreview();
}

QList<QueuedEdit> PatchReviewDialog::selectedEdits() const {
    QList<QueuedEdit> out;
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* it = m_tree->topLevelItem(i);
        if (it->checkState(0) != Qt::Checked) continue;
        const QString path = it->data(0, Qt::UserRole).toString();
        for (const QueuedEdit& e : m_edits)
            if (e.path == path) { out << e; break; }
    }
    return out;
}

void PatchReviewDialog::refreshPreview() {
    QTreeWidgetItem* it = m_tree->currentItem();
    if (!it) { m_preview->clear(); return; }
    const QString path = it->data(0, Qt::UserRole).toString();
    for (const QueuedEdit& e : m_edits) {
        if (e.path != path) continue;
        const QString diff = PatchQueue::unifiedDiff(e);
        QString html = "<pre style='font-family:Consolas,monospace;font-size:10pt;margin:0'>";
        for (const QString& line : diff.split('\n')) {
            QString color = "#d4d4d4";
            if (line.startsWith("@@")) color = "#569cd6";
            else if (line.startsWith("+++") || line.startsWith("---")) color = "#c586c0";
            else if (line.startsWith("diff ")) color = "#858585";
            else if (line.startsWith("+")) color = "#6a9955";
            else if (line.startsWith("-")) color = "#f44747";
            html += QString("<span style='color:%1'>%2</span>\n").arg(color, line.toHtmlEscaped());
        }
        html += "</pre>";
        m_preview->setHtml(html);
        return;
    }
    m_preview->clear();
}
