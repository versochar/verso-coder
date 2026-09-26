#pragma once
#include <QMap>
#include <QUrl>
#include <algorithm>
#include <QSettings>
#include <QString>
#include <QStringList>

// Stage 28: yer imleri — dosya → satırlar (1-based, sıralı, tekil).
// QSettings kalıcı. Saf mantık (test edilebilir).
class BookmarkStore {
public:
    void toggle(const QString& file, int line) {
        if (file.isEmpty() || line <= 0) return;
        QList<int> ls = lines(file);
        if (ls.contains(line)) ls.removeAll(line);
        else {
            ls << line;
            std::sort(ls.begin(), ls.end());
        }
        if (ls.isEmpty()) m_marks.remove(file);
        else m_marks[file] = ls;
    }
    QList<int> lines(const QString& file) const { return m_marks.value(file); }
    bool has(const QString& file, int line) const {
        return m_marks.value(file).contains(line);
    }
    void remove(const QString& file, int line) {
        QList<int> ls = lines(file);
        ls.removeAll(line);
        if (ls.isEmpty()) m_marks.remove(file);
        else m_marks[file] = ls;
    }
    void clearFile(const QString& file) { m_marks.remove(file); }
    QStringList files() const { return m_marks.keys(); }
    // Sonraki/önceki işaret (yoksa -1)
    int next(const QString& file, int line) const {
        for (int l : lines(file))
            if (l > line) return l;
        return -1;
    }
    int prev(const QString& file, int line) const {
        int best = -1;
        for (int l : lines(file))
            if (l < line) best = l;
        return best;
    }
    static QString encKey(const QString& file) {
        return QString::fromUtf8(QUrl::toPercentEncoding(file, ""));
    }
    static QString decKey(const QString& key) {
        return QUrl::fromPercentEncoding(key.toUtf8());
    }
    void load() {
        m_marks.clear();
        QSettings q("Verso", "VersoCoder");
        q.beginGroup("bookmarks");
        for (const QString& k : q.childKeys()) {
            QList<int> ls;
            for (const QString& n : q.value(k).toStringList()) {
                const int l = n.toInt();
                if (l > 0) ls << l;
            }
            std::sort(ls.begin(), ls.end());
            const QString f = decKey(k);
            if (!f.isEmpty() && !ls.isEmpty()) m_marks[f] = ls;
        }
        q.endGroup();
    }
    void save() const {
        QSettings q("Verso", "VersoCoder");
        q.beginGroup("bookmarks");
        for (const QString& k : q.childKeys()) q.remove(k);
        for (auto it = m_marks.constBegin(); it != m_marks.constEnd(); ++it) {
            QStringList ns;
            for (int l : it.value()) ns << QString::number(l);
            q.setValue(encKey(it.key()), ns);
        }
        q.endGroup();
        q.sync();
    }

private:
    QMap<QString, QList<int>> m_marks;
};
