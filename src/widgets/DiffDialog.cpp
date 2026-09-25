#include "DiffDialog.h"
#include "../core/AccentColor.h"
#include "../core/ThemeManager.h"
#include "../core/WordDiff.h"
#include <QColor>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextEdit>
#include <QVBoxLayout>

int DiffHunk::adds() const {
    int n = 0;
    for (const auto& l : lines) if (l.startsWith('+')) ++n;
    return n;
}
int DiffHunk::dels() const {
    int n = 0;
    for (const auto& l : lines) if (l.startsWith('-')) ++n;
    return n;
}

QList<FileDiff> DiffEngine::parseUnified(const QString& diffText) {
    QList<FileDiff> out;
    FileDiff* cur = nullptr;
    DiffHunk* hunk = nullptr;
    static const QRegularExpression hunkRx("^@@ -(\\d+)(?:,(\\d+))? \\+(\\d+)(?:,(\\d+))? @@(.*)$");
    for (QString ln : diffText.split('\n')) {
        if (ln.endsWith('\r')) ln.chop(1);
        if (ln.startsWith("diff --git ")) {
            out.append(FileDiff());
            cur = &out.last();
            hunk = nullptr;
            // diff --git a/x b/y
            QString rest = ln.mid(11);
            int sp = rest.indexOf(" b/");
            if (sp > 0 && rest.startsWith("a/")) {
                cur->oldPath = rest.left(sp);
                cur->newPath = rest.mid(sp + 1);
                cur->relPath = cur->newPath.mid(2);
            }
        } else if (cur && ln.startsWith("index ")) {
            cur->indexLine = ln;
        } else if (cur && ln.startsWith("--- ")) {
            cur->oldPath = ln.mid(4).trimmed();
            if (cur->relPath.isEmpty() && cur->oldPath.startsWith("a/"))
                cur->relPath = cur->oldPath.mid(2);
        } else if (cur && ln.startsWith("+++ ")) {
            cur->newPath = ln.mid(4).trimmed();
            if (cur->relPath.isEmpty() && cur->newPath.startsWith("b/"))
                cur->relPath = cur->newPath.mid(2);
            if (cur->relPath == "/dev/null") cur->relPath = cur->oldPath.mid(2);
        } else if (cur) {
            auto m = hunkRx.match(ln);
            if (m.hasMatch()) {
                cur->hunks.append(DiffHunk());
                hunk = &cur->hunks.last();
                hunk->header = ln;
                hunk->oldStart = m.captured(1).toInt();
                hunk->oldCount = m.captured(2).isEmpty() ? 1 : m.captured(2).toInt();
                hunk->newStart = m.captured(3).toInt();
                hunk->newCount = m.captured(4).isEmpty() ? 1 : m.captured(4).toInt();
            } else if (hunk && (ln.startsWith(' ') || ln.startsWith('+') || ln.startsWith('-'))) {
                hunk->lines << ln;
            }
            // "\ No newline at end of file" yoksayılır
        }
    }
    // Boş dosyaları ele (hunk yoksa)
    for (int i = out.size() - 1; i >= 0; --i)
        if (out[i].hunks.isEmpty()) out.removeAt(i);
    return out;
}

QString DiffEngine::buildPatch(const FileDiff& fd, const QList<int>& pick) {
    QString out = QString("diff --git %1 %2\n").arg(fd.oldPath, fd.newPath);
    if (!fd.indexLine.isEmpty()) out += fd.indexLine + "\n";
    out += "--- " + fd.oldPath + "\n";
    out += "+++ " + fd.newPath + "\n";
    for (int i : pick) {
        if (i < 0 || i >= fd.hunks.size()) continue;
        const DiffHunk& h = fd.hunks[i];
        out += h.header + "\n";
        for (const QString& l : h.lines) out += l + "\n";
    }
    return out;
}

QString DiffEngine::renderUnified(const DiffHunk& h) {
    const ThemeTokens tk = ThemeManager::instance().tokens();
    QString html = "<span style='color:" + tk.synKeyword.name() + ";'>" +
                   h.header.toHtmlEscaped() + "</span><br>";
    // kelime bazlı vurgu için önceki '-' satırını hatırla
    QString lastDel;
    for (const QString& ln : h.lines) {
        QString esc = ln.toHtmlEscaped();
        if (ln.startsWith('+')) {
            QString body = (lastDel.isEmpty())
                ? esc
                : WordDiff::highlightChanges(lastDel, ln.mid(1),
                                             AccentColor::soft(tk.success, tk.bg, 0.55).name(),
                                             tk.success.name()).prepend("+");
            html += "<span style='color:" + tk.success.name() + ";'>" + body + "</span><br>";
            lastDel.clear();
        }
        else if (ln.startsWith('-')) {
            html += "<span style='color:" + tk.error.name() + ";'>" + esc + "</span><br>";
            lastDel = ln.mid(1);
        }
        else {
            html += esc + "<br>";
            lastDel.clear();
        }
    }
    return html;
}

