#include "ReferencesDialog.h"
#include <QFile>
#include <QFileInfo>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>

ReferencesDialog::ReferencesDialog(const QString& symbol, const QList<LspLocation>& locs,
                                   QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(QString("Referanslar: %1 (%2)").arg(symbol).arg(locs.size()));
    resize(560, 480);
    auto* lay = new QVBoxLayout(this);
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabels({"Dosya / Satır"});
    m_tree->setRootIsDecorated(true);
    lay->addWidget(m_tree, 2);
    // Stage 24: seçili göndermenin bağlam önizlemesi
    m_preview = new QTextEdit(this);
    m_preview->setReadOnly(true);
    m_preview->setMaximumHeight(110);
    m_preview->setFont(QFont("Consolas, monospace", 10));
    lay->addWidget(m_preview, 1);
    for (const QString& f : LocationSet::files(locs)) {
        auto* top = new QTreeWidgetItem(m_tree);
        const auto inFile = LocationSet::forFile(locs, f);
        top->setText(0, QString("%1 (%2)").arg(QFileInfo(f).fileName()).arg(inFile.size()));
        top->setData(0, Qt::UserRole, f);
        for (const LspLocation& l : inFile) {
            auto* ch = new QTreeWidgetItem(top);
            ch->setText(0, QString("Satır %1, sütun %2").arg(l.line + 1).arg(l.col + 1));
            ch->setData(0, Qt::UserRole, QVariant::fromValue<QString>(l.path));
            ch->setData(0, Qt::UserRole + 1, l.line);
            ch->setData(0, Qt::UserRole + 2, l.col);
        }
        top->setExpanded(true);
    }
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, &ReferencesDialog::onJump);
    connect(m_tree, &QTreeWidget::currentItemChanged, this,
            &ReferencesDialog::onPreview);
    onPreview();
}

// Stage 24: seçili düğümün ±2 satır bağlamı
void ReferencesDialog::onPreview() {
    auto* it = m_tree->currentItem();
    if (!it) { m_preview->clear(); return; }
    if (it->childCount() > 0) it = it->child(0);
    if (!it) { m_preview->clear(); return; }
    const QString path = it->data(0, Qt::UserRole).toString();
    const int line = it->data(0, Qt::UserRole + 1).toInt(); // 0-based
    QFile f(path);
    if (path.isEmpty() || !f.open(QIODevice::ReadOnly)) {
        m_preview->setPlainText("(önizleme yok)");
        return;
    }
    const QStringList lines = QString::fromUtf8(f.read(1 << 20)).split('\n');
    QStringList out;
    for (int i = qMax(0, line - 2); i <= qMin(lines.size() - 1, line + 2); ++i) {
        QString mark = (i == line) ? "▶ " : "  ";
        out << QString("%1%2: %3").arg(mark).arg(i + 1).arg(lines[i].left(120));
    }
    m_preview->setPlainText(out.join('\n'));
}

void ReferencesDialog::onJump() {
    auto* it = m_tree->currentItem();
    if (!it) return;
    QString path = it->data(0, Qt::UserRole).toString();
    // Üst düğümse ilk çocuğa in
    if (it->childCount() > 0) it = it->child(0);
    path = it->data(0, Qt::UserRole).toString();
    m_sel.path = path;
    m_sel.line = it->data(0, Qt::UserRole + 1).toInt();
    m_sel.col = it->data(0, Qt::UserRole + 2).toInt();
    if (m_sel.path.isEmpty()) return;
    accept();
}
