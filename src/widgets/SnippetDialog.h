#pragma once
#include "../core/SnippetManager.h"
#include <QDialog>

class QListWidget;

// Stage 17: snippet paleti — ekle/çalıştır/sil + özel snippet düzenleme.
class SnippetDialog : public QDialog {
    Q_OBJECT
public:
    explicit SnippetDialog(const QString& suffix, QWidget* parent = nullptr);

signals:
    void insertRequested(const SnippetDef& s);

private slots:
    void refresh();
    void addCustom();
    void removeSelected();
    void insertSelected();

private:
    QListWidget* m_list;
    QString m_suffix;
    QList<SnippetDef> m_items;
};
