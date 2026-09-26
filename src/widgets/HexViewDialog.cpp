#include "HexViewDialog.h"
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

HexViewDialog::HexViewDialog(const QString& path, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Hex: " + QFileInfo(path).fileName());
    resize(680, 520);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);
    auto* row = new QHBoxLayout();
    row->addWidget(new QLabel("Adres (hex):", this));
    m_addr = new QLineEdit("0", this);
    auto* bGo = new QPushButton("Git", this);
    row->addWidget(m_addr, 1);
    row->addWidget(bGo);
    lay->addLayout(row);
    m_view = new QTextEdit(this);
    m_view->setReadOnly(true);
    m_view->setFontFamily("monospace");
    lay->addWidget(m_view, 1);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(box, &QDialogButtonBox::rejected, this, &HexViewDialog::reject);
    lay->addWidget(box);
    connect(bGo, &QPushButton::clicked, this, &HexViewDialog::goToAddress);
    connect(m_addr, &QLineEdit::returnPressed, this, &HexViewDialog::goToAddress);
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) m_data = f.read(1 << 20); // ilk 1 MB
    render(0);
}

void HexViewDialog::goToAddress() {
    bool ok = false;
    const qint64 a = m_addr->text().trimmed().toLongLong(&ok, 16);
    if (!ok) return;
    render(qMax<qint64>(0, a - (a % 16)));
}

void HexViewDialog::render(qint64 from, qint64 lines) {
    QStringList out;
    for (qint64 off = from; off < m_data.size() && lines-- > 0; off += 16) {
        QString hex, asc;
        for (int i = 0; i < 16; ++i) {
            if (off + i < m_data.size()) {
                const unsigned char b = m_data[off + i];
                hex += QString("%1 ").arg(b, 2, 16, QChar('0'));
                asc += (b >= 32 && b < 127) ? QChar(b) : '.';
            } else {
                hex += "   ";
                asc += ' ';
            }
        }
        out << QString("%1  %2 |%3|").arg(off, 8, 16, QChar('0')).arg(hex, asc);
    }
    if (out.isEmpty()) out << "(boş)";
    m_view->setPlainText(out.join('\n'));
}
