#include "NewFileDialog.h"
#include "../core/LanguageSupport.h"
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

NewFileDialog::NewFileDialog(bool changeMode, QWidget* parent)
    : QDialog(parent), m_changeMode(changeMode) {
    setWindowTitle(changeMode ? "Dosya Dilini Değiştir" : "Yeni Dosya — Dil Seç");
    setMinimumWidth(320);
    auto* lay = new QVBoxLayout(this);
    if (!changeMode) {
        auto* hint = new QLabel("Dil seç; iskelet otomatik eklenir.", this);
        hint->setObjectName("welcomeActionSub");
        lay->addWidget(hint);
    }
    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText("Dil süz…");
    m_filter->setClearButtonEnabled(true);
    connect(m_filter, &QLineEdit::textChanged, this, &NewFileDialog::onFilter);
    lay->addWidget(m_filter);
    m_list = new QListWidget(this);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &NewFileDialog::onAccept);
    lay->addWidget(m_list, 1);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                     this);
    connect(box, &QDialogButtonBox::accepted, this, &NewFileDialog::onAccept);
    connect(box, &QDialogButtonBox::rejected, this, &NewFileDialog::reject);
    lay->addWidget(box);
    refill(QString());
    m_filter->setFocus();
}

void NewFileDialog::refill(const QString& filter) {
    m_list->clear();
    const QString f = filter.trimmed().toLower();
    auto addRow = [this](const QString& title, const QString& id, bool header) {
        auto* it = new QListWidgetItem(title, m_list);
        it->setData(Qt::UserRole, header ? QString() : id);
        if (header) {
            it->setFlags(Qt::NoItemFlags);
            it->setSelected(false);
        }
        return it;
    };
    const QStringList recent = LanguageSupport::recentLangs();
    const QList<LanguageSupport::LangDef> all = LanguageSupport::newFileLanguages();
    auto match = [&](const LanguageSupport::LangDef& d) {
        return f.isEmpty() || d.title.toLower().contains(f) || d.id.contains(f);
    };
    // Son kullanılanlar (süzme boşken)
    if (f.isEmpty() && !recent.isEmpty()) {
        addRow("Son kullanılanlar", QString(), true);
        for (const QString& id : recent)
            if (const LanguageSupport::LangDef* d = LanguageSupport::findLang(id))
                addRow(d->title + "  (" + d->exts.join(", ") + ")", d->id, false);
        addRow("Tüm diller", QString(), true);
    }
    for (const LanguageSupport::LangDef& d : all) {
        if (!match(d)) continue;
        const QString extra = d.exts.isEmpty() ? "uzantısız" : d.exts.join(", ");
        addRow(d.title + "  (" + extra + ")", d.id, false);
    }
    // Varsayılan seçim: ilk seçilebilir satır
    for (int i = 0; i < m_list->count(); ++i)
        if (!m_list->item(i)->data(Qt::UserRole).toString().isEmpty()) {
            m_list->setCurrentRow(i);
            break;
        }
}

void NewFileDialog::onFilter(const QString& text) {
    refill(text);
}

void NewFileDialog::onAccept() {
    auto* it = m_list->currentItem();
    const QString id = it ? it->data(Qt::UserRole).toString() : QString();
    if (id.isEmpty()) return; // başlık satırı — yoksay
    m_lang = id;
    accept();
}
