#include "PortForwardPanel.h"
#include <QHeaderView>
#include <QInputDialog>
#include <QPushButton>
#include <QVBoxLayout>

PortForwardPanel::PortForwardPanel(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels({"Etiket", "Yerel", "Hedef"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    lay->addWidget(m_table, 1);
    auto* row = new QHBoxLayout();
    auto* bAdd = new QPushButton("Tünel+", this);
    auto* bDel = new QPushButton("Kapat", this);
    auto* bRef = new QPushButton("Yenile", this);
    row->addWidget(bAdd);
    row->addWidget(bDel);
    row->addWidget(bRef);
    row->addStretch(1);
    lay->addLayout(row);
    connect(bAdd, &QPushButton::clicked, this, &PortForwardPanel::addRule);
    connect(bDel, &QPushButton::clicked, this, &PortForwardPanel::removeSelected);
    connect(bRef, &QPushButton::clicked, this, &PortForwardPanel::refresh);
}

void PortForwardPanel::setForwarder(PortForwarder* f) {
    if (m_fwd) {
        disconnect(m_fwd, nullptr, this, nullptr);
    }
    m_fwd = f;
    if (m_fwd) {
        connect(m_fwd, &PortForwarder::tunnelUp, this, [this](const QString&) { refresh(); });
        connect(m_fwd, &PortForwarder::tunnelDown, this,
                [this](const QString&, const QString& r) {
                    refresh();
                    emit statusMessage(r);
                });
    }
    refresh();
}

void PortForwardPanel::refresh() {
    m_table->setRowCount(0);
    if (!m_fwd) return;
    const auto rules = m_fwd->active();
    m_table->setRowCount(rules.size());
    for (int i = 0; i < rules.size(); ++i) {
        m_table->setItem(i, 0, new QTableWidgetItem(rules[i].label));
        m_table->setItem(i, 1, new QTableWidgetItem(QString::number(rules[i].localPort)));
        m_table->setItem(i, 2, new QTableWidgetItem(
            rules[i].targetHost + ":" + QString::number(rules[i].targetPort)));
    }
}

void PortForwardPanel::addRule() {
    if (!m_fwd) {
        emit statusMessage("Önce uzağa bağlanın.");
        return;
    }
    bool ok = false;
    const QString spec = QInputDialog::getText(this, "Yeni Tünel",
        "yerelPort:hedefHost:hedefPort [etiket]\nÖrn: 8080:localhost:8080 web",
        QLineEdit::Normal, "8080:localhost:8080", &ok);
    if (!ok || spec.trimmed().isEmpty()) return;
    const QStringList parts = spec.simplified().split(' ');
    const QStringList hp = parts[0].split(':');
    if (hp.size() != 3) {
        emit statusMessage("Biçim: yerelPort:hedefHost:hedefPort");
        return;
    }
    ForwardRule r;
    r.localPort = hp[0].toInt();
    r.targetHost = hp[1];
    r.targetPort = hp[2].toInt();
    r.label = parts.size() > 1 ? parts[1] : r.spec();
    if (!r.valid()) {
        emit statusMessage("Geçersiz tünel kuralı.");
        return;
    }
    if (!m_fwd->start(r)) emit statusMessage("Tünel açılamadı (port dolu olabilir).");
    else emit statusMessage("Tünel açık: " + r.spec());
    refresh();
}

void PortForwardPanel::removeSelected() {
    if (!m_fwd) return;
    const int row = m_table->currentRow();
    if (row < 0) return;
    m_fwd->stop(m_table->item(row, 0)->text());
    refresh();
}
