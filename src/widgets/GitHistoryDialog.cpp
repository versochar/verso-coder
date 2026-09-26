#include "GitHistoryDialog.h"
#include <QDateTime>
#include <QFileInfo>
#include <QProcess>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>

GitHistoryDialog::GitHistoryDialog(const QString& workdir, const QString& file,
                                   QWidget* parent)
    : QDialog(parent), m_dir(workdir), m_file(file) {
    setWindowTitle("Geçmiş: " + QFileInfo(file).fileName());
    resize(700, 520);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);
    m_list = new QTreeWidget(this);
    m_list->setHeaderLabels({"Commit", "Tarih", "Mesaj"});
    m_list->setRootIsDecorated(false);
    m_list->setColumnWidth(0, 90);
    m_list->setColumnWidth(1, 140);
    lay->addWidget(m_list, 1);
    m_diff = new QTextEdit(this);
    m_diff->setReadOnly(true);
    m_diff->setFontFamily("monospace");
    m_diff->setMaximumHeight(220);
    lay->addWidget(m_diff, 1);
    connect(m_list, &QTreeWidget::currentItemChanged, this,
            &GitHistoryDialog::showDiff);
    refreshList();
}

static QString runGit(const QString& dir, const QStringList& args) {
    QProcess p;
    p.setWorkingDirectory(dir);
    p.start("git", args);
    if (!p.waitForFinished(10000)) return {};
    return QString::fromUtf8(p.readAllStandardOutput());
}

void GitHistoryDialog::refreshList() {
    m_list->clear();
    const QString out = runGit(m_dir, {"log", "--follow", "--format=%H%x1f%at%x1f%s",
                                       "--", m_file});
    for (const QString& ln : out.split('\n', Qt::SkipEmptyParts)) {
        const QStringList p = ln.split('\x1f');
        if (p.size() < 3) continue;
        auto* it = new QTreeWidgetItem(
            m_list, {p[0].left(8),
                     QDateTime::fromSecsSinceEpoch(p[1].toLongLong()).toString("yyyy-MM-dd HH:mm"),
                     p[2].left(120)});
        it->setData(0, Qt::UserRole, p[0]);
        if (m_list->topLevelItemCount() == 1) m_list->setCurrentItem(it);
    }
    if (m_list->topLevelItemCount() == 0)
        new QTreeWidgetItem(m_list, {"(geçmiş yok — git deposu mu?)"});
}

void GitHistoryDialog::showDiff() {
    auto* it = m_list->currentItem();
    if (!it) return;
    const QString sha = it->data(0, Qt::UserRole).toString();
    if (sha.isEmpty()) {
        m_diff->clear();
        return;
    }
    m_diff->setPlainText(runGit(m_dir, {"show", "--no-color", "--format=", sha, "--", m_file})
                             .left(20000));
}
