#include "CommandPalette.h"
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QSettings>
#include <QVBoxLayout>

static const int kMaxRecents = 8;

QStringList CommandPalette::recents() {
    QSettings q("Verso", "VersoCoder");
    return q.value("palette/recents").toStringList().mid(0, kMaxRecents);
}

void CommandPalette::pushRecent(const QString& id) {
    QStringList r = recents();
    r.removeAll(id);
    r.prepend(id);
    while (r.size() > kMaxRecents) r.removeLast();
    QSettings q("Verso", "VersoCoder");
    q.setValue("palette/recents", r);
}

CommandPalette::CommandPalette(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Komut Paleti");
    resize(560, 440);
    auto* lay = new QVBoxLayout(this);
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText("Komut ara... (fuzzy)");
    m_search->setClearButtonEnabled(true);
    m_list = new QListWidget(this);
    m_count = new QLabel(this);
    m_count->setObjectName("paletteCount");
    lay->addWidget(m_search);
    lay->addWidget(m_list, 1);
    lay->addWidget(m_count);
    connect(m_search, &QLineEdit::textChanged, this, &CommandPalette::refilter);
    connect(m_list, &QListWidget::itemActivated, this, [this](QListWidgetItem* it) {
        m_selected = it->data(Qt::UserRole).toString();
        accept();
    });
    connect(m_search, &QLineEdit::returnPressed, this, [this]() {
        if (m_list->count() > 0) {
            m_selected = m_list->item(0)->data(Qt::UserRole).toString();
            accept();
        }
    });
}

void CommandPalette::setCommands(const QList<PaletteCommand>& cmds) {
    m_cmds = cmds;
    m_recents = recents();
    refilter();
    m_search->setFocus();
}

int CommandPalette::matchScore(const QString& pattern, const QString& text) {
    if (pattern.isEmpty()) return 1000;
    QString p = pattern.toLower(), t = text.toLower();
    int ti = 0, score = 0, last = -1;
    for (QChar c : p) {
        int i = t.indexOf(c, ti);
        if (i < 0) return -1;
        score += (last + 1 == i) ? 10 : 1;
        last = i;
        ti = i + 1;
    }
    return score;
}

void CommandPalette::refilter() {
    QString q = m_search->text();
    if (q.startsWith(">")) q = q.mid(1).trimmed();
    struct Hit { int score; PaletteCommand c; };
    QList<Hit> hits;
    for (const auto& c : m_cmds) {
        int s = matchScore(q, c.title + " " + c.id);
        if (s >= 0) {
            // Stage 10: son kullanılanlar öne gelir
            if (m_recents.contains(c.id)) s += 500;
            hits.append({s, c});
        }
    }
    std::sort(hits.begin(), hits.end(), [](const Hit& a, const Hit& b) { return a.score > b.score; });
    m_list->clear();
    for (const auto& h : hits) {
        const bool recent = m_recents.contains(h.c.id);
        QString label = h.c.keys.isEmpty() ? h.c.title : QString("%1   [%2]").arg(h.c.title, h.c.keys);
        if (recent) label = "★  " + label;
        auto* it = new QListWidgetItem(label);
        if (recent) {
            QFont f = it->font();
            f.setBold(true);
            it->setFont(f);
        }
        it->setData(Qt::UserRole, h.c.id);
        it->setToolTip(h.c.hint);
        m_list->addItem(it);
    }
    if (m_list->count() > 0) m_list->setCurrentRow(0);
    m_count->setText(m_list->count() == 0
                         ? "eşleşme yok — farklı bir anahtar kelime deneyin"
                         : QString("%1 komut").arg(m_list->count()));
}
