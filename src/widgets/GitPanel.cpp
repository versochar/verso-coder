#include "GitPanel.h"
#include "DiffDialog.h"
#include "../core/CommitMsg.h"
#include <QCheckBox>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

GitPanel::GitPanel(QWidget* parent) : QWidget(parent), m_dir(QDir::homePath()) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    // Branch satırı
    auto* bRow = new QHBoxLayout();
    m_branches = new QComboBox(this);
    m_branches->setToolTip("Branch (seç = checkout)");
    auto* bRefresh = new QPushButton("⟳", this);
    bRefresh->setFixedWidth(30);
    bRow->addWidget(m_branches, 1);
    bRow->addWidget(bRefresh);

    m_ahead = new QLabel(this);
    m_ahead->setStyleSheet("color:#858585;font-size:11px;");
    m_ahead->setToolTip("Yukarı akışa göre önde/geride (yoksa boş)");
    bRow->addWidget(m_ahead);

    auto* nbRow = new QHBoxLayout();
    m_newBranch = new QLineEdit(this);
    m_newBranch->setPlaceholderText("yeni-branch-adı");
    auto* bCreate = new QPushButton("+ Oluştur", this);
    nbRow->addWidget(m_newBranch, 1);
    nbRow->addWidget(bCreate);

    // Değişen dosyalar
    lay->addWidget(new QLabel("BRANCH", this));
    lay->addLayout(bRow);
    lay->addLayout(nbRow);
    lay->addWidget(new QLabel("DEĞİŞİKLİKLER (dosya seç)", this));
    m_files = new QListWidget(this);
    m_files->setMaximumHeight(160);
    lay->addWidget(m_files);

    auto* fRow1 = new QHBoxLayout();
    auto* bStage = new QPushButton("Stage", this);
    auto* bUnstage = new QPushButton("Unstage", this);
    auto* bDiff = new QPushButton("Diff", this);
    auto* bDiffCached = new QPushButton("Diff(staged)", this);
    auto* bOpen = new QPushButton("Aç", this);
    fRow1->addWidget(bStage); fRow1->addWidget(bUnstage);
    fRow1->addWidget(bDiff); fRow1->addWidget(bDiffCached); fRow1->addWidget(bOpen);
    lay->addLayout(fRow1);

    auto* fRow1b = new QHBoxLayout();
    auto* bHunk = new QPushButton("Hunk...", this);
    bHunk->setToolTip("Hunk bazlı stage/discard (yan yana diff)");
    auto* bHunkC = new QPushButton("Hunk(staged)...", this);
    bHunkC->setToolTip("Staged hunk'ları tek tek geri al");
    fRow1b->addWidget(bHunk, 1);
    fRow1b->addWidget(bHunkC, 1);
    lay->addLayout(fRow1b);

    auto* fRow2 = new QHBoxLayout();
    auto* bStageAll = new QPushButton("Tümünü Stage'le", this);
    auto* bLog = new QPushButton("Log", this);
    fRow2->addWidget(bStageAll, 1);
    fRow2->addWidget(bLog, 1);
    lay->addLayout(fRow2);

    // Stage 43: stash (kaydet/uygula/bırak)
    lay->addWidget(new QLabel("STASH", this));
    auto* sRow = new QHBoxLayout();
    m_stash = new QComboBox(this);
    m_stash->setToolTip("Stash listesi (seç = uygula)");
    auto* bStashSave = new QPushButton("Kaydet", this);
    bStashSave->setToolTip("Çalışmayı stash'e al (git stash push)");
    auto* bStashApply = new QPushButton("Uygula", this);
    bStashApply->setToolTip("Seçili stash'i uygula (listede kalır)");
    auto* bStashDrop = new QPushButton("Bırak", this);
    bStashDrop->setToolTip("Seçili stash'i sil");
    sRow->addWidget(m_stash, 1);
    sRow->addWidget(bStashSave);
    sRow->addWidget(bStashApply);
    sRow->addWidget(bStashDrop);
    lay->addLayout(sRow);
    connect(bStashSave, &QPushButton::clicked, this, &GitPanel::stashSave);
    connect(bStashApply, &QPushButton::clicked, this, &GitPanel::stashApply);
    connect(bStashDrop, &QPushButton::clicked, this, &GitPanel::stashDrop);

    // Commit
    m_msg = new QLineEdit(this);
    m_msg->setPlaceholderText("Commit mesajı...");
    auto* cRow = new QHBoxLayout();
    // Stage 21: ileti şablonu + amend
    m_tpl = new QComboBox(this);
    m_tpl->setToolTip("İleti şablonu öneki");
    m_tpl->addItem("Şablon…");
    m_tpl->addItems(CommitMsg::templates());
    connect(m_tpl, QOverload<int>::of(&QComboBox::activated), this, [this](int i) {
        if (i <= 0) return;
        m_msg->setText(CommitMsg::applyTemplate(m_tpl->itemText(i), m_msg->text()));
        m_msg->setFocus();
        m_tpl->setCurrentIndex(0);
    });
    m_amend = new QCheckBox("Amend", this);
    m_amend->setToolTip("Son commit'i değiştir (commit --amend)");
    cRow->addWidget(m_tpl, 1);
    cRow->addWidget(m_amend);
    auto* cRow2 = new QHBoxLayout();
    auto* bCommit = new QPushButton("Commit", this);
    auto* bPush = new QPushButton("Push", this);
    auto* bPull = new QPushButton("Pull", this);
    cRow2->addWidget(bCommit); cRow2->addWidget(bPush); cRow2->addWidget(bPull);
    lay->addWidget(m_msg);
    lay->addLayout(cRow);
    lay->addLayout(cRow2);

    m_out = new QTextEdit(this);
    m_out->setReadOnly(true);
    m_out->setFont(QFont("Consolas, monospace", 10));
    lay->addWidget(m_out, 1);

    connect(bRefresh, &QPushButton::clicked, this, &GitPanel::refresh);
    connect(bCreate, &QPushButton::clicked, this, [this]() {
        QString b = m_newBranch->text().trimmed();
        if (b.isEmpty()) return;
        Cmd r = runGit({"checkout", "-b", b});
        log("checkout -b " + b, r.out + r.err);
        m_newBranch->clear();
        refresh();
    });
    connect(m_branches, QOverload<int>::of(&QComboBox::activated), this, [this](int i) {
        QString b = m_branches->itemText(i).trimmed();
        if (b.startsWith("* ")) b = b.mid(2);
        if (b.isEmpty()) return;
        Cmd r = runGit({"checkout", b});
        log("checkout " + b, r.out + r.err);
        refresh();
    });
    connect(bStage, &QPushButton::clicked, this, &GitPanel::stageSelected);
    connect(bUnstage, &QPushButton::clicked, this, &GitPanel::unstageSelected);
    connect(bDiff, &QPushButton::clicked, this, [this]() { showDiff(false); });
    connect(bDiffCached, &QPushButton::clicked, this, [this]() { showDiff(true); });
    connect(bHunk, &QPushButton::clicked, this, [this]() { showHunkDiff(false); });
    connect(bHunkC, &QPushButton::clicked, this, [this]() { showHunkDiff(true); });
    connect(bOpen, &QPushButton::clicked, this, &GitPanel::openSelected);
    connect(m_files, &QListWidget::itemDoubleClicked, this, &GitPanel::openSelected);
    connect(bStageAll, &QPushButton::clicked, this, [this]() {
        log("add -A", runGit({"add", "-A"}).out); refresh();
    });
    connect(bLog, &QPushButton::clicked, this, &GitPanel::refreshLog);
    connect(bCommit, &QPushButton::clicked, this, &GitPanel::commit);
    connect(bPush, &QPushButton::clicked, this, [this]() {
        Cmd r = runGit({"push"});
        log("push", r.out + r.err); refreshLog();
    });
    connect(bPull, &QPushButton::clicked, this, [this]() {
        Cmd r = runGit({"pull", "--ff-only"});
        log("pull --ff-only", r.out + r.err); refresh();
    });
}

