#include "GitTools.h"
#include <QComboBox>
#include <QDir>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

static QString gitRun(const QString& dir, const QStringList& args, int ms = 10000) {
    QProcess p;
    p.setWorkingDirectory(dir);
    p.start("git", args);
    p.waitForFinished(ms);
    return QString::fromUtf8(p.readAllStandardOutput());
}

// ---------- Geçmiş ----------
HistoryTab::HistoryTab(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    auto* top = new QHBoxLayout();
    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText("Filtre (mesaj/yazar)...");
    auto* bRef = new QPushButton("Yenile", this);
    top->addWidget(m_filter, 1);
    top->addWidget(bRef);
    m_log = new QTreeWidget(this);
    m_log->setHeaderLabels({"Grafik", "Özet", "Yazar", "Tarih"});
    m_log->setRootIsDecorated(false);
    m_log->setFont(QFont("Consolas, monospace", 10));
    m_detail = new QTextEdit(this);
    m_detail->setReadOnly(true);
    m_detail->setMaximumHeight(180);
    m_detail->setFont(QFont("Consolas, monospace", 10));
    auto* row = new QHBoxLayout();
    auto* bShow = new QPushButton("Göster", this);
    auto* bPick = new QPushButton("Cherry-pick", this);
    auto* bRevert = new QPushButton("Revert", this);
    row->addWidget(bShow);
    row->addWidget(bPick);
    row->addWidget(bRevert);
    lay->addLayout(top);
    lay->addWidget(m_log, 1);
    lay->addLayout(row);
    lay->addWidget(m_detail);

    connect(bRef, &QPushButton::clicked, this, &HistoryTab::refresh);
    connect(m_filter, &QLineEdit::returnPressed, this, &HistoryTab::refresh);
    connect(bShow, &QPushButton::clicked, this, &HistoryTab::showSelected);
    connect(m_log, &QTreeWidget::itemDoubleClicked, this, &HistoryTab::showSelected);
    connect(bPick, &QPushButton::clicked, this, [this]() {
        auto* it = m_log->currentItem();
        if (!it) return;
        QString sha = it->data(0, Qt::UserRole).toString();
        QString out = runGit({"cherry-pick", sha});
        m_detail->setPlainText("$ git cherry-pick " + sha.left(8) + "\n" + out);
        refresh();
    });
    connect(bRevert, &QPushButton::clicked, this, [this]() {
        auto* it = m_log->currentItem();
        if (!it) return;
        QString sha = it->data(0, Qt::UserRole).toString();
        QString out = runGit({"revert", "--no-edit", sha});
        m_detail->setPlainText("$ git revert " + sha.left(8) + "\n" + out);
        refresh();
    });
}

void HistoryTab::setWorkdir(const QString& dir) {
    m_dir = dir;
    refresh();
}

QString HistoryTab::runGit(const QStringList& args) {
    return gitRun(m_dir, args);
}

void HistoryTab::refresh() {
    m_log->clear();
    QString f = m_filter->text().trimmed();
    QStringList args = {"log", "--graph", "--color=never",
                        "--pretty=format:%H%x1f%h%x1f%an%x1f%ad%x1f%s",
                        "--date=short", "-150"};
    QString out = runGit(args);
    for (const QString& ln : out.split('\n', Qt::SkipEmptyParts)) {
        // Grafik öneki + \x1f ayrımlı alanlar
        int h = ln.indexOf('\x1f');
        QString graph, rest;
        if (h < 0) { graph = ln; rest = ""; }
        else { graph = ln.left(h); rest = ln.mid(h + 1); }
        QStringList cols = rest.split('\x1f');
        while (cols.size() < 5) cols << "";
        if (!f.isEmpty() && !cols[4].contains(f, Qt::CaseInsensitive) &&
            !cols[2].contains(f, Qt::CaseInsensitive))
            continue;
        auto* it = new QTreeWidgetItem(m_log, {graph, cols[4], cols[2], cols[3]});
        it->setData(0, Qt::UserRole, cols[0]);
        it->setToolTip(1, cols[0]);
        m_log->addTopLevelItem(it);
    }
    m_log->resizeColumnToContents(0);
}

void HistoryTab::showSelected() {
    auto* it = m_log->currentItem();
    if (!it) return;
    QString sha = it->data(0, Qt::UserRole).toString();
    m_detail->setPlainText(runGit({"show", "--stat", "--no-color", sha}).left(6000));
}

// ---------- Stash ----------
StashTab::StashTab(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    m_list = new QListWidget(this);
    auto* top = new QHBoxLayout();
    m_msg = new QLineEdit(this);
    m_msg->setPlaceholderText("stash mesajı (opsiyonel)");
    auto* bPush = new QPushButton("Stash'le", this);
    top->addWidget(m_msg, 1);
    top->addWidget(bPush);
    auto* row = new QHBoxLayout();
    auto* bApply = new QPushButton("Uygula", this);
    auto* bPop = new QPushButton("Pop", this);
    auto* bDrop = new QPushButton("Sil", this);
    auto* bRef = new QPushButton("Yenile", this);
    row->addWidget(bApply);
    row->addWidget(bPop);
    row->addWidget(bDrop);
    row->addWidget(bRef);
    lay->addWidget(m_list, 1);
    lay->addLayout(top);
    lay->addLayout(row);

    connect(bRef, &QPushButton::clicked, this, &StashTab::refresh);
    connect(bPush, &QPushButton::clicked, this, &StashTab::push);
    connect(bApply, &QPushButton::clicked, this, [this]() { apply(0); });
    connect(bPop, &QPushButton::clicked, this, [this]() { apply(1); });
    connect(bDrop, &QPushButton::clicked, this, [this]() {
        auto* it = m_list->currentItem();
        if (!it) return;
        runGit({"stash", "drop", it->data(Qt::UserRole).toString()});
        refresh();
    });
}

