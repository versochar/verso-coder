#pragma once
#include <QWidget>

class QLabel;
class QVBoxLayout;

// Stage 10: yeniden tasarlanmış karşılama ekranı — tüm sekmeler kapandığında
// editör alanında gösterilir: başlık, hızlı eylemler, son dosyalar, kısayol ipuçları.
class WelcomeView : public QWidget {
    Q_OBJECT
public:
    explicit WelcomeView(QWidget* parent = nullptr);

    void setRecentFiles(const QStringList& files);
    int recentCount() const { return m_recent.size(); }
    // Stage 28: son projeler
    void setRecentProjects(const QStringList& dirs);

signals:
    void commandRequested(const QString& cmdId);
    void fileRequested(const QString& path);
    void projectRequested(const QString& dir); // Stage 28

protected:
    // Stage 20: hızlı eylem butonlarında hover renk sabitleme
    bool eventFilter(QObject* o, QEvent* e) override;

private:
    QWidget* buildQuickActions();
    QWidget* buildRecents();
    QWidget* buildProjects(); // Stage 28: son projeler kartı
    QWidget* buildShortcuts();
    QWidget* buildDemoMode(); // Stage 19: canlı demo önizleme kartı

    QStringList m_recent;
    QVBoxLayout* m_recentsHost = nullptr;
    QLabel* m_recentsTitle = nullptr;
    QStringList m_projects; // Stage 28
    QVBoxLayout* m_projectsHost = nullptr;
};
