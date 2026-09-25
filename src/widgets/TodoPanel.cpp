#include "TodoPanel.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QMap>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QtConcurrent>

TodoPanel::TodoPanel(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    auto* top = new QHBoxLayout();
    m_pattern = new QLineEdit("TODO|FIXME|HACK|XXX|BUG|NOTE", this);
    m_pattern->setToolTip("Regex etiket deseni");
    auto* bScan = new QPushButton("Tara", this);
    top->addWidget(m_pattern, 1);
    top->addWidget(bScan);
    m_status = new QLabel("", this);
    m_status->setStyleSheet("color:#858585;font-size:11px;");
    m_list = new QTreeWidget(this);
    m_list->setHeaderLabels({"Görevler"});
    m_list->setRootIsDecorated(true);
    lay->addLayout(top);
    lay->addWidget(m_status);
    lay->addWidget(m_list, 1);

    connect(bScan, &QPushButton::clicked, this, &TodoPanel::runScan);
    connect(m_pattern, &QLineEdit::returnPressed, this, &TodoPanel::runScan);
    connect(&m_watcher, &QFutureWatcher<QList<TodoHit>>::finished, this, &TodoPanel::onScanDone);
    connect(m_list, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* it, int) {
        if (!it) return;
        QString f = it->data(0, Qt::UserRole + 1).toString();
        int line = it->data(0, Qt::UserRole + 2).toInt();
        if (!f.isEmpty()) emit fileOpened(f, line);
    });
}

void TodoPanel::setRoot(const QString& root) {
    m_root = root;
    runScan();
}

QList<TodoHit> TodoPanel::scanSync(const QString& root, const QString& pattern, int maxHits) {
    QList<TodoHit> out;
    QRegularExpression rx("\\b(" + pattern + ")\\b\\s*:?\\s*(.*)",
                          QRegularExpression::CaseInsensitiveOption);
    if (!rx.isValid() || root.isEmpty()) return out;
    QDirIterator it(root, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    int scanned = 0;
    while (it.hasNext() && out.size() < maxHits && scanned < 8000) {
        QString p = it.next();
        if (p.contains("/.git/") || p.contains("/build/") || p.contains("/node_modules/")) {
            ++scanned;
            continue;
        }
        QFile f(p);
        if (!f.open(QIODevice::ReadOnly)) { ++scanned; continue; }
        QByteArray raw = f.read(1 << 20);
        if (raw.contains('\0')) { ++scanned; continue; }
        const QStringList lines = QString::fromUtf8(raw).split('\n');
        for (int i = 0; i < lines.size() && out.size() < maxHits; ++i) {
            auto m = rx.match(lines[i]);
            if (m.hasMatch())
                out.append({p, i + 1, m.captured(1).toUpper(), m.captured(2).trimmed().left(160)});
        }
        ++scanned;
    }
    return out;
}

void TodoPanel::runScan() {
    if (m_watcher.isRunning()) m_watcher.cancel();
    m_list->clear();
    if (m_root.isEmpty() || m_pattern->text().trimmed().isEmpty()) return;
    m_status->setText("Taranıyor...");
    QString root = m_root, pat = m_pattern->text();
    m_watcher.setFuture(QtConcurrent::run(scanSync, root, pat, 2000));
}

void TodoPanel::onScanDone() {
    if (m_watcher.isCanceled()) return;
    auto hits = m_watcher.result();
    QMap<QString, QList<TodoHit>> byFile;
    QMap<QString, int> byTag;
    for (const auto& h : hits) {
        byFile[h.file].append(h);
        byTag[h.tag]++;
    }
    m_list->setUpdatesEnabled(false);
    for (auto it = byFile.begin(); it != byFile.end(); ++it) {
        auto* top = new QTreeWidgetItem(m_list, {QFileInfo(it.key()).fileName() +
                                                 QString(" (%1)").arg(it.value().size())});
        top->setData(0, Qt::UserRole + 1, it.key());
        top->setData(0, Qt::UserRole + 2, it.value().first().line);
        top->setExpanded(true);
        for (const auto& h : it.value()) {
            auto* child = new QTreeWidgetItem(
                top, {QString("[%1] %2: %3").arg(h.tag).arg(h.line).arg(h.text)});
            child->setData(0, Qt::UserRole + 1, h.file);
            child->setData(0, Qt::UserRole + 2, h.line);
        }
    }
    m_list->setUpdatesEnabled(true);
    QStringList tags;
    for (auto it = byTag.begin(); it != byTag.end(); ++it) tags << QString("%1×%2").arg(it.key()).arg(it.value());
    m_status->setText(QString("%1 görev — %2").arg(qlonglong(hits.size())).arg(tags.join(" ")));
}
