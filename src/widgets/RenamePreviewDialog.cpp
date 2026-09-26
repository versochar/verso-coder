#include "RenamePreviewDialog.h"
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QLineEdit>
#include <QListWidget>
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
    // Stage 24: dosya listesi (işareti kaldırılan uygulanmaz)
    m_files = new QListWidget(this);
    m_files->setMaximumHeight(90);
    m_files->setVisible(false);
    lay->addWidget(m_files);
    m_preview = new QTextEdit(this);
    m_preview->setReadOnly(true);
    m_preview->setFontFamily("monospace");
    lay->addWidget(m_preview, 1);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    box->button(QDialogButtonBox::Ok)->setText("Uygula");
    box->button(QDialogButtonBox::Cancel)->setText("Vazgeç");
    lay->addWidget(box);    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_name, &QLineEdit::textChanged, this, &RenamePreviewDialog::onNameChanged);
    onNameChanged(oldName);
}

QString RenamePreviewDialog::newName() const {
    return m_name->text().trimmed();
}

// Stage 24: etkilenen dosyalar (hepsi işaretli başlar)
void RenamePreviewDialog::setFileList(const QStringList& files) {
    m_files->clear();
    if (files.size() < 2) {
        m_files->setVisible(false);
        return;
    }
    m_files->setVisible(true);
    for (const QString& f : files) {
        auto* it = new QListWidgetItem(QFileInfo(f).fileName(), m_files);
        it->setToolTip(f);
        it->setData(Qt::UserRole, f);
        it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
        it->setCheckState(Qt::Checked);
    }
}

QStringList RenamePreviewDialog::excludedFiles() const {
    QStringList out;
    if (!m_files || !m_files->isVisible()) return out;
    for (int i = 0; i < m_files->count(); ++i) {
        auto* it = m_files->item(i);
        if (it->checkState() != Qt::Checked) out << it->data(Qt::UserRole).toString();
    }
    return out;
}

void RenamePreviewDialog::onNameChanged(const QString& t) {
    // Önizleme: her düzenlemenin yeni metnini yazılan adla değiştir
    QList<LspTextEdit> sim = m_edits;
    for (LspTextEdit& e : sim) e.newText = t;
    QStringList pv = TextEdits::preview(m_text, sim);
    if (pv.size() > 60) pv = pv.mid(0, 60) << QString("... (%1 yer daha)").arg(m_edits.size() - 60);
    m_preview->setPlainText(pv.join('\n'));
}