void StashTab::setWorkdir(const QString& dir) {
    m_dir = dir;
    refresh();
}

QString StashTab::runGit(const QStringList& args) {
    return gitRun(m_dir, args);
}

void StashTab::refresh() {
    m_list->clear();
    int i = 0;
    for (const QString& ln : runGit({"stash", "list"}).split('\n', Qt::SkipEmptyParts)) {
        // "stash@{0}: WIP on main: mesaj"
        auto* it = new QListWidgetItem(ln);
        it->setData(Qt::UserRole, QString("stash@{%1}").arg(i++));
        m_list->addItem(it);
    }
    if (m_list->count() == 0) m_list->addItem("(stash boş)");
}

void StashTab::push() {
    QStringList args = {"stash", "push"};
    if (!m_msg->text().trimmed().isEmpty()) args << "-m" << m_msg->text().trimmed();
    runGit(args);
    m_msg->clear();
    refresh();
}

void StashTab::apply(int pop) {
    auto* it = m_list->currentItem();
    if (!it || it->data(Qt::UserRole).toString().isEmpty()) return;
    runGit({"stash", pop ? "pop" : "apply", it->data(Qt::UserRole).toString()});
    refresh();
}

// ---------- Uzak ----------
RemoteTab::RemoteTab(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    m_out = new QTextEdit(this);
    m_out->setReadOnly(true);
    m_out->setFont(QFont("Consolas, monospace", 10));
    auto* addRow = new QHBoxLayout();
    m_name = new QLineEdit(this);
    m_name->setPlaceholderText("ad (origin)");
    m_name->setMaximumWidth(110);
    m_url = new QLineEdit(this);
    m_url->setPlaceholderText("url");
    auto* bAdd = new QPushButton("Ekle", this);
    auto* bDel = new QPushButton("Sil", this);
    addRow->addWidget(m_name);
    addRow->addWidget(m_url, 1);
    addRow->addWidget(bAdd);
    addRow->addWidget(bDel);
    auto* row = new QHBoxLayout();
    auto* bFetch = new QPushButton("Fetch --prune", this);
    auto* bPush = new QPushButton("Push -u origin HEAD", this);
    auto* bRef = new QPushButton("Yenile", this);
    row->addWidget(bFetch);
    row->addWidget(bPush);
    row->addWidget(bRef);
    lay->addWidget(m_out, 1);
    lay->addLayout(addRow);
    lay->addLayout(row);

    connect(bRef, &QPushButton::clicked, this, &RemoteTab::refresh);
    connect(bAdd, &QPushButton::clicked, this, [this]() {
        if (m_name->text().trimmed().isEmpty() || m_url->text().trimmed().isEmpty()) return;
        m_out->setPlainText(runGit({"remote", "add", m_name->text().trimmed(), m_url->text().trimmed()}));
        refresh();
    });
    connect(bDel, &QPushButton::clicked, this, [this]() {
        QString n = QInputDialog::getText(this, "Uzak sil", "Ad:");
        if (n.trimmed().isEmpty()) return;
        m_out->setPlainText(runGit({"remote", "remove", n.trimmed()}));
        refresh();
    });
    connect(bFetch, &QPushButton::clicked, this, [this]() {
        m_out->setPlainText(runGit({"fetch", "--all", "--prune"}));
        refresh();
    });
    connect(bPush, &QPushButton::clicked, this, [this]() {
        m_out->setPlainText(runGit({"push", "-u", "origin", "HEAD"}).left(3000));
        refresh();
    });
}

void RemoteTab::setWorkdir(const QString& dir) {
    m_dir = dir;
    refresh();
}

QString RemoteTab::runGit(const QStringList& args) {
    return gitRun(m_dir, args);
}

void RemoteTab::refresh() {
    QString t = "$ git remote -v\n" + runGit({"remote", "-v"});
    t += "\n$ git branch -vv (kısa)\n" + runGit({"branch", "-vv"});
    // ahead/behind
    QProcess p;
    p.setWorkingDirectory(m_dir);
    p.start("git", {"rev-list", "--left-right", "--count", "HEAD...@{upstream}"});
    p.waitForFinished(5000);
    if (p.exitCode() == 0) {
        QStringList ab = QString::fromUtf8(p.readAllStandardOutput()).trimmed().split('\t');
        if (ab.size() == 2)
            t += QString("\nupstream: +%1 ileri / -%2 geri").arg(ab[0], ab[1]);
    } else {
        t += "\nupstream: yok";
    }
    m_out->setPlainText(t);
}
