#include "BulkRenameDialog.h"
#include <QBrush>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QTreeWidget>
#include <QVBoxLayout>

BulkRenameDialog::BulkRenameDialog(const QString& dir, QWidget* parent)
    : QDialog(parent), m_dir(dir) {
    setWindowTitle("Toplu Yeniden Adlandır");
    resize(560, 420);
    auto* lay = new QVBoxLayout(this);
    auto* form = new QFormLayout();
    m_from = new QLineEdit("test_*.cpp", this);
    m_to = new QLineEdit("spec_*.cpp", this);
    form->addRow("Desen:", m_from);
    form->addRow("Yeni ad:", m_to);
    lay->addLayout(form);
    m_preview = new QTreeWidget(this);
    m_preview->setHeaderLabels({"Eski", "Yeni"});
    lay->addWidget(m_preview, 1);
    QDir d(dir);
    m_files = d.entryList(QDir::Files | QDir::NoDotAndDotDot);
    connect(m_from, &QLineEdit::textChanged, this, &BulkRenameDialog::refreshPreview);
    connect(m_to, &QLineEdit::textChanged, this, &BulkRenameDialog::refreshPreview);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Close,
                                     this);
    box->button(QDialogButtonBox::Apply)->setText("Tümünü Uygula");
    connect(box->button(QDialogButtonBox::Apply), &QPushButton::clicked, this,
            &BulkRenameDialog::applyAll);
    connect(box, &QDialogButtonBox::rejected, this, &BulkRenameDialog::reject);
    lay->addWidget(box);
    refreshPreview();
}

QString BulkRenameDialog::applyPattern(const QString& name, const QString& from,
                                       const QString& to) {
    // "*" → yakalama grubu; hedefteki "*" yakalanana yazılır
    QString rx = QRegularExpression::escape(from);
    rx.replace("\\*", "(.*)");
    auto m = QRegularExpression("^" + rx + "$").match(name);
    if (!m.hasMatch()) return {};
    QString out = to;
    out.replace("*", m.captured(1));
    return out;
}

void BulkRenameDialog::refreshPreview() {
    m_preview->clear();
    const QString from = m_from->text(), to = m_to->text();
    if (from.isEmpty() || to.isEmpty() || !to.contains('*') || !from.contains('*'))
        return;
    for (const QString& f : m_files) {
        const QString n = applyPattern(f, from, to);
        if (n.isEmpty() || n == f) continue;
        auto* it = new QTreeWidgetItem(m_preview, {f, n});
        if (QFileInfo::exists(m_dir + "/" + n)) {
            it->setForeground(1, QBrush(QColor("#f44747")));
            it->setToolTip(1, "Hedef zaten var — atlanacak");
        }
    }
}

void BulkRenameDialog::applyAll() {
    const QString from = m_from->text(), to = m_to->text();
    int ok = 0, skip = 0;
    for (const QString& f : m_files) {
        const QString n = applyPattern(f, from, to);
        if (n.isEmpty() || n == f || QFileInfo::exists(m_dir + "/" + n)) {
            if (!n.isEmpty() && n != f) ++skip;
            continue;
        }
        if (QFile::rename(m_dir + "/" + f, m_dir + "/" + n)) ++ok;
        else ++skip;
    }
    QMessageBox::information(this, "Toplu Adlandır", QString("%1 taşındı, %2 atlandı").arg(ok).arg(skip));
    QDir d(m_dir);
    m_files = d.entryList(QDir::Files | QDir::NoDotAndDotDot);
    refreshPreview();
}
