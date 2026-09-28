#pragma once
#include "../core/PluginStore.h"
#include <QDialog>

class PluginEngine;
class QLabel;
class QListWidget;
class QPushButton;
class QTextEdit;
class QNetworkAccessManager;

// Eklenti Mağazası: kayıt deposundaki (git) eklentileri listeler,
// tek tıkla kurar/günceller/kaldırır. Kurulumda izin onayı ister.
// Ağ işlemleri eşzamansızdır (arayüz donmaz); düğmeler işlem bitene kilitli.
class PluginStoreDialog : public QDialog {
    Q_OBJECT
public:
    explicit PluginStoreDialog(PluginEngine* engine, QWidget* parent = nullptr);

signals:
    void changed(); // kur/güncelle/kaldır sonrası (yönetici listesini tazeler)

private slots:
    void refreshRemote();
    void installSelected();
    void updateAll();
    void nextUpdate();
    void removeSelected();
    void refreshList();

private:
    PluginStore::Entry curEntry() const;
    void setBusy(bool busy, const QString& msg);
    PluginEngine* m_eng = nullptr;
    QListWidget* m_list = nullptr;
    QTextEdit* m_info = nullptr;
    QLabel* m_status = nullptr;
    QPushButton* m_btnRefresh = nullptr;
    QPushButton* m_btnInstall = nullptr;
    QPushButton* m_btnUpdateAll = nullptr;
    QNetworkAccessManager* m_net = nullptr;
    QList<PluginStore::Entry> m_entries;
    QString m_lastError;
    QString m_offlineNote;
    QList<PluginStore::Entry> m_updateQueue;
    int m_updateDone = 0;
    QString m_updateError;
    bool m_loading = false;
};
