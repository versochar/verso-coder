#include "ApplyEditDialog.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

ApplyEditDialog::ApplyEditDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("AI düzenlemesini onayla");
    resize(860, 520);
    auto* lay = new QVBoxLayout(this);
    auto* panes = new QHBoxLayout();
    m_old = new QTextEdit(this);
    m_new = new QTextEdit(this);
    m_old->setReadOnly(true);
    m_new->setReadOnly(true);
    m_old->setFont(QFont("Consolas, monospace", 10));
    m_new->setFont(QFont("Consolas, monospace", 10));
    auto* left = new QVBoxLayout();
    left->addWidget(new QLabel("Mevcut:", this));
    left->addWidget(m_old, 1);
    auto* right = new QVBoxLayout();
    right->addWidget(new QLabel("AI önerisi:", this));
    right->addWidget(m_new, 1);
    panes->addLayout(left, 1);
    panes->addLayout(right, 1);
    lay->addLayout(panes, 1);

    auto* btns = new QHBoxLayout();
    auto* bNo = new QPushButton("Vazgeç", this);
    auto* bYes = new QPushButton("Uygula", this);
    bYes->setDefault(true);
    btns->addStretch(1);
    btns->addWidget(bNo);
    btns->addWidget(bYes);
    connect(bNo, &QPushButton::clicked, this, &QDialog::reject);
    connect(bYes, &QPushButton::clicked, this, &QDialog::accept);
    lay->addLayout(btns);
}

void ApplyEditDialog::setTexts(const QString& filePath, const QString& oldText, const QString& newText) {
    setWindowTitle("Onayla: " + filePath);
    m_old->setPlainText(oldText);
    m_new->setPlainText(newText);
}

QString ApplyEditDialog::newText() const { return m_new->toPlainText(); }
