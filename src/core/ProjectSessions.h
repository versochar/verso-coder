#pragma once
#include <QSettings>
#include <QStringList>

// Proje bazlı oturumlar + adlandırılmış anlık görüntüler.
// QSettings grupları: "proj:<md5(root)>" ve "snap:<ad>".
struct DocSession {
    QString root;
    QStringList files, cursors;
    int active = 0;
    QStringList files2, cursors2;
    int active2 = 0;
    QStringList folds;
};

class ProjectSessions {
public:
    explicit ProjectSessions(QSettings* settings);
    void save(const QString& key, const DocSession& s);
    bool load(const QString& key, DocSession& s) const;
    void remove(const QString& key);
    QStringList snapshots() const; // snap: adları
    QStringList recentRoots() const;
    void touchRoot(const QString& root);

    static QString projectKey(const QString& root);
    static QString snapshotKey(const QString& name);

private:
    QSettings* m_s;
};
