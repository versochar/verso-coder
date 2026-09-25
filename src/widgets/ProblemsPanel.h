#pragma once
#include "../core/LspClient.h"
#include <QMap>
#include <QWidget>

class QListWidget;
class QStackedWidget;
class EmptyState;

// LSP diagnostic listesi: dosyaya göre gruplu, çift tıkla satıra git.
class ProblemsPanel : public QWidget {
    Q_OBJECT
public:
    explicit ProblemsPanel(QWidget* parent = nullptr);
    void setDiagnostics(const QMap<QString, QList<LspDiag>>& all);
    int count() const;

signals:
    void fileOpened(const QString& path, int line);
    void fixWithAiRequested(const QString& path, int line, const QString& message); // Stage 7

private:
    void fixSelected();
    QListWidget* m_list;
    QStackedWidget* m_stack = nullptr; // Stage 10: liste / boş durum
    EmptyState* m_empty = nullptr;
};
