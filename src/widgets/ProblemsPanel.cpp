#include "ProblemsPanel.h"
#include "EmptyState.h"
#include <QFileInfo>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

ProblemsPanel::ProblemsPanel(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    auto* row = new QHBoxLayout();
    auto* bFix = new QPushButton("AI ile düzelt", this);
    bFix->setToolTip("Seçili sorunu AI'a gönder (ajan modunda araçlarla düzeltebilir)");
    row->addWidget(bFix);
    row->addStretch(1);
    lay->addLayout(row);

    // Stage 10: liste + boş durum yığını
    m_stack = new QStackedWidget(this);
    m_empty = new EmptyState("search", "Sorun yok",
                             "Tanılama temiz — clangd/pylsp aktif dosyaları kontrol ediyor",
                             "Dosya Aç", m_stack);
    m_list = new QListWidget(this);
    m_list->setFont(QFont("Consolas, monospace", 10));
    m_stack->addWidget(m_list);
    m_stack->addWidget(m_empty);
    m_stack->setCurrentWidget(m_list);
    lay->addWidget(m_stack);
    connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* it) {
        if (!it) return;
        emit fileOpened(it->data(Qt::UserRole).toString(), it->data(Qt::UserRole + 1).toInt());
    });
    connect(bFix, &QPushButton::clicked, this, &ProblemsPanel::fixSelected);
}

void ProblemsPanel::fixSelected() {
    QListWidgetItem* it = m_list->currentItem();
    if (!it) return;
    const QString path = it->data(Qt::UserRole).toString();
    if (path.isEmpty()) return;
    emit fixWithAiRequested(path, it->data(Qt::UserRole + 1).toInt(),
                            it->data(Qt::UserRole + 2).toString());
}

void ProblemsPanel::setDiagnostics(const QMap<QString, QList<LspDiag>>& all) {
    m_list->clear();
    int total = 0;
    for (auto it = all.begin(); it != all.end(); ++it) {
        if (it.value().isEmpty()) continue;
        auto* head = new QListWidgetItem(
            QString("— %1 (%2)").arg(QFileInfo(it.key()).fileName()).arg(it.value().size()));
        head->setForeground(QColor("#858585"));
        head->setFlags(head->flags() & ~Qt::ItemIsSelectable);
        m_list->addItem(head);
        for (const auto& d : it.value()) {
            QString sev = d.severity == 1 ? "E" : d.severity == 2 ? "W" : "i";
            auto* li = new QListWidgetItem(
                QString("[%1] %2:%3  %4").arg(sev).arg(d.line + 1).arg(d.col + 1).arg(d.message.left(160)));
            li->setData(Qt::UserRole, d.path);
            li->setData(Qt::UserRole + 1, d.line + 1);
            li->setData(Qt::UserRole + 2, d.message);
            li->setForeground(d.severity == 1 ? QColor("#f44747") : d.severity == 2 ? QColor("#cca700")
                                                                                   : QColor("#858585"));
            m_list->addItem(li);
            ++total;
        }
    }
    // Stage 10: boş durum görünümü
    m_stack->setCurrentWidget(total == 0 ? static_cast<QWidget*>(m_empty)
                                         : static_cast<QWidget*>(m_list));
}

int ProblemsPanel::count() const {
    int n = 0;
    for (int i = 0; i < m_list->count(); ++i)
        if (m_list->item(i)->flags() & Qt::ItemIsSelectable) ++n;
    return n;
}