QString DiffEngine::renderSideBySide(const DiffHunk& h) {
    // Hizalı eski|yeni satırlar
    struct Row { QString oldL, newL; char kind; }; // ' ' aynı, 'd' sil, 'a' ekle
    QList<Row> rows;
    QList<QString> delBuf;
    auto flushDel = [&]() {
        for (const QString& d : delBuf) rows.append({d, "", 'd'});
        delBuf.clear();
    };
    for (const QString& ln : h.lines) {
        if (ln.startsWith('-')) delBuf << ln.mid(1);
        else if (ln.startsWith('+')) {
            if (!delBuf.isEmpty()) rows.append({delBuf.takeFirst(), ln.mid(1), ' '});
            else rows.append({"", ln.mid(1), 'a'});
        } else {
            flushDel();
            rows.append({ln.mid(1), ln.mid(1), ' '});
        }
    }
    flushDel();
    const ThemeTokens tk = ThemeManager::instance().tokens();
    const QString wBg = AccentColor::soft(tk.success, tk.bg, 0.55).name();
    const QString wFg = tk.success.name();
    QString html = "<table width='100%' cellspacing='0' cellpadding='2'>";
    int lo = h.oldStart, ln = h.newStart;
    auto cell = [](const QString& t, const QColor& bg) {
        return QString("<td bgcolor='%1'><font face='monospace'>%2</font></td>")
            .arg(bg.name(), t.toHtmlEscaped());
    };
    auto cellHtml = [](const QString& body, const QColor& bg) {
        return QString("<td bgcolor='%1'><font face='monospace'>%2</font></td>")
            .arg(bg.name(), body);
    };
    for (const Row& r : rows) {
        QString loobg = r.kind == ' ' ? "#1e1e1e" : "#3a1d1d";
        QString newbg = r.kind == ' ' ? "#1e1e1e" : (r.kind == 'a' ? "#1d3a1d" : "#1e1e1e");
        html += "<tr>";
        html += QString("<td><font face='monospace' color='#858585'>%1</font></td>").arg(r.oldL.isNull() ? "" : QString::number(lo));
        html += cell(r.oldL, QColor(loobg));
        html += QString("<td><font face='monospace' color='#858585'>%1</font></td>").arg(r.newL.isNull() ? "" : QString::number(ln));
        // Stage 10: değişen satırlarda yalnız farklı kelimeler vurgulanır
        if (r.kind == ' ' && r.oldL != r.newL)
            html += cellHtml(WordDiff::highlightChanges(r.oldL, r.newL, wBg, wFg), QColor(newbg));
        else
            html += cell(r.newL, QColor(newbg));
        html += "</tr>";
        if (!r.oldL.isNull()) ++lo;
        if (!r.newL.isNull()) ++ln;
    }
    return html + "</table>";
}

// ---------- Dialog ----------
DiffDialog::DiffDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Diff");
    resize(900, 600);
    auto* lay = new QVBoxLayout(this);
    auto* top = new QHBoxLayout();
    m_fileBox = new QComboBox(this);
    m_mode = new QComboBox(this);
    m_mode->addItems({"Birleşik", "Yan Yana"});
    m_mode->setCurrentIndex(1);
    top->addWidget(m_fileBox, 1);
    top->addWidget(new QLabel("Görünüm:", this));
    top->addWidget(m_mode);

    auto* mid = new QHBoxLayout();
    m_hunks = new QListWidget(this);
    m_hunks->setMaximumWidth(280);
    m_preview = new QTextEdit(this);
    m_preview->setReadOnly(true);
    m_preview->setFont(QFont("Consolas, monospace", 10));
    mid->addWidget(m_hunks);
    mid->addWidget(m_preview, 1);

    auto* btns = new QHBoxLayout();
    auto* bStage = new QPushButton("Hunk'u Stage'le", this);
    auto* bDiscard = new QPushButton("Hunk'u Geri Al", this);
    auto* bAll = new QPushButton("Tümünü Stage'le", this);
    auto* bClose = new QPushButton("Kapat", this);
    btns->addWidget(bStage);
    btns->addWidget(bDiscard);
    btns->addWidget(bAll);
    btns->addStretch(1);
    btns->addWidget(bClose);

    lay->addLayout(top);
    lay->addLayout(mid, 1);
    lay->addLayout(btns);

    connect(m_fileBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DiffDialog::renderFileList);
    connect(m_hunks, &QListWidget::currentRowChanged, this, &DiffDialog::refreshPreview);
    connect(m_mode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DiffDialog::refreshPreview);
    connect(bStage, &QPushButton::clicked, this, &DiffDialog::stageHunk);
    connect(bDiscard, &QPushButton::clicked, this, &DiffDialog::discardHunk);
    connect(bAll, &QPushButton::clicked, this, &DiffDialog::stageAll);
    connect(bClose, &QPushButton::clicked, this, &QDialog::accept);
}

void DiffDialog::setRepoDiff(const QString& repoRoot, const QString& diffText, bool cached) {
    m_repo = repoRoot;
    m_cached = cached;
    m_readOnly = false;
    m_files = DiffEngine::parseUnified(diffText);
    setWindowTitle(cached ? "Diff (staged)" : "Diff (unstaged)");
    renderFileList();
}

