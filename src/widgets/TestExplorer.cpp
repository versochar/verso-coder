#include "TestExplorer.h"
#include "../core/SettingsManager.h"
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QSplitter>
#include <QPlainTextEdit>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>

TestExplorer::TestExplorer(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    m_bar = new QToolBar(this);
    m_bar->addAction("🔍 Keşfet", this, &TestExplorer::discoverRequested);
    m_bar->addAction("▶ Tümünü Çalıştır", this, &TestExplorer::runAllRequested);
    m_bar->addAction("↻ Başarısızlar", this, &TestExplorer::runFailedRequested);
    m_bar->addAction("■ Durdur", this, &TestExplorer::stopRequested);
    // Stage 25: kaydetmede ilgili testi koştur
    auto* bAuto = new QCheckBox("Otomatik (kaydetmede)", this);
    bAuto->setToolTip("Kaydedilen dosyanın ilgili testini arka planda koşturur");
    bAuto->setChecked(SettingsManager::instance().load().testOnSave);
    connect(bAuto, &QCheckBox::toggled, this, [](bool on) {
        AppSettings s = SettingsManager::instance().load();
        s.testOnSave = on;
        SettingsManager::instance().save(s);
    });
    m_bar->addWidget(bAuto);
    m_filter = new QLineEdit(m_bar);
    m_filter->setPlaceholderText("Süz...");
    m_filter->setClearButtonEnabled(true);
    m_filter->setMaximumWidth(160);
    m_bar->addWidget(m_filter);
    connect(m_filter, &QLineEdit::textChanged, this, &TestExplorer::applyFilter);
    lay->addWidget(m_bar);

    auto* split = new QSplitter(Qt::Vertical, this);
    m_tree = new QTreeWidget(split);
    m_tree->setHeaderLabels({"Test", "Durum", "Süre"});
    m_tree->setRootIsDecorated(true);
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this,
            &TestExplorer::onItemDoubleClicked);
    m_log = new QPlainTextEdit(split);
    m_log->setReadOnly(true);
    QFont logFont("monospace");
    logFont.setStyleHint(QFont::Monospace);
    m_log->setFont(logFont);
    m_log->setMaximumBlockCount(2000);
    split->setStretchFactor(0, 2);
    split->setStretchFactor(1, 1);
    lay->addWidget(split, 1);

    auto* bottom = new QHBoxLayout();
    m_bar2 = new QProgressBar(this);
    m_bar2->setTextVisible(false);
    m_bar2->setMaximumHeight(10);
    m_summary = new QLabel("Test yok", this);
    bottom->addWidget(m_bar2, 1);
    bottom->addWidget(m_summary);
    lay->addLayout(bottom);
}

void TestExplorer::setTests(const QList<TestCase>& tests) {
    m_tests = tests;
    m_failed.clear();
    m_tree->clear();
    QMap<QString, QTreeWidgetItem*> groups;
    for (const TestCase& t : tests) {
        const QString g = t.suite.isEmpty() ? "(genel)" : t.suite;
        QTreeWidgetItem* top = groups.value(g);
        if (!top) {
            top = new QTreeWidgetItem(m_tree, QStringList(g));
            groups[g] = top;
        }
        auto* it = new QTreeWidgetItem(top, QStringList({t.name, "—", ""}));
        it->setData(0, Qt::UserRole, t.id());
    }
    m_tree->expandAll();
    m_summary->setText(QString("%1 test bulundu").arg(tests.size()));
}

void TestExplorer::setResults(const QList<TestCase>& results) {
    QMap<QString, QString> status;
    QMap<QString, double> times;
    QMap<QString, QString> details;
    for (const TestCase& t : results) {
        status[t.id()] = t.status;
        times[t.id()] = t.timeMs;
        if (!t.detail.isEmpty()) details[t.id()] = t.detail;
    }
    int pass = 0, fail = 0, skip = 0;
    m_failed.clear();
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        const QString id = (*it)->data(0, Qt::UserRole).toString();
        if (!id.isEmpty() && status.contains(id)) {
            const QString st = status[id];
            (*it)->setText(1, st == "pass" ? "✓" : (st == "fail" ? "✗" : "○"));
            (*it)->setForeground(1, st == "pass" ? QColor("#4ec9b0")
                                 : (st == "fail" ? QColor("#f44747") : QColor("#858585")));
            if (times[id] > 0)
                (*it)->setText(2, QString("%1 ms").arg(times[id], 0, 'f', 0));
            if (details.contains(id)) (*it)->setToolTip(0, details[id]);
            if (st == "pass") ++pass;
            else if (st == "fail") {
                ++fail;
                if (!m_failed.contains(id)) m_failed << id;
            } else ++skip;
        }
        ++it;
    }
    setSummary(pass, fail, skip);
    const int total = pass + fail + skip;
    m_bar2->setRange(0, qMax(1, total));
    m_bar2->setValue(pass + fail + skip);
}

void TestExplorer::setRunning(bool on) {
    m_bar2->setRange(0, 0);
    if (!on) m_bar2->setRange(0, 1);
}

void TestExplorer::appendLog(const QString& text) {
    m_log->moveCursor(QTextCursor::End);
    m_log->insertPlainText(text);
    m_log->moveCursor(QTextCursor::End);
}

void TestExplorer::clearLog() {
    m_log->clear();
}

QString TestExplorer::logText() const {
    return m_log->toPlainText();
}

void TestExplorer::setSummary(int pass, int fail, int skip) {
    m_summary->setText(QString("✓ %1  ✗ %2  ○ %3").arg(pass).arg(fail).arg(skip));
}

void TestExplorer::onItemDoubleClicked() {
    auto* it = m_tree->currentItem();
    if (!it) return;
    const QString id = it->data(0, Qt::UserRole).toString();
    if (!id.isEmpty()) emit runOneRequested(id);
}

void TestExplorer::applyFilter(const QString& text) {
    const QString q = text.trimmed().toLower();
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        QTreeWidgetItem* item = *it;
        if (item->childCount() > 0) {
            // Grup: çocuğu görünen varsa açık tut
            bool gorunur = q.isEmpty();
            for (int i = 0; i < item->childCount() && !gorunur; ++i) {
                QTreeWidgetItem* c = item->child(i);
                if (c->text(0).toLower().contains(q) ||
                    c->data(0, Qt::UserRole).toString().toLower().contains(q))
                    gorunur = true;
            }
            if (q.isEmpty() || item->text(0).toLower().contains(q)) gorunur = true;
            item->setHidden(!gorunur);
            item->setExpanded(gorunur && !q.isEmpty());
        } else {
            const bool gorunur =
                q.isEmpty() || item->text(0).toLower().contains(q) ||
                item->data(0, Qt::UserRole).toString().toLower().contains(q);
            item->setHidden(!gorunur);
        }
        ++it;
    }
}
