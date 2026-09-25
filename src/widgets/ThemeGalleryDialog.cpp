#include "ThemeGalleryDialog.h"
#include "../core/AccentColor.h"
#include "../core/ThemeStore.h"
#include <QColorDialog>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

// ---------- ThemeCard ----------
ThemeCard::ThemeCard(const ThemeTokens& t, QWidget* parent)
    : QWidget(parent), m_t(t) {
    setFixedSize(168, 132);
    setCursor(Qt::PointingHandCursor);
    setToolTip(t.name + (t.dark ? " (koyu)" : " (açık)"));
}

void ThemeCard::setCurrent(bool on) {
    if (m_current == on) return;
    m_current = on;
    update();
}

void ThemeCard::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // kart zemin: editör arka planı
    QRectF r = rect().adjusted(4, 4, -4, -4);
    p.setPen(Qt::NoPen);
    p.setBrush(m_t.bg);
    p.drawRoundedRect(r, 8, 8);

    // üst bant: sekme çubuğu (surface) + accent alt çizgi
    QRectF bar(r.left(), r.top(), r.width(), 16);
    p.setBrush(m_t.surface);
    p.drawRoundedRect(bar, 8, 8);
    p.drawRect(QRectF(bar.left(), bar.top() + 8, bar.width(), 8)); // alt köşeleri kapat
    p.setBrush(AccentColor::soft(m_t.accent, m_t.bg, 1.0));
    p.drawRect(QRectF(bar.left() + 10, bar.bottom() - 2, 34, 2));

    // gutter şeridi
    p.setBrush(m_t.gutterBg);
    p.drawRect(QRectF(r.left(), bar.bottom(), 20, r.height() - 16 - 26));

    // kod satırları (sözdizimi renkleri)
    const QColor lines[] = {m_t.synKeyword, m_t.synType, m_t.synString, m_t.synFunc,
                            m_t.synComment, m_t.synNumber, m_t.synKeyword, m_t.synType};
    const int widths[] = {52, 38, 64, 30, 48, 26, 58, 34};
    double y = bar.bottom() + 10;
    for (int i = 0; i < 8; ++i) {
        p.setBrush(lines[i]);
        p.drawRoundedRect(QRectF(r.left() + 28, y, widths[i], 4), 2, 2);
        y += 9;
    }

    // alt buton şeridi: accent "buton"
    p.setBrush(m_t.accent);
    p.drawRoundedRect(QRectF(r.left() + 8, r.bottom() - 18, 46, 10), 5, 5);
    p.setBrush(m_t.surfaceAlt);
    p.drawRoundedRect(QRectF(r.left() + 60, r.bottom() - 18, 30, 10), 5, 5);
    p.setBrush(ThemeTokens::withAlphaF(m_t.error, 0.8));
    p.drawRoundedRect(QRectF(r.right() - 18, r.bottom() - 18, 10, 10), 5, 5);

    // seçim / odak çerçevesi
    if (m_current) {
        QPen pen(m_t.accent, 2);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r.adjusted(1, 1, -1, -1), 8, 8);
    } else {
        QPen pen(m_t.border, 1);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r.adjusted(1, 1, -1, -1), 8, 8);
    }

    // tema adı
    p.setPen(m_t.text);
    QFont f = font();
    f.setPointSizeF(7.5);
    f.setBold(true);
    p.setFont(f);
    p.drawText(QRectF(r.left(), r.bottom() + 5, r.width(), 14),
               Qt::AlignHCenter, m_t.name);
}

void ThemeCard::mouseDoubleClickEvent(QMouseEvent*) {
    emit activated(m_t.name);
}

