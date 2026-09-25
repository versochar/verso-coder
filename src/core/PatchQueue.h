#pragma once
#include <QList>
#include <QString>

// Ajanın önerdiği tek bir dosya düzenlemesi.
struct QueuedEdit {
    QString path;      // tam yol
    QString oldText;   // özgün içerik (boş olabilir = yeni dosya)
    QString newText;   // yeni içerik
    bool wholeFile = true;
};

// Çoklu dosya düzenleme kuyruğu + basit LCS tabanlı unified diff üretici.
class PatchQueue {
public:
    void add(const QueuedEdit& e); // aynı yol için son yazım kazanır (old korunur)
    int count() const { return m_edits.size(); }
    bool isEmpty() const { return m_edits.isEmpty(); }
    QList<QueuedEdit> edits() const { return m_edits; }
    QueuedEdit take(int i) const { return m_edits.value(i); }
    void removeAt(int i) { if (i >= 0 && i < m_edits.size()) m_edits.removeAt(i); }
    void clear() { m_edits.clear(); }

    // DiffDialog::parseUnified ile uyumlu unified diff metni üretir.
    static QString unifiedDiff(const QueuedEdit& e, int context = 3);
    static QString allDiffs(const QList<QueuedEdit>& edits);

private:
    QList<QueuedEdit> m_edits;
};
