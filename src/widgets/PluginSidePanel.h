#pragma once
#include <QMap>
#include <QString>
#include <QWidget>

class PluginEngine;
class QLineEdit;
class QListWidget;
class QListWidgetItem;

// VS Code "Extensions" görünümü: kurulu eklentiler (aç/kapa + arama),
// eklenti görünümleri, Mağaza/Yönetici/Klasör kısayolları.
class PluginSidePanel : public QWidget {
    Q_OBJECT
public:
    explicit PluginSidePanel(QWidget* parent = nullptr);
    void setEngine(PluginEngine* eng) { m_eng = eng; }
    void setViews(const QMap<QString, QString>& views) { m_views = views; }
    void refresh(); // motor + arama durumundan listeyi kurar

signals:
    void manageRequested();
    void storeRequested();
    void folderRequested();
    void viewRequested(const QString& cmdId);
    void pluginsChanged(); // aç/kapa sonrası (yeniden yükleme yapıldı)

private slots:
    void onItemChanged(QListWidgetItem* it);
    void onViewActivated(QListWidgetItem* it);

private:
    PluginEngine* m_eng = nullptr;
    QMap<QString, QString> m_views; // cmdId → başlık
    QLineEdit* m_search = nullptr;
    QListWidget* m_list = nullptr;
    QListWidget* m_viewList = nullptr;
    bool m_guard = false;
};