// ---------- ThemeGalleryDialog ----------
ThemeGalleryDialog::ThemeGalleryDialog(const QString& currentTheme,
                                       const QColor& currentAccent, QWidget* parent)
    : QDialog(parent), m_current(currentTheme), m_accent(currentAccent) {
    setWindowTitle("Tema Galerisi");
    resize(660, 520);

    auto* lay = new QVBoxLayout(this);

    // accent satırı
    auto* accRow = new QHBoxLayout();
    accRow->addWidget(new QLabel("Vurgu rengi:", this));
    auto* bTheme = new QPushButton("Tema rengi", this);
    bTheme->setToolTip("Seçili temanın kendi vurgu rengi kullanılır");
    connect(bTheme, &QPushButton::clicked, this, [this]() {
        m_accent = QColor();
        if (m_accentLabel) m_accentLabel->setText("tema varsayılanı");
        emit accentSelected(m_accent);
    });
    accRow->addWidget(bTheme);
    for (const QString& n : AccentColor::presetNames()) {
        const QColor c = AccentColor::preset(n);
        auto* b = new QPushButton(this);
        b->setFixedSize(26, 22);
        b->setToolTip(n);
        b->setStyleSheet(QString("background:%1;border:1px solid #888;border-radius:5px;")
                             .arg(c.name()));
        connect(b, &QPushButton::clicked, this, [this, c]() {
            m_accent = c;
            if (m_accentLabel) m_accentLabel->setText(m_accent.name());
            emit accentSelected(m_accent);
        });
        accRow->addWidget(b);
    }
    auto* bCustom = new QPushButton("Özel...", this);
    connect(bCustom, &QPushButton::clicked, this, [this]() {
        const QColor c = QColorDialog::getColor(
            m_accent.isValid() ? m_accent : QColor("#007acc"), this, "Vurgu Rengi Seç");
        if (!c.isValid()) return;
        m_accent = c;
        if (m_accentLabel) m_accentLabel->setText(m_accent.name());
        emit accentSelected(m_accent);
    });
    accRow->addWidget(bCustom);
    m_accentLabel = new QLabel(m_accent.isValid() ? m_accent.name() : "tema varsayılanı", this);
    m_accentLabel->setStyleSheet("color:#888;");
    accRow->addWidget(m_accentLabel);
    accRow->addStretch(1);
    lay->addLayout(accRow);

    // kart ızgarası (kaydırılabilir)
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    auto* host = new QWidget(scroll);
    m_grid = new QGridLayout(host);
    m_grid->setSpacing(10);
    scroll->setWidget(host);
    lay->addWidget(scroll, 1);

    buildCards();

    // alt butonlar
    auto* row = new QHBoxLayout();
    auto* bImport = new QPushButton("Tema İçe Aktar...", this);
    auto* bExport = new QPushButton("Dışa Aktar...", this);
    auto* bClose = new QPushButton("Kapat", this);
    connect(bImport, &QPushButton::clicked, this, [this]() {
        const QString f = QFileDialog::getOpenFileName(this, "Tema JSON'u Seç",
                                                       QString(), "JSON (*.json)");
        if (f.isEmpty()) return;
        QString name;
        if (ThemeStore::instance().importTheme(f, &name)) {
            buildCards();
            QMessageBox::information(this, "Tema", "İçe aktarıldı: " + name);
        } else {
            QMessageBox::warning(this, "Tema", "Geçersiz tema dosyası.");
        }
    });
    connect(bExport, &QPushButton::clicked, this, [this]() {
        const QString f = QFileDialog::getSaveFileName(this, "Temayı Dışa Aktar",
                                                       m_current + ".json", "JSON (*.json)");
        if (f.isEmpty()) return;
        if (!ThemeStore::instance().exportTheme(m_current, f))
            QMessageBox::warning(this, "Tema", "Dışa aktarılamadı.");
    });
    connect(bClose, &QPushButton::clicked, this, &QDialog::accept);
    row->addWidget(bImport);
    row->addWidget(bExport);
    row->addStretch(1);
    row->addWidget(bClose);
    lay->addLayout(row);
}

void ThemeGalleryDialog::buildCards() {
    // eskileri temizle
    for (ThemeCard* c : m_cards) c->deleteLater();
    m_cards.clear();
    if (m_grid) {
        while (m_grid->count()) {
            QLayoutItem* it = m_grid->takeAt(0);
            delete it;
        }
    }

    const QStringList names = ThemeStore::instance().themeNames();
    int row = 0, col = 0;
    for (const QString& n : names) {
        const ThemeTokens t = ThemeStore::instance().theme(n);
        if (!t.isValid()) continue;
        auto* card = new ThemeCard(t, this);
        card->setCurrent(n == m_current);
        connect(card, &ThemeCard::activated, this, [this, card](const QString& name) {
            m_current = name;
            for (ThemeCard* c : m_cards) c->setCurrent(c->themeName() == name);
            emit themeSelected(name);
        });
        m_grid->addWidget(card, row, col);
        m_cards << card;
        if (++col >= 3) { col = 0; ++row; }
    }
}
