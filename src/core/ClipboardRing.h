#pragma once
#include <QString>
#include <QStringList>

// Stage 21: pano halkası — kopyalanan son N metin; Ctrl+Shift+V ile döner.
// Saf veri yapısı (test edilebilir); QClipboard bağlantısı MainWindow'da.
class ClipboardRing {
public:
    explicit ClipboardRing(int cap = 20) : m_cap(qMax(1, cap)) {}

    void push(const QString& text) {
        const QString t = text;
        if (t.isEmpty()) return;
        m_items.removeAll(t);
        m_items.prepend(t);
        while (m_items.size() > m_cap) m_items.removeLast();
        m_pos = 0;
    }
    // Bir sonraki girdiyi döndürür (halka turu); boşsa "".
    QString next() {
        if (m_items.isEmpty()) return {};
        m_pos = (m_pos + 1) % m_items.size();
        return m_items[m_pos];
    }
    QString current() const {
        return m_items.isEmpty() ? QString() : m_items[m_pos % m_items.size()];
    }
    int size() const { return m_items.size(); }
    void clear() { m_items.clear(); m_pos = 0; }
    // Stage 30: tüm girdiler (önce sabitler)
    QStringList items() const { return m_pinned + m_items; }
    // Stage 30: sabitleme (sabitler turun dışında kalır, en üstte listelenir)
    void pin(const QString& text) {
        if (text.isEmpty() || m_pinned.contains(text)) return;
        m_pinned.prepend(text);
        m_items.removeAll(text);
    }
    void unpin(const QString& text) { m_pinned.removeAll(text); }
    QStringList pinned() const { return m_pinned; }

private:
    QStringList m_items;
    QStringList m_pinned;
    int m_pos = 0;
    int m_cap = 20;
};
