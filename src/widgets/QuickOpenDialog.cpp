#include "QuickOpenDialog.h"
#include <QFileInfo>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

QuickOpenDialog::QuickOpenDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Hızlı Aç (Ctrl+P)");
    resize(560, 420);
    auto* lay = new QVBoxLayout(this);
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText("Dosya ara...");
    m_search->setClearButtonEnabled(true);
    m_list = new QListWidget(this);
    lay->addWidget(m_search);
    lay->addWidget(m_list, 1);
    connect(m_search, &QLineEdit::textChanged, this, &QuickOpenDialog::refilter);
    connect(m_list, &QListWidget::itemActivated, this, [this](QListWidgetItem* it) {
        m_selected = it->data(Qt::UserRole).toString();
        accept();
    });
    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem* it) {
        m_selected = it->data(Qt::UserRole).toString();
    });
    connect(m_search, &QLineEdit::returnPressed, this, [this]() {
        if (m_list->count() > 0) {
            m_selected = m_list->item(0)->data(Qt::UserRole).toString();
            accept();
        }
    });
}

void QuickOpenDialog::setFiles(const QStringList& files) {
    m_files = files;
    refilter();
    m_search->setFocus();
}

int QuickOpenDialog::fuzzyScore(const QString& pattern, const QString& text) {
    if (pattern.isEmpty()) return 1000;
    QString p = pattern.toLower(), t = text.toLower();
    int ti = 0, score = 0, last = -1;
    for (QChar c : p) {
        int i = t.indexOf(c, ti);
        if (i < 0) return -1;
        score += (last + 1 == i) ? 10 : 1; // ardışık eşleşme bonusu
        if (t.mid(qMax(0, i - 1), 1) == "/") score += 5; // yol başı bonusu
        last = i;
        ti = i + 1;
    }
    return score - text.length() / 100; // kısa yol hafif bonus
}

void QuickOpenDialog::refilter() {
    QString q = m_search->text();
    struct Hit { int score; QString f; };
    QList<Hit> hits;
    for (const QString& f : m_files) {
        QString name = QFileInfo(f).fileName() + " " + f;
        int s = fuzzyScore(q, name);
        if (s >= 0) hits.append({s, f});
    }
    std::sort(hits.begin(), hits.end(), [](const Hit& a, const Hit& b) { return a.score > b.score; });
    m_list->clear();
    for (int i = 0; i < qMin(200, (int)hits.size()); ++i) {
        auto* it = new QListWidgetItem(QFileInfo(hits[i].f).fileName() + "  —  " + hits[i].f);
        it->setData(Qt::UserRole, hits[i].f);
        m_list->addItem(it);
    }
    if (m_list->count() > 0) m_list->setCurrentRow(0);
}
