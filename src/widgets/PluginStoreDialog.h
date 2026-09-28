#pragma once
#include "../core/PluginStore.h"
#include <QDialog>

class PluginEngine;
class QListWidget;
class QTextEdit;

// Eklenti Mağazası: kayıt deposundaki (git) eklentileri listeler,
// tek tıkla kurar/günceller/kaldırır. Kurulumda izin onayı ister.
class PluginStoreDialog : public QDialog {
    Q_OBJECT
public:
    explicit PluginStoreDialog(PluginEngine* engine, QWidget* parent = nullptr);

signals:
    void changed(); // kur/güncelle/kaldır sonrası (yönetici listesini tazeler)

private slots:
    void refreshRemote();
    void installSelected();
    void removeSelected();
    void refreshList();

private:
    PluginStore::Entry curEntry() const;
    PluginEngine* m_eng = nullptr;
    QListWidget* m_list = nullptr;
    QTextEdit* m_info = nullptr;
    QList<PluginStore::Entry> m_entries;
    QString m_lastError;
};
