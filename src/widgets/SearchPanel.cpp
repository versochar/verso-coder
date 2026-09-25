#include "SearchPanel.h"
#include "../core/ReplaceEngine.h"
#include <QCheckBox>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QtConcurrent>

SearchPanel::SearchPanel(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    m_query = new QLineEdit(this);
    m_query->setPlaceholderText("Ara... (Enter)");
    m_query->setClearButtonEnabled(true);
    m_replace = new QLineEdit(this);
    m_replace->setPlaceholderText("Değiştir...");
    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText("Filtre: *.cpp *.h *.py (boş=tümü)");
    m_exclude = new QLineEdit(this); // Stage 17
    m_exclude->setPlaceholderText("Hariç: *test* build/* (boş=yok)");
    m_exclude->setClearButtonEnabled(true);
    m_regex = new QCheckBox("Regex", this);
    m_caseSens = new QCheckBox("Aa", this);
    m_caseSens->setToolTip("Büyük/küçük harf duyarlı");

    auto* optRow = new QHBoxLayout();
    optRow->addWidget(m_regex);
    optRow->addWidget(m_caseSens);
    optRow->addStretch(1);

    auto* btnRow = new QHBoxLayout();
    m_btnSearch = new QPushButton("Ara", this);
    m_btnSearch->setDefault(true);
    m_btnReplaceAll = new QPushButton("Tümünü Değiştir", this);
    m_btnReplaceFile = new QPushButton("Dosyada", this); // Stage 17
    m_btnReplaceFile->setToolTip("Seçili dosyadaki eşleşmeleri önizlemeyle değiştir");
    btnRow->addWidget(m_btnSearch, 1);
    btnRow->addWidget(m_btnReplaceAll, 1);
    btnRow->addWidget(m_btnReplaceFile, 1);

    m_status = new QLabel("", this);
    m_status->setStyleSheet("color:#858585;font-size:11px;");
    m_results = new QTreeWidget(this);
    m_results->setHeaderLabels({"Sonuçlar"});
    m_results->setRootIsDecorated(true);
    m_preview = new QTextEdit(this);
    m_preview->setReadOnly(true);
    m_preview->setMaximumHeight(130);
    m_preview->setFont(QFont("Consolas, monospace", 10));
    m_preview->setPlaceholderText("Önizleme: sonuç seç...");

    lay->addWidget(m_query);
    lay->addWidget(m_replace);
    lay->addWidget(m_filter);
    lay->addWidget(m_exclude); // Stage 17
    lay->addLayout(optRow);
    lay->addLayout(btnRow);
    lay->addWidget(m_status);
    lay->addWidget(m_results, 1);
    lay->addWidget(m_preview);

    connect(m_btnSearch, &QPushButton::clicked, this, &SearchPanel::runSearch);
    connect(m_query, &QLineEdit::returnPressed, this, &SearchPanel::runSearch);
    connect(m_btnReplaceAll, &QPushButton::clicked, this, &SearchPanel::replaceAll);
    connect(m_btnReplaceFile, &QPushButton::clicked, this,
            &SearchPanel::replaceFileSelected);
    connect(&m_watcher, &QFutureWatcher<QList<SearchHit>>::finished, this, &SearchPanel::onSearchDone);
    connect(m_results, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* it, int) {
        if (!it) return;
        QString f = it->data(0, Qt::UserRole + 1).toString();
        int line = it->data(0, Qt::UserRole + 2).toInt();
        if (!f.isEmpty()) emit fileOpened(f, line);
    });
    connect(m_results, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem* it, QTreeWidgetItem*) {
                if (!it) return;
                QString f = it->data(0, Qt::UserRole + 1).toString();
                int line = it->data(0, Qt::UserRole + 2).toInt();
                if (f.isEmpty() || line <= 0) return;
                m_preview->setPlainText(contextSnippet(f, line));
            });
}

