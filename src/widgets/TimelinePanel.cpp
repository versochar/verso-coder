#include "TimelinePanel.h"
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

TimelinePanel::TimelinePanel(const QString& storeDir, QWidget* parent)
    : QWidget(parent), m_hist(storeDir) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    m_list = new QListWidget(this);
    m_list->setMaximumHeight(180);
    lay->addWidget(m_list, 1);
    m_preview = new QTextEdit(this);
    m_preview->setReadOnly(true);
    m_preview->setFont(QFont("monospace", 10));
    m_preview->setPlaceholderText("Sürüm önizlemesi...");
    lay->addWidget(m_preview, 2);
    auto* row = new QHBoxLayout();
    auto* bDiff = new QPushButton("Fark", this);
    auto* bRestore = new QPushButton("Geri Yükle", this);
    auto* bClear = new QPushButton("Temizle", this);
    auto* bRef = new QPushButton("Yenile", this);
    row->addWidget(bDiff);
    row->addWidget(bRestore);
    row->addWidget(bClear);
    row->addWidget(bRef);
    row->addStretch(1);
    lay->addLayout(row);
    connect(m_list, &QListWidget::currentRowChanged, this, &TimelinePanel::onSelect);
    connect(bDiff, &QPushButton::clicked, this, &TimelinePanel::showDiff);
    connect(bRestore, &QPushButton::clicked, this, &TimelinePanel::doRestore);
    connect(bClear, &QPushButton::clicked, this, &TimelinePanel::doClear);
    connect(bRef, &QPushButton::clicked, this, &TimelinePanel::refresh);
}

void TimelinePanel::setFile(const QString& filePath, const QString& currentText) {
    m_file = filePath;
    m_current = currentText;
    refresh();
}

void TimelinePanel::refresh() {
    m_list->clear();
    m_preview->clear();
    m_snaps.clear();
    if (m_file.isEmpty() || m_file.startsWith("ssh://")) {
        m_list->addItem(m_file.isEmpty() ? "(dosya yok)" : "(uzak dosya: geçmiş yok)");
        return;
    }
    m_snaps = m_hist.list(m_file);
    if (m_snaps.isEmpty()) {
        m_list->addItem("(anlık görüntü yok — kaydedince oluşur)");
        return;
    }
    for (const HistorySnap& s : m_snaps) {
        const QString when = s.when.isValid() ? s.when.toString("dd.MM HH:mm:ss")
                                              : s.id;
        m_list->addItem(QString("%1  (%2 KB)").arg(when).arg(s.size / 1024 + 1));
    }
    m_list->setCurrentRow(0);
}

void TimelinePanel::onSelect() {
    const int r = m_list->currentRow();
    if (r < 0 || r >= m_snaps.size()) return;
    m_preview->setPlainText(m_hist.read(m_snaps[r]).left(20000));
}

void TimelinePanel::showDiff() {
    const int r = m_list->currentRow();
    if (r < 0 || r >= m_snaps.size()) return;
    const QString old = m_hist.read(m_snaps[r]);
    // WordDiff satır farkı → sade metin özeti
    const QStringList a = old.split('\n'), b = m_current.split('\n');
    QStringList out;
    const int n = qMax(a.size(), b.size());
    int shown = 0;
    for (int i = 0; i < n && shown < 40; ++i) {
        const QString x = (i < a.size()) ? a[i] : QString();
        const QString y = (i < b.size()) ? b[i] : QString();
        if (x != y) {
            out << QString("- %1").arg(x.left(120));
            out << QString("+ %1").arg(y.left(120));
            ++shown;
        }
    }
    if (shown == 0) out << "(fark yok)";
    m_preview->setPlainText(out.join('\n'));
    emit statusMessage(QString("%1 farklı satır").arg(shown));
}

void TimelinePanel::doRestore() {
    const int r = m_list->currentRow();
    if (r < 0 || r >= m_snaps.size()) return;
    auto ans = QMessageBox::question(
        this, "Geri Yükle",
        "Bu sürüm editöre yazılsın mı?\n(mevcut içerik önce anlık görüntülenir)");
    if (ans != QMessageBox::Yes) return;
    emit restoreRequested(m_file, m_hist.read(m_snaps[r]));
}

void TimelinePanel::doClear() {
    if (m_file.isEmpty()) return;
    auto ans = QMessageBox::question(this, "Temizle", "Bu dosyanın geçmişi silinsin mi?");
    if (ans != QMessageBox::Yes) return;
    m_hist.clear(m_file);
    refresh();
}
