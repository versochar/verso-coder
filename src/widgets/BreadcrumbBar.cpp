#include "BreadcrumbBar.h"
#include "../core/FileIcons.h"
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>

BreadcrumbBar::BreadcrumbBar(QWidget* parent) : QWidget(parent) {
    m_lay = new QHBoxLayout(this);
    m_lay->setContentsMargins(4, 2, 4, 2);
    m_lay->setSpacing(2);
    m_symbols = nullptr;
}

QList<CrumbSymbol> BreadcrumbBar::parseSymbols(const QString& text) {
    QList<CrumbSymbol> out;
    static const QRegularExpression rxClass("^\\s*(class|struct)\\s+(\\w+)");
    static const QRegularExpression rxFunc("^\\s*[\\w:<>\\*&,~]+\\s+([\\w:~]+)\\s*\\([^;]*\\)\\s*(const)?\\s*(\\{|$)");
    static const QRegularExpression rxPy("^\\s*(def|class)\\s+(\\w+)");
    const QStringList lines = text.left(200000).split('\n'); // 200KB cap
    for (int i = 0; i < lines.size(); ++i) {
        const QString& ln = lines[i];
        QRegularExpressionMatch m;
        if ((m = rxPy.match(ln)).hasMatch()) {
            const bool isClass = (m.captured(1) == "class");
            out.append({i + 1, m.captured(2), isClass ? "class" : "func"});
        } else if ((m = rxClass.match(ln)).hasMatch())
            out.append({i + 1, m.captured(2), "class"});
        else if ((m = rxFunc.match(ln)).hasMatch()) {
            QString name = m.captured(1);
            if (name.size() < 40 && !name.contains("if") && !name.contains("for"))
                out.append({i + 1, name + "()", "func"});
        }
        if (out.size() > 300) break;
    }
    return out;
}

void BreadcrumbBar::setPath(const QString& root, const QString& filePath, const QString& docText) {
    QLayoutItem* it;
    while ((it = m_lay->takeAt(0)) != nullptr) { delete it->widget(); delete it; }
    m_symbols = nullptr;
    if (filePath.isEmpty()) {
        m_lay->addWidget(new QLabel("—", this));
        m_lay->addStretch(1);
        return;
    }
    QDir r(root);
    QString rel = r.relativeFilePath(filePath);
    QStringList parts = rel.split('/', Qt::SkipEmptyParts);
    QString accum = root;
    auto* bRoot = new QPushButton(QFileInfo(root).fileName().isEmpty() ? root : QFileInfo(root).fileName(), this);
    bRoot->setFlat(true);
    bRoot->setStyleSheet("text-align:left;padding:2px 4px;color:#858585;");
    connect(bRoot, &QPushButton::clicked, this, [this, root]() { emit crumbClicked(root); });
    m_lay->addWidget(bRoot);
    FileIconProvider icons;
    for (int i = 0; i < parts.size(); ++i) {
        m_lay->addWidget(new QLabel("›", this));
        accum += "/" + parts[i];
        QString acc = accum;
        bool isFile = (i == parts.size() - 1);
        auto* b = new QPushButton(parts[i], this);
        b->setFlat(true);
        // Stage 11: dosya ikonları
        if (isFile) b->setIcon(icons.icon(QFileInfo(acc)));
        b->setStyleSheet(QString("text-align:left;padding:2px 4px;%1").arg(isFile ? "font-weight:bold;" : "color:#858585;"));
        connect(b, &QPushButton::clicked, this, [this, acc]() { emit crumbClicked(acc); });
        m_lay->addWidget(b);
    }
    m_lay->addStretch(1);
    // Sembol kutusu: tür önekli, sınıflar önce
    m_symbols = new QComboBox(this);
    m_symbols->setMaximumWidth(240);
    m_symbols->setToolTip("Sınıf / fonksiyon (tıkla: satıra git)");
    m_symbols->addItem("Sembol yok");
    auto syms = parseSymbols(docText);
    QList<CrumbSymbol> classes, funcs;
    for (const auto& s : syms)
        (s.kind == "class" ? classes : funcs).append(s);
    const QList<CrumbSymbol> ordered = classes + funcs;
    for (const auto& s : ordered) {
        const QString glyph = (s.kind == "class") ? "▣ " : "ƒ ";
        m_symbols->addItem(QString("%1%2  :%3").arg(glyph, s.name).arg(s.line), s.line);
    }
    connect(m_symbols, QOverload<int>::of(&QComboBox::activated), this, [this, ordered](int idx) {
        if (idx > 0 && idx - 1 < ordered.size()) emit symbolActivated(ordered[idx - 1].line);
    });
    m_lay->addWidget(m_symbols);
}