QString SearchPanel::contextSnippet(const QString& file, int line, int radius) {
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QStringList lines = QString::fromUtf8(f.read(1 << 20)).split('\n');
    int from = qMax(1, line - radius), to = qMin(lines.size(), line + radius);
    QString out;
    for (int i = from; i <= to; ++i)
        out += QString("%1%2  %3\n").arg(i == line ? ">" : " ").arg(i, 4).arg(lines[i - 1].left(160));
    return out;
}

void SearchPanel::setRoot(const QString& root) { m_root = root; }
void SearchPanel::focusSearch() { m_query->setFocus(); m_query->selectAll(); }

QList<SearchHit> SearchPanel::searchInFile(const QString& file, const QString& query,
                                           bool useRegex, bool caseSens) {
    QList<SearchHit> out;
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) return out;
    QByteArray raw = f.read(1 << 20);
    if (raw.contains('\0')) return out;
    QString text = QString::fromUtf8(raw);
    if (query.isEmpty()) return out;

    QRegularExpression rx;
    if (useRegex) {
        auto cs = caseSens ? QRegularExpression::NoPatternOption
                           : QRegularExpression::CaseInsensitiveOption;
        rx = QRegularExpression(query, cs);
        if (!rx.isValid()) return out;
    }
    const QStringList lines = text.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        const QString& ln = lines[i];
        if (useRegex) {
            for (auto m = rx.match(ln); m.hasMatch(); m = rx.match(ln, m.capturedEnd()))
                out.append({file, i + 1, m.capturedStart() + 1, ln.trimmed().left(160)});
        } else {
            Qt::CaseSensitivity c = caseSens ? Qt::CaseSensitive : Qt::CaseInsensitive;
            int idx = ln.indexOf(query, 0, c);
            while (idx >= 0) {
                out.append({file, i + 1, idx + 1, ln.trimmed().left(160)});
                idx = ln.indexOf(query, idx + qMax(1, query.size()), c);
            }
        }
        if (out.size() > 5000) break;
    }
    return out;
}

