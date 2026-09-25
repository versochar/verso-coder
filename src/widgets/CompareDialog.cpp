#include "CompareDialog.h"
#include <QCryptographicHash>
#include <QColor>
#include <QSet>
#include <algorithm>
#include <QCheckBox>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

CompareDialog::CompareDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Klasör Karşılaştır");
    resize(640, 480);
    auto* lay = new QVBoxLayout(this);
    auto* ra = new QHBoxLayout();
    m_a = new QLineEdit(this);
    auto* bA = new QPushButton("...", this);
    bA->setFixedWidth(32);
    ra->addWidget(new QLabel("A:", this));
    ra->addWidget(m_a, 1);
    ra->addWidget(bA);
    auto* rb = new QHBoxLayout();
    m_b = new QLineEdit(this);
    auto* bB = new QPushButton("...", this);
    bB->setFixedWidth(32);
    rb->addWidget(new QLabel("B:", this));
    rb->addWidget(m_b, 1);
    rb->addWidget(bB);
    auto* rc = new QHBoxLayout();
    m_hideSame = new QCheckBox("Aynı olanları gizle", this);
    m_hideSame->setChecked(true);
    auto* bRun = new QPushButton("Karşılaştır", this);
    rc->addWidget(m_hideSame, 1);
    rc->addWidget(bRun);
    m_list = new QTreeWidget(this);
    m_list->setHeaderLabels({"Durum", "Dosya"});
    lay->addLayout(ra);
    lay->addLayout(rb);
    lay->addLayout(rc);
    lay->addWidget(m_list, 1);

    connect(bA, &QPushButton::clicked, this, [this]() {
        QString d = QFileDialog::getExistingDirectory(this, "A dizini", m_a->text());
        if (!d.isEmpty()) m_a->setText(d);
    });
    connect(bB, &QPushButton::clicked, this, [this]() {
        QString d = QFileDialog::getExistingDirectory(this, "B dizini", m_b->text());
        if (!d.isEmpty()) m_b->setText(d);
    });
    connect(bRun, &QPushButton::clicked, this, &CompareDialog::run);
    connect(m_hideSame, &QCheckBox::toggled, this, &CompareDialog::run);
    connect(m_list, &QTreeWidget::itemDoubleClicked, this, &CompareDialog::openDiff);
}

void CompareDialog::setDirA(const QString& dir) {
    m_a->setText(dir);
}

static QMap<QString, QFileInfo> walkFiles(const QString& root) {
    QMap<QString, QFileInfo> out;
    QDirIterator it(root, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    int n = 0;
    while (it.hasNext() && n < 20000) {
        QString p = it.next();
        if (p.contains("/.git/")) continue;
        out[QDir(root).relativeFilePath(p)] = QFileInfo(p);
        ++n;
    }
    return out;
}

bool CompareDialog::filesEqual(const QString& fa, const QString& fb) {
    QFileInfo a(fa), b(fb);
    if (a.size() != b.size()) return false;
    if (a.size() > 50 * 1024 * 1024) return a.lastModified() == b.lastModified();
    QFile fa1(fa), fb1(fb);
    if (!fa1.open(QIODevice::ReadOnly) || !fb1.open(QIODevice::ReadOnly)) return false;
    QCryptographicHash ha(QCryptographicHash::Sha1), hb(QCryptographicHash::Sha1);
    while (!fa1.atEnd()) { ha.addData(fa1.read(1 << 16)); hb.addData(fb1.read(1 << 16)); }
    return ha.result() == hb.result() && !fa1.atEnd() == !fb1.atEnd();
}

QList<CompareRow> CompareDialog::compareDirs(const QString& a, const QString& b) {
    QList<CompareRow> out;
    auto wa = walkFiles(a);
    auto wb = walkFiles(b);
    QSet<QString> keys;
    for (auto it = wa.begin(); it != wa.end(); ++it) keys.insert(it.key());
    for (auto it = wb.begin(); it != wb.end(); ++it) keys.insert(it.key());
    for (const QString& rel : keys) {
        bool inA = wa.contains(rel), inB = wb.contains(rel);
        if (inA && !inB) out.append({rel, "sadeceA"});
        else if (!inA && inB) out.append({rel, "sadeceB"});
        else if (filesEqual(wa[rel].absoluteFilePath(), wb[rel].absoluteFilePath()))
            out.append({rel, "ayni"});
        else out.append({rel, "farkli"});
    }
    std::sort(out.begin(), out.end(),
              [](const CompareRow& x, const CompareRow& y) { return x.rel < y.rel; });
    return out;
}

void CompareDialog::run() {
    m_list->clear();
    if (m_a->text().isEmpty() || m_b->text().isEmpty()) return;
    for (const auto& r : compareDirs(m_a->text(), m_b->text())) {
        if (r.status == "ayni" && m_hideSame->isChecked()) continue;
        QString label = r.status == "sadeceA" ? "sadece A" : r.status == "sadeceB" ? "sadece B"
                        : r.status == "farkli"    ? "farklı"
                                                  : "aynı";
        auto* it = new QTreeWidgetItem(m_list, {label, r.rel});
        it->setData(0, Qt::UserRole, r.status);
        it->setForeground(0, r.status == "farkli"   ? QColor("#ce9178")
                           : r.status == "sadeceA"  ? QColor("#569cd6")
                           : r.status == "sadeceB"  ? QColor("#6a9955")
                                                    : QColor("#858585"));
        m_list->addTopLevelItem(it);
    }
    m_list->resizeColumnToContents(0);
}

void CompareDialog::openDiff(QTreeWidgetItem* it) {
    if (!it || it->data(0, Qt::UserRole).toString() != "farkli") return;
    QString rel = it->text(1);
    emit diffRequested(m_a->text() + "/" + rel, m_b->text() + "/" + rel, "Karşılaştır: " + rel);
}
