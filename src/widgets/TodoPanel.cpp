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
#include <algorithm>
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
        static QRegularExpression atRe("@([A-Za-z0-9_çğıöşüÇĞİÖŞÜ.-]+)");
        static QRegularExpression prRe("!p([1-3])");
        for (int i = 0; i < lines.size() && out.size() < maxHits; ++i) {
            auto m = rx.match(lines[i]);
            if (!m.hasMatch()) continue;
            TodoHit h;
            h.file = p;
            h.line = i + 1;
            h.tag = m.captured(1).toUpper();
            h.text = m.captured(2).trimmed().left(160);
            // Stage 24: "@kullanıcı" + "!p1..p3"
            auto am = atRe.match(h.text);
            if (am.hasMatch()) h.assignee = am.captured(1);
            auto pm = prRe.match(h.text);
            if (pm.hasMatch()) h.priority = pm.captured(1).toInt();
            out.append(h);
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
    // Stage 24: öncelik → satır sırası
    std::sort(hits.begin(), hits.end(), [](const TodoHit& a, const TodoHit& b) {
        if (!!a.priority != !!b.priority) return a.priority > 0 && b.priority == 0;
        if (a.priority != b.priority) return a.priority < b.priority;
        if (a.file != b.file) return a.file < b.file;
        return a.line < b.line;
    });
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
            // Stage 24: "[TAG] satır: metin @kullanıcı !p1"
            QString label = QString("[%1] %2: %3").arg(h.tag).arg(h.line).arg(h.text);
            if (!h.assignee.isEmpty()) label += "  @" + h.assignee;
            if (h.priority > 0) label += QString("  !p%1").arg(h.priority);
            auto* child = new QTreeWidgetItem(top, {label});
            child->setData(0, Qt::UserRole + 1, h.file);
            child->setData(0, Qt::UserRole + 2, h.line);
        }
    }
    m_list->setUpdatesEnabled(true);
    QStringList tags;
    for (auto it = byTag.begin(); it != byTag.end(); ++it) tags << QString("%1×%2").arg(it.key()).arg(it.value());
    m_status->setText(QString("%1 görev — %2").arg(qlonglong(hits.size())).arg(tags.join(" ")));
}
