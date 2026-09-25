#pragma once
#include "../core/ConnectionProfile.h"
#include "../core/RemoteFileSystem.h"
#include <QTreeWidget>
#include <QWidget>

class SshSession;

// Stage 16: uzak dosya gezgini — SFTP/ls ile uzak ağaç, aç/kaydet/sil/yenile.
class RemoteExplorer : public QWidget {
    Q_OBJECT
public:
    explicit RemoteExplorer(QWidget* parent = nullptr);

    void setSession(SshSession* s, const ConnectionProfile& p);
    bool hasSession() const { return m_session != nullptr; }
    QString remoteRoot() const { return m_profile.remoteRoot; }

signals:
    void fileOpenRequested(const QString& remotePath); // "ssh://..." değil, ham uzak yol
    void statusMessage(const QString& msg);

public slots:
    void refresh();
    void goUp();
    void newFolder();
    void newFile();
    void removeSelected();
    void uploadHere();
    void downloadSelected();

private slots:
    void onItemActivated(QTreeWidgetItem* it, int col);
    void onContextMenu(const QPoint& pos);

private:
    void listDir(const QString& dir, QTreeWidgetItem* parent);
    QString selectedPath() const;

    QTreeWidget* m_tree;
    SshSession* m_session = nullptr;
    ConnectionProfile m_profile;
    QString m_cur = "/";
};