QList<SearchHit> SearchPanel::runSearchSync(const QString& root, const QString& query,
                                            const QString& filter, bool useRegex,
                                            bool caseSens, int maxHits,
                                            const QString& exclude) {
    QList<SearchHit> hits;
    if (query.isEmpty() || root.isEmpty()) return hits;
    QDirIterator it(root, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    int scanned = 0;
    while (it.hasNext() && hits.size() < maxHits && scanned < 8000) {
        QString p = it.next();
        if (p.contains("/.git/") || p.contains("/build/") || p.contains("/node_modules/")) continue;
        // Stage 17: dahil + hariç glob (ReplaceEngine)
        if (!ReplaceEngine::fileAllowed(p, filter, exclude)) { ++scanned; continue; }
        hits.append(searchInFile(p, query, useRegex, caseSens));
        ++scanned;
    }
    return hits;
}

void SearchPanel::runSearch() {
    if (m_watcher.isRunning()) m_watcher.cancel();
    m_results->clear();
    m_hits.clear();
    if (m_query->text().isEmpty() || m_root.isEmpty()) return;
    m_btnSearch->setEnabled(false);
    m_status->setText("Aranıyor (arka plan)...");
    QString q = m_query->text(), flt = m_filter->text(), root = m_root;
    bool rx = m_regex->isChecked(), cs = m_caseSens->isChecked();
    const QString excl = m_exclude->text(); // Stage 17
    m_watcher.setFuture(QtConcurrent::run(runSearchSync, root, q, flt, rx, cs, 2000,
                                          excl));
}

void SearchPanel::onSearchDone() {
    m_btnSearch->setEnabled(true);
    if (m_watcher.isCanceled()) return;
    m_hits = m_watcher.result();
    QMap<QString, QList<SearchHit>> byFile;
    for (const auto& h : m_hits) byFile[h.file].append(h);
    m_results->setUpdatesEnabled(false);
    for (auto itf = byFile.begin(); itf != byFile.end(); ++itf) {
        auto* top = new QTreeWidgetItem(m_results,
            {QString("%1 (%2)").arg(QFileInfo(itf.key()).fileName()).arg(itf.value().size())});
        top->setData(0, Qt::UserRole + 1, itf.key());
        top->setData(0, Qt::UserRole + 2, itf.value().first().line);
        top->setExpanded(true);
        for (const auto& h : itf.value()) {
            auto* child = new QTreeWidgetItem(top,
                {QString("%1:%2  %3").arg(h.line).arg(h.col).arg(h.preview)});
            child->setData(0, Qt::UserRole + 1, h.file);
            child->setData(0, Qt::UserRole + 2, h.line);
        }
    }
    m_results->setUpdatesEnabled(true);
    m_status->setText(QString("%1 eşleşme").arg(m_hits.size()));
    if (m_hits.isEmpty()) {
        auto* none = new QTreeWidgetItem(m_results, {"Sonuç yok"});
        none->setDisabled(true);
    }
}

void SearchPanel::replaceAll() {
    if (m_hits.isEmpty()) return;
    auto r = QMessageBox::question(this, "Toplu değiştir",
        QString("%1 eşleşme '%2' ile değiştirilsin mi?").arg(m_hits.size()).arg(m_replace->text()),
        QMessageBox::Yes | QMessageBox::Cancel);
    if (r != QMessageBox::Yes) return;
    QMap<QString, QList<SearchHit>> byFile;
    for (const auto& h : m_hits) byFile[h.file].append(h);
    int files = 0;
    for (auto itf = byFile.begin(); itf != byFile.end(); ++itf) {
        applyReplace(itf.key(), itf.value());
        ++files;
    }
    runSearch();
    QMessageBox::information(this, "Değiştir", QString("%1 dosyada değiştirildi.").arg(files));
}

void SearchPanel::applyReplace(const QString& file, const QList<SearchHit>&) {
    // Stage 17: ReplaceEngine — durum korumalı, satır bazlı
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QString text = QString::fromUtf8(f.readAll());
    f.close();
    auto [next, edits] = ReplaceEngine::applyFile(file, text, m_query->text(),
        m_replace->text(), m_regex->isChecked(), m_caseSens->isChecked());
    if (edits.isEmpty()) return;
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    f.write(next.toUtf8());
}

// Stage 17: seçili dosyanın değişiklik önizlemesi + tek dosyada uygulama
void SearchPanel::previewReplaceFor(const QString& file) {
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QString text = QString::fromUtf8(f.read(1 << 20));
    f.close();
    auto [next, edits] = ReplaceEngine::applyFile(file, text, m_query->text(),
        m_replace->text(), m_regex->isChecked(), m_caseSens->isChecked());
    Q_UNUSED(next);
    if (edits.isEmpty()) {
        m_preview->setPlainText("(bu dosyada değişiklik yok)");
        return;
    }
    QStringList out;
    out << QString("%1 — %2 satır:").arg(file).arg(edits.size());
    for (const ReplaceEdit& e : edits.mid(0, 30)) {
        out << QString("  %1: - %2").arg(e.line1).arg(e.before);
        out << QString("  %1: + %2").arg(e.line1).arg(e.after);
    }
    if (edits.size() > 30) out << QString("  ... +%1 satır").arg(edits.size() - 30);
    m_preview->setPlainText(out.join('\n'));
}

void SearchPanel::replaceFileSelected() {
    auto* it = m_results->currentItem();
    if (!it) return;
    // Dosya düğümü de, isabet düğümü de UserRole+1'de dosya yolunu taşır
    const QString file = it->data(0, Qt::UserRole + 1).toString();
    if (file.isEmpty()) return;
    previewReplaceFor(file);
    auto r = QMessageBox::question(this, "Dosyada değiştir",
        QString("%1\nbu dosyada değiştirilsin mi?").arg(file),
        QMessageBox::Yes | QMessageBox::Cancel);
    if (r != QMessageBox::Yes) return;
    applyReplace(file, {});
    runSearch();
}

void SearchPanel::focusExclude() {
    m_exclude->setFocus();
    m_exclude->selectAll();
}
