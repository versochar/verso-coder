#include "SearchEditorDialog.h"
#include "../core/ReplaceEngine.h"
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

SearchEditorDialog::SearchEditorDialog(const QString& root, const QList<SearchHit>& hits,
                                       const QString& query, const QString& replacement,
                                       bool useRegex, bool caseSens, QWidget* parent)
    : QDialog(parent),
      m_root(root),
      m_hits(hits),
      m_query(query),
      m_replace(replacement),
      m_useRegex(useRegex),
      m_caseSens(caseSens) {
    setWindowTitle(QString("Arama: %1 (%2)").arg(query).arg(hits.size()));
    resize(680, 480);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabels({"Dosya / Satır"});
    m_tree->setRootIsDecorated(true);
    lay->addWidget(m_tree, 1);
    QMap<QString, QList<SearchHit>> byFile;
    for (const SearchHit& h : hits) byFile[h.file].append(h);
    for (auto it = byFile.constBegin(); it != byFile.constEnd(); ++it) {
        auto* top = new QTreeWidgetItem(
            m_tree, {QString("%1 (%2)").arg(QFileInfo(it.key()).fileName()).arg(it.value().size())});
        top->setFlags(top->flags() | Qt::ItemIsUserCheckable);
        top->setCheckState(0, Qt::Checked);
        top->setData(0, Qt::UserRole, it.key());
        top->setExpanded(true);
        for (const SearchHit& h : it.value()) {
            auto* ch = new QTreeWidgetItem(
                top, {QString("%1: %2").arg(h.line).arg(h.preview.left(120))});
            ch->setData(0, Qt::UserRole, h.file);
            ch->setData(0, Qt::UserRole + 1, h.line);
        }
    }
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem* it, int) {
                if (!it) return;
                const QString f = it->data(0, Qt::UserRole).toString();
                if (!f.isEmpty()) emit jumpRequested(f, it->data(0, Qt::UserRole + 1).toInt());
            });
    auto* row = new QHBoxLayout();
    auto* bApply = new QPushButton("İşaretli Dosyalarda Değiştir", this);
    bApply->setToolTip("Yalnızca işaretli dosyalara uygular");
    row->addStretch(1);
    row->addWidget(bApply);
    lay->addLayout(row);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(box, &QDialogButtonBox::rejected, this, &SearchEditorDialog::reject);
    lay->addWidget(box);
    connect(bApply, &QPushButton::clicked, this, &SearchEditorDialog::applyChecked);
}

void SearchEditorDialog::applyChecked() {
    QStringList files;
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto* top = m_tree->topLevelItem(i);
        if (top->checkState(0) == Qt::Checked)
            files << top->data(0, Qt::UserRole).toString();
    }
    if (files.isEmpty()) return;
    auto r = QMessageBox::question(this, "Toplu Değiştir",
                                   QString("%1 dosyaya uygulanacak. Emin misiniz?")
                                       .arg(files.size()),
                                   QMessageBox::Yes | QMessageBox::Cancel);
    if (r != QMessageBox::Yes) return;
    int n = 0;
    for (const QString& f : files) {
        QFile qf(f);
        if (!qf.open(QIODevice::ReadOnly)) continue;
        const QString text = QString::fromUtf8(qf.readAll());
        qf.close();
        auto [next, edits] = ReplaceEngine::applyFile(f, text, m_query, m_replace,
                                                      m_useRegex, m_caseSens);
        if (edits.isEmpty()) continue;
        if (qf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qf.write(next.toUtf8());
            n += edits.size();
        }
    }
    QMessageBox::information(this, "Toplu Değiştir", QString("%1 değişiklik uygulandı").arg(n));
    accept();
}