void GitPanel::setWorkdir(const QString& dir) {
    m_dir = dir;
    refresh();
}

GitPanel::Cmd GitPanel::runGit(const QStringList& args, int timeoutMs) {
    Cmd c;
    QProcess p(this);
    p.setWorkingDirectory(m_dir);
    p.start("git", args);
    p.waitForFinished(timeoutMs);
    c.exit = p.exitCode();
    c.out = QString::fromUtf8(p.readAllStandardOutput());
    c.err = QString::fromUtf8(p.readAllStandardError());
    return c;
}

void GitPanel::log(const QString& cmd, const QString& text) {
    QString t = text.trimmed();
    if (t.isEmpty()) t = "(çıktı yok)";
    m_out->setPlainText("$ git " + cmd + "\n" + t + "\n\n" + m_out->toPlainText().left(6000));
}

QString GitPanel::selectedFile() const {
    auto* it = m_files->currentItem();
    if (!it) return {};
    return it->data(Qt::UserRole).toString();
}

void GitPanel::refresh() {
    Cmd repo = runGit({"rev-parse", "--show-toplevel"});
    if (repo.exit != 0) {
        m_files->clear();
        m_branches->clear();
        m_out->setPlainText("Git deposu bulunamadı: " + m_dir);
        return;
    }
    // Dosya listesi
    m_files->clear();
    Cmd st = runGit({"status", "--porcelain=v1"});
    for (const QString& ln : st.out.split('\n', Qt::SkipEmptyParts)) {
        if (ln.size() < 4) continue;
        QString xy = ln.left(2);
        QString path = ln.mid(3).trimmed();
        if (path.contains(" -> ")) path = path.split(" -> ").last(); // rename
        auto* it = new QListWidgetItem(QString("[%1] %2").arg(xy, path));
        it->setData(Qt::UserRole, QDir(m_dir).absoluteFilePath(path));
        if (xy.trimmed() == "??")
            it->setForeground(QColor("#6a9955"));
        else if (xy[0] != ' ' && xy[0] != '?')
            it->setForeground(QColor("#4ec9b0")); // staged
        else
            it->setForeground(QColor("#ce9178")); // unstaged
        m_files->addItem(it);
    }
    if (m_files->count() == 0)
        m_files->addItem("(temiz — değişiklik yok)");
    refreshBranches();
    refreshStash();
}