void DiffDialog::setExternalDiff(const QString& diffText, const QString& title) {
    m_readOnly = true;
    m_files = DiffEngine::parseUnified(diffText);
    setWindowTitle(title);
    renderFileList();
    // Stage butonlarını gizle (salt okunur)
    for (QPushButton* b : findChildren<QPushButton*>())
        if (b->text().contains("Stage") || b->text().contains("Geri Al")) b->setVisible(false);
}

void DiffDialog::renderFileList() {
    m_fileBox->blockSignals(true);
    m_fileBox->clear();
    for (const auto& f : m_files)
        m_fileBox->addItem(QString("%1 (%2 hunk)").arg(f.relPath).arg(f.hunks.size()));
    m_fileBox->blockSignals(false);
    m_hunks->clear();
    if (m_files.isEmpty()) {
        m_preview->setPlainText("(fark yok)");
        return;
    }
    int fi = qMax(0, m_fileBox->currentIndex());
    const FileDiff& fd = m_files[fi];
    for (int i = 0; i < fd.hunks.size(); ++i) {
        const DiffHunk& h = fd.hunks[i];
        auto* it = new QListWidgetItem(
            QString("@@ %1 +%2  (+%3/-%4)").arg(h.oldStart).arg(h.newStart).arg(h.adds()).arg(h.dels()));
        it->setData(Qt::UserRole, i);
        m_hunks->addItem(it);
    }
    m_hunks->setCurrentRow(0);
    refreshPreview();
}

void DiffDialog::refreshPreview() {
    if (m_files.isEmpty()) return;
    int fi = qMax(0, m_fileBox->currentIndex());
    int hi = m_hunks->currentRow();
    if (fi >= m_files.size() || hi < 0 || hi >= m_files[fi].hunks.size()) return;
    const DiffHunk& h = m_files[fi].hunks[hi];
    m_preview->setHtml(m_mode->currentIndex() == 1 ? DiffEngine::renderSideBySide(h)
                                                   : DiffEngine::renderUnified(h));
}

QString DiffDialog::runGit(const QStringList& args, int timeoutMs) {
    QProcess p(this);
    p.setWorkingDirectory(m_repo);
    p.start("git", args);
    p.waitForFinished(timeoutMs);
    return QString::fromUtf8(p.readAllStandardOutput()) + QString::fromUtf8(p.readAllStandardError());
}

static bool applyPatch(const QString& repo, const QString& patch, const QStringList& extra) {
    QProcess p;
    p.setWorkingDirectory(repo);
    QStringList args = {"apply"};
    args << extra;
    args << "-";
    p.start("git", args);
    p.write(patch.toUtf8());
    p.closeWriteChannel();
    p.waitForFinished(10000);
    return p.exitCode() == 0;
}

void DiffDialog::stageHunk() {
    if (m_readOnly || m_files.isEmpty()) return;
    int fi = qMax(0, m_fileBox->currentIndex());
    int hi = m_hunks->currentRow();
    if (fi >= m_files.size() || hi < 0) return;
    QString patch = DiffEngine::buildPatch(m_files[fi], {hi});
    bool ok = m_cached ? applyPatch(m_repo, patch, {"--cached", "-R", "--unidiff-zero"})
                       : applyPatch(m_repo, patch, {"--cached", "--unidiff-zero"});
    if (!ok) { QMessageBox::warning(this, "Diff", "Hunk uygulanamadı."); return; }
    // Yenile
    QString diff = runGit(m_cached ? QStringList({"diff", "--cached", "--no-color"})
                                   : QStringList({"diff", "--no-color"}));
    setRepoDiff(m_repo, diff, m_cached);
}

void DiffDialog::discardHunk() {
    if (m_readOnly || m_files.isEmpty()) return;
    auto r = QMessageBox::question(this, "Geri Al", "Seçili hunk'taki değişiklikler atılsın mı?",
                                   QMessageBox::Yes | QMessageBox::Cancel);
    if (r != QMessageBox::Yes) return;
    int fi = qMax(0, m_fileBox->currentIndex());
    int hi = m_hunks->currentRow();
    if (fi >= m_files.size() || hi < 0) return;
    QString patch = DiffEngine::buildPatch(m_files[fi], {hi});
    bool ok = m_cached ? applyPatch(m_repo, patch, {"--cached", "-R", "--unidiff-zero"})
                       : applyPatch(m_repo, patch, {"-R", "--unidiff-zero"});
    if (!ok) { QMessageBox::warning(this, "Diff", "Hunk geri alınamadı."); return; }
    QString diff = runGit(m_cached ? QStringList({"diff", "--cached", "--no-color"})
                                   : QStringList({"diff", "--no-color"}));
    setRepoDiff(m_repo, diff, m_cached);
}

void DiffDialog::stageAll() {
    if (m_readOnly) return;
    runGit({"add", "-A"});
    QString diff = runGit(m_cached ? QStringList({"diff", "--cached", "--no-color"})
                                   : QStringList({"diff", "--no-color"}));
    setRepoDiff(m_repo, diff, m_cached);
}
