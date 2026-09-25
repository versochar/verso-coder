#pragma once
#include "../core/LocalHistory.h"
#include <QListWidget>
#include <QWidget>

class QTextEdit;

// Stage 17: zaman çizelgesi — dosyanın anlık görüntüleri + önizleme + geri yükle.
class TimelinePanel : public QWidget {
    Q_OBJECT
public:
    explicit TimelinePanel(const QString& storeDir, QWidget* parent = nullptr);

    void setFile(const QString& filePath, const QString& currentText);

signals:
    void restoreRequested(const QString& filePath, const QString& content);
    void statusMessage(const QString& msg);

private slots:
    void refresh();
    void onSelect();
    void showDiff();
    void doRestore();
    void doClear();

private:
    LocalHistory m_hist;
    QString m_file;
    QString m_current;
    QListWidget* m_list;
    QTextEdit* m_preview;
    QList<HistorySnap> m_snaps;
};