void GitPanel::refreshStash() {
    if (!m_stash) return;
    Cmd l = runGit({"stash", "list", "--format=%gd: %gs"});
    m_stash->blockSignals(true);
    m_stash->clear();
    for (const QString& ln : l.out.split('\n', Qt::SkipEmptyParts))
        m_stash->addItem(ln.trimmed());
    if (m_stash->count() == 0) m_stash->addItem("(stash yok)");
    m_stash->blockSignals(false);
}

void GitPanel::stashSave() {
    Cmd r = runGit({"stash", "push", "-m", "verso-stash"});
    log("stash push", r.out + r.err);
    refresh();
}

void GitPanel::stashApply() {
    if (!m_stash || m_stash->count() == 0) return;
    const QString entry = m_stash->currentText().split(':').first().trimmed();
    if (entry == "(stash yok)") return;
    Cmd r = runGit({"stash", "apply", entry});
    log("stash apply " + entry, r.out + r.err);
    refresh();
}

void GitPanel::stashDrop() {
    if (!m_stash || m_stash->count() == 0) return;
    const QString entry = m_stash->currentText().split(':').first().trimmed();
    if (entry == "(stash yok)") return;
    Cmd r = runGit({"stash", "drop", entry});
    log("stash drop " + entry, r.out + r.err);
    refresh();
}

