#include "RenamePreviewDialog.h"
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

RenamePreviewDialog::RenamePreviewDialog(const QString& oldName,
                                         const QList<LspTextEdit>& edits,
                                         const QString& text, QWidget* parent)
    : QDialog(parent), m_edits(edits), m_text(text) {
    setWindowTitle(QString("Yeniden Adlandır: %1 (%2 yer)").arg(oldName).arg(edits.size()));
    resize(520, 420);
    auto* lay = new QVBoxLayout(this);
    m_name = new QLineEdit(oldName, this);
    m_name->selectAll();
    lay->addWidget(m_name);
    m_preview = new QTextEdit(this);
    m_preview->setReadOnly(true);
    m_preview->setFontFamily("monospace");
    lay->addWidget(m_preview, 1);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    box->button(QDialogButtonBox::Ok)->setText("Uygula");
    box->button(QDialogButtonBox::Cancel)->setText("Vazgeç");
    lay->addWidget(box);
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_name, &QLineEdit::textChanged, this, &RenamePreviewDialog::onNameChanged);
    onNameChanged(oldName);
}

QString RenamePreviewDialog::newName() const {
    return m_name->text().trimmed();
}

void RenamePreviewDialog::onNameChanged(const QString& t) {
    // Önizleme: her düzenlemenin yeni metnini yazılan adla değiştir
    QList<LspTextEdit> sim = m_edits;
    for (LspTextEdit& e : sim) e.newText = t;
    QStringList pv = TextEdits::preview(m_text, sim);
    if (pv.size() > 60) pv = pv.mid(0, 60) << QString("... (%1 yer daha)").arg(m_edits.size() - 60);
    m_preview->setPlainText(pv.join('\n'));
}
