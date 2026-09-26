#pragma once
#include <QDialog>
#include <QList>
#include <QMap>

#include "../widgets/SearchPanel.h"

// Stage 30: arama düzenleyicisi — sonuçlar kalıcı listede; dosya bazında
// işaretle, toplu değiştir (önizlemeli).
class SearchEditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit SearchEditorDialog(const QString& root, const QList<SearchHit>& hits,
                                const QString& query, const QString& replacement,
                                bool useRegex, bool caseSens, QWidget* parent = nullptr);

signals:
    void jumpRequested(const QString& path, int line);

private slots:
    void applyChecked();

private:
    QString m_root;
    QList<SearchHit> m_hits;
    QString m_query;
    QString m_replace;
    bool m_useRegex = false;
    bool m_caseSens = false;
    class QTreeWidget* m_tree = nullptr;
};
