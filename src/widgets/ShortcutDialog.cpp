#include "ShortcutDialog.h"
#include "../core/Commands.h"
#include "../core/SettingsManager.h"
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QTreeWidget>
#include <QVBoxLayout>

ShortcutDialog::ShortcutDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Klavye Kısayolları");
    resize(520, 480);
    auto* lay = new QVBoxLayout(this);
    auto* filter = new QLineEdit(this);
    filter->setPlaceholderText("Süz…");
    filter->setClearButtonEnabled(true);
    lay->addWidget(filter);
    auto* tree = new QTreeWidget(this);
    tree->setHeaderLabels({"Eylem", "Tuş"});
    tree->setRootIsDecorated(false);
    tree->setColumnWidth(0, 320);
    lay->addWidget(tree, 1);
    const AppSettings s = SettingsManager::instance().load();
    auto refill = [&](const QString& f) {
        tree->clear();
        const QString fl = f.trimmed().toLower();
        for (const UiCommand& c : defaultCommands()) {
            const QString k = effectiveKeys(s.shortcuts, c);
            if (!fl.isEmpty() && !c.title.toLower().contains(fl) &&
                !k.toLower().contains(fl))
                continue;
            auto* it = new QTreeWidgetItem(tree, {c.title, k});
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            Q_UNUSED(it);
        }
    };
    refill(QString());
    connect(filter, &QLineEdit::textChanged, this, refill);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(box, &QDialogButtonBox::rejected, this, &ShortcutDialog::reject);
    lay->addWidget(box);
}
