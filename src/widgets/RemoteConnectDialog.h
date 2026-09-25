#pragma once
#include "../core/ConnectionProfile.h"
#include <QDialog>

class QComboBox;
class QCheckBox;
class QLineEdit;
class QSpinBox;

// Stage 16: uzak bağlantı diyaloğu — profil seç/yeni/düzenle/sil + bağlan.
class RemoteConnectDialog : public QDialog {
    Q_OBJECT
public:
    explicit RemoteConnectDialog(QWidget* parent = nullptr);
    ConnectionProfile selected() const { return m_current; }

signals:
    void connectRequested(const ConnectionProfile& p);

private slots:
    void refreshList();
    void onSelect(const QString& name);
    void saveCurrent();
    void deleteCurrent();
    void newProfile();
    void doConnect();

private:
    void loadToUi(const ConnectionProfile& p);
    void collectFromUi();

    QComboBox* m_list;
    QLineEdit* m_name;
    QLineEdit* m_host;
    QSpinBox* m_port;
    QLineEdit* m_user;
    QLineEdit* m_key;
    QLineEdit* m_jump;
    QLineEdit* m_root;
    QCheckBox* m_trust; // Stage 16: TOFU
    ConnectionProfile m_current;
};
