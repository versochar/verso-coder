#include "WelcomeView.h"
#include "../core/IconTheme.h"
#include "../core/ThemeManager.h"
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

WelcomeView::WelcomeView(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* host = new QWidget(scroll);
    auto* lay = new QVBoxLayout(host);
    lay->setContentsMargins(48, 40, 48, 40);
    lay->setSpacing(18);
    lay->addStretch(1);

    // Başlık
    auto* title = new QLabel("Verso Coder", host);
    title->setObjectName("welcomeTitle");
    lay->addWidget(title, 0, Qt::AlignHCenter);
    auto* sub = new QLabel("hafif, yerel ve hızlı — ne varsa buradan başlar", host);
    sub->setObjectName("welcomeSub");
    lay->addWidget(sub, 0, Qt::AlignHCenter);
    lay->addSpacing(10);

    lay->addWidget(buildQuickActions(), 0, Qt::AlignHCenter);
    lay->addWidget(buildRecents(), 0, Qt::AlignHCenter);
    lay->addWidget(buildShortcuts(), 0, Qt::AlignHCenter);
    lay->addWidget(buildDemoMode(), 0, Qt::AlignHCenter);
    lay->addStretch(2);

    scroll->setWidget(host);
    outer->addWidget(scroll);
}

QWidget* WelcomeView::buildQuickActions() {
    auto* card = new QFrame(this);
    card->setObjectName("welcomeCard");
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(24, 18, 24, 18);
    lay->setSpacing(10);

    auto* head = new QLabel("Hızlı Başlangıç", card);
    head->setObjectName("welcomeSection");
    lay->addWidget(head);

    struct Action { const char* icon; const char* title; const char* sub; const char* cmd; };
    const Action acts[] = {
        {"explorer", "Klasör Aç", "Projeyi gezginde göster", "file.openFolder"},
        {"file", "Yeni Dosya", "Boş sekme aç (Ctrl+N)", "file.new"},
        {"search", "Hızlı Aç", "Dosya adıyla bul (Ctrl+P)", "nav.quickOpen"},
        {"palette", "Komut Paleti", "Tüm komutlar (Ctrl+Shift+P)", "nav.palette"},
        {"palette", "Tema Galerisi", "10 tema + vurgu rengi", "view.theme"},
    };
    auto* rows = new QVBoxLayout();
    rows->setSpacing(4);
    for (const Action& a : acts) {
        auto* b = new QPushButton(card);
        b->setObjectName("welcomeAction");
        b->setCursor(Qt::PointingHandCursor);
        b->setToolTip(a.sub);
        auto* row = new QHBoxLayout(b);
        row->setContentsMargins(10, 8, 10, 8);
        row->setSpacing(10);
        auto* ic = new QLabel(b);
        ic->setPixmap(IconTheme::pixmap(a.icon, ThemeManager::instance().tokens().text, 18));
        auto* t1 = new QLabel(a.title, b);
        t1->setObjectName("welcomeActionTitle");
        auto* t2 = new QLabel(a.sub, b);
        t2->setObjectName("welcomeActionSub");
        row->addWidget(ic);
        row->addWidget(t1);
        row->addStretch(1);
        row->addWidget(t2);
        const QString cmd = a.cmd;
        connect(b, &QPushButton::clicked, this, [this, cmd]() { emit commandRequested(cmd); });
        // QPushButton boyut ipucunu iç düzenden almaz → içeriğe göre sabitle
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        b->setMinimumSize(row->totalSizeHint());
        rows->addWidget(b);
    }
    lay->addLayout(rows);
    return card;
}

QWidget* WelcomeView::buildRecents() {
    auto* card = new QFrame(this);
    card->setObjectName("welcomeCard");
    m_recentsHost = new QVBoxLayout(card);
    m_recentsHost->setContentsMargins(24, 18, 24, 18);
    m_recentsHost->setSpacing(6);
    m_recentsTitle = new QLabel("Son Dosyalar", card);
    m_recentsTitle->setObjectName("welcomeSection");
    m_recentsHost->addWidget(m_recentsTitle);
    return card;
}

