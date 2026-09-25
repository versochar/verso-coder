#pragma once
#include <QString>
#include <QObject>

class GitVersionManager : public QObject {
    Q_OBJECT
public:
    explicit GitVersionManager(QObject* parent = nullptr);
    ~GitVersionManager() override = default;

    // Git reposunun kök dizinini ayarla
    void setRepoRoot(const QString& root) { m_repoRoot = root; }

    // Reponun mevcut sürüm/commit bilgisini al
    QString currentVersion();

    // Yeni bir sürüm/tag oluştur (opt-in)
    bool createTag(const QString& tagName, const QString& message = QString());

    // Son commit mesajını al
    QString lastCommitMessage();

    // Repository'yu temizle (working directory değişiklikleri iptal)
    bool resetHard();

signals:
    void versionChanged(const QString& version);
    void errorOccurred(const QString& message);

private:
    QString m_repoRoot;
};