void GitPanel::refreshBranches() {
    QString cur;
    Cmd head = runGit({"rev-parse", "--abbrev-ref", "HEAD"});
    if (head.exit == 0) cur = head.out.trimmed();
    // Stage 43: önde/geride (yukarı akış yoksa sessiz)
    if (m_ahead) {
        m_ahead->clear();
        Cmd ab = runGit({"rev-list", "--left-right", "--count", "HEAD...@{upstream}"});
        if (ab.exit == 0) {
            const QStringList parts = ab.out.trimmed().split(QRegularExpression("\\s+"));
            if (parts.size() == 2) {
                const int ahead = parts[0].toInt(), behind = parts[1].toInt();
                if (ahead > 0 || behind > 0)
                    m_ahead->setText(QString("↑%1 ↓%2").arg(ahead).arg(behind));
            }
        }
    }
    Cmd b = runGit({"branch", "--format=%(refname:short)"});
    m_branches->blockSignals(true);
    m_branches->clear();
    for (const QString& name : b.out.split('\n', Qt::SkipEmptyParts)) {
        QString n = name.trimmed();
        m_branches->addItem(n == cur ? "* " + n : n);
        if (n == cur) m_branches->setCurrentIndex(m_branches->count() - 1);
    }
    m_branches->blockSignals(false);
}

void GitPanel::refreshLog() {
    Cmd l = runGit({"log", "--oneline", "--graph", "--decorate", "-30"});
    if (l.exit != 0) log("log", l.err);
    else log("log --oneline --graph -30", l.out);
}

void GitPanel::stageSelected() {
    QString f = selectedFile();
    if (f.isEmpty()) return;
    Cmd r = runGit({"add", "--", f});
    log("add -- " + QFileInfo(f).fileName(), r.out + r.err);
    refresh();
}

void GitPanel::unstageSelected() {
    QString f = selectedFile();
    if (f.isEmpty()) return;
    Cmd r = runGit({"restore", "--staged", "--", f});
    if (r.exit != 0) r = runGit({"reset", "HEAD", "--", f}); // eski git uyumluluğu
    log("unstage " + QFileInfo(f).fileName(), r.out + r.err);
    refresh();
}

void GitPanel::showDiff(bool cached) {
    QString f = selectedFile();
    QStringList args = {"diff", "--no-color"};
    if (cached) args << "--cached";
    if (!f.isEmpty()) args << "--" << f;
    Cmd r = runGit(args);
    QString diff = r.out.isEmpty() ? "(fark yok)" : r.out.left(8000);
    // Basit renklendirme
    QString html;
    for (const QString& ln : diff.split('\n')) {
        QString esc = ln.toHtmlEscaped();
        if (ln.startsWith("+") && !ln.startsWith("+++"))
            html += "<span style='color:#6a9955;'>" + esc + "</span><br>";
        else if (ln.startsWith("-") && !ln.startsWith("---"))
            html += "<span style='color:#f44747;'>" + esc + "</span><br>";
        else if (ln.startsWith("@@"))
            html += "<span style='color:#569cd6;'>" + esc + "</span><br>";
        else
            html += esc + "<br>";
    }
    m_out->setHtml(QString("<b>$ git %1</b><br>%2<hr>%3")
        .arg(args.join(" ").toHtmlEscaped(), html, m_out->toHtml()));
}

void GitPanel::showHunkDiff(bool cached) {
    QStringList args = {"diff", "--no-color"};
    if (cached) args << "--cached";
    QString f = selectedFile();
    if (!f.isEmpty()) args << "--" << f;
    Cmd r = runGit(args, 15000);
    if (r.out.trimmed().isEmpty()) { log(args.join(" "), "(fark yok)"); return; }
    DiffDialog d(this);
    d.setRepoDiff(m_dir, r.out, cached);
    d.exec();
    refresh();
    refreshLog();
}

void GitPanel::setCommitMessage(const QString& msg) {
    m_msg->setText(msg.trimmed().split('\n').first().trimmed().left(100));
    m_msg->setFocus();
}

void GitPanel::commit() {
    QString m = m_msg->text().trimmed();
    if (m.isEmpty()) { log("commit", "Önce commit mesajı yaz."); return; }
    QStringList args = {"commit", "-m", m};
    if (m_amend && m_amend->isChecked()) args = {"commit", "--amend", "-m", m};
    Cmd r = runGit(args);
    log(args.join(" "), r.out + r.err);
    m_msg->clear();
    refresh();
    refreshLog();
}

void GitPanel::openSelected() {
    QString f = selectedFile();
    if (!f.isEmpty() && QFileInfo(f).isFile()) emit fileOpenRequested(f);
}