QWidget* WelcomeView::buildShortcuts() {
    auto* card = new QFrame(this);
    card->setObjectName("welcomeCard");
    auto* lay = new QHBoxLayout(card);
    lay->setContentsMargins(24, 14, 24, 14);
    lay->setSpacing(24);
    const QPair<QString, QString> tips[] = {
        {"Ctrl+P", "Hızlı aç"}, {"Ctrl+Shift+P", "Komut paleti"},
        {"Ctrl+G", "Satıra git"}, {"F11", "Zen modu"},
        {"Ctrl+K, Ctrl+T", "Tema galerisi"},
    };
    for (const auto& t : tips) {
        auto* box = new QVBoxLayout();
        auto* k = new QLabel(t.first, card);
        k->setObjectName("welcomeKey");
        k->setAlignment(Qt::AlignHCenter);
        auto* d = new QLabel(t.second, card);
        d->setObjectName("welcomeKeySub");
        d->setAlignment(Qt::AlignHCenter);
        box->addWidget(k);
        box->addWidget(d);
        lay->addLayout(box);
    }
    return card;
}

void WelcomeView::setRecentFiles(const QStringList& files) {
    m_recent.clear();
    // geçersiz/kopya yolları ayıkla, en fazla 8 dosya
    for (const QString& f : files) {
        if (f.isEmpty() || !QFile::exists(f)) continue;
        if (!m_recent.contains(f)) m_recent << f;
        if (m_recent.size() >= 8) break;
    }

    // eski satırları temizle (başlık hariç)
    while (m_recentsHost->count() > 1) {
        QLayoutItem* it = m_recentsHost->takeAt(1);
        if (it->widget()) it->widget()->deleteLater();
        delete it;
    }

    if (m_recent.isEmpty()) {
        auto* none = new QLabel("Henüz dosya açılmadı — Ctrl+P ile başlayın", this);
        none->setObjectName("welcomeActionSub");
        m_recentsHost->addWidget(none);
        return;
    }

    const ThemeTokens tk = ThemeManager::instance().tokens();
    for (const QString& f : m_recent) {
        auto* b = new QPushButton(this);
        b->setObjectName("welcomeAction");
        b->setCursor(Qt::PointingHandCursor);
        b->setToolTip(f);
        auto* row = new QHBoxLayout(b);
        row->setContentsMargins(10, 6, 10, 6);
        row->setSpacing(10);
        auto* ic = new QLabel(b);
        ic->setPixmap(IconTheme::pixmap("file", tk.textDim, 16));
        auto* name = new QLabel(QFileInfo(f).fileName(), b);
        name->setObjectName("welcomeActionTitle");
        auto* path = new QLabel(QFileInfo(f).path(), b);
        path->setObjectName("welcomeActionSub");
        row->addWidget(ic);
        row->addWidget(name);
        row->addStretch(1);
        row->addWidget(path);
        connect(b, &QPushButton::clicked, this, [this, f]() { emit fileRequested(f); });
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        b->setMinimumSize(row->totalSizeHint());
        m_recentsHost->addWidget(b);
    }
}

QWidget* WelcomeView::buildDemoMode() {
    auto* card = new QFrame(this);
    card->setObjectName("welcomeCard");
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(24, 18, 24, 18);
    lay->setSpacing(10);

    auto* head = new QLabel("Canlı Demo Modu", card);
    head->setObjectName("welcomeSection");
    lay->addWidget(head);

    auto* desc = new QLabel("Demo modu, editorun Canlı İşbirliği özelliklerini hızlı preview yapmanızı sağlar.", card);
    desc->setObjectName("welcomeSub");
    lay->addWidget(desc);

    auto* btnStart = new QPushButton("Demo Başlat", card);
    btnStart->setObjectName("welcomeAction");
    btnStart->setCursor(Qt::PointingHandCursor);
    btnStart->setToolTip("Kamera/girdi yakalayıcı ile demo başlat");
    connect(btnStart, &QPushButton::clicked, this,
            [this]() { emit commandRequested("view.demoStart"); });
    lay->addWidget(btnStart);

    auto* btnStop = new QPushButton("Demo Durdur", card);
    btnStop->setObjectName("welcomeAction");
    btnStop->setCursor(Qt::PointingHandCursor);
    connect(btnStop, &QPushButton::clicked, this,
            [this]() { emit commandRequested("view.demoStop"); });
    lay->addWidget(btnStop);

    return card;
}
