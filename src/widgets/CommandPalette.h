#pragma once
#include <QDialog>
#include <QStringList>

class QLabel;
class QLineEdit;
class QListWidget;

struct PaletteCommand {
    QString id;
    QString title;
    QString hint;
    QString keys;
};

// Ctrl+Shift+P komut paleti: fuzzy filtreli eylem listesi.
// Stage 10: son kullanılan komutlar öne gelir (★), sonuç sayacı gösterilir.
class CommandPalette : public QDialog {
    Q_OBJECT
public:
    explicit CommandPalette(QWidget* parent = nullptr);
    void setCommands(const QList<PaletteCommand>& cmds);
    QString selectedId() const { return m_selected; }
    static int matchScore(const QString& pattern, const QString& text);

    // Son kullanılan komutlar (QSettings'ta kalıcı, en fazla 8)
    static QStringList recents();
    static void pushRecent(const QString& id);

private slots:
    void refilter();

private:
    QLineEdit* m_search;
    QListWidget* m_list;
    QLabel* m_count;
    QList<PaletteCommand> m_cmds;
    QStringList m_recents;
    QString m_selected;
};
