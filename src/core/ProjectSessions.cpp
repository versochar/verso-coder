#include "ProjectSessions.h"
#include <QCryptographicHash>

ProjectSessions::ProjectSessions(QSettings* settings) : m_s(settings) {}

QString ProjectSessions::projectKey(const QString& root) {
    return "proj:" + QString::fromUtf8(
        QCryptographicHash::hash(root.toUtf8(), QCryptographicHash::Md5).toHex());
}

QString ProjectSessions::snapshotKey(const QString& name) {
    return "snap:" + name;
}

void ProjectSessions::save(const QString& key, const DocSession& s) {
    m_s->beginGroup(key);
    m_s->setValue("root", s.root);
    m_s->setValue("files", s.files);
    m_s->setValue("cursors", s.cursors);
    m_s->setValue("active", s.active);
    m_s->setValue("files2", s.files2);
    m_s->setValue("cursors2", s.cursors2);
    m_s->setValue("active2", s.active2);
    m_s->setValue("folds", s.folds);
    m_s->endGroup();
    m_s->sync();
}

bool ProjectSessions::load(const QString& key, DocSession& s) const {
    m_s->beginGroup(key);
    bool has = m_s->contains("files") || m_s->contains("files2");
    s.root = m_s->value("root").toString();
    s.files = m_s->value("files").toStringList();
    s.cursors = m_s->value("cursors").toStringList();
    s.active = m_s->value("active", 0).toInt();
    s.files2 = m_s->value("files2").toStringList();
    s.cursors2 = m_s->value("cursors2").toStringList();
    s.active2 = m_s->value("active2", 0).toInt();
    s.folds = m_s->value("folds").toStringList();
    m_s->endGroup();
    return has;
}

void ProjectSessions::remove(const QString& key) {
    m_s->remove(key);
}

QStringList ProjectSessions::snapshots() const {
    QStringList out;
    for (const QString& g : m_s->childGroups())
        if (g.startsWith("snap:")) out << g.mid(5);
    return out;
}

QStringList ProjectSessions::recentRoots() const {
    QStringList out;
    for (const QString& g : m_s->childGroups()) {
        if (!g.startsWith("proj:")) continue;
        m_s->beginGroup(g);
        QString r = m_s->value("root").toString();
        m_s->endGroup();
        if (!r.isEmpty()) out << r;
    }
    return out;
}

void ProjectSessions::touchRoot(const QString& root) {
    QStringList roots = recentRoots();
    roots.removeAll(root);
    roots.prepend(root);
    while (roots.size() > 10) roots.takeLast();
    // Sıralı liste olarak sakla
    m_s->beginGroup("meta");
    m_s->setValue("recentRoots", roots);
    m_s->endGroup();
}
