#pragma once
#include <QDialog>
#include <QStringList>

class QComboBox;
class QListWidget;
class QTextEdit;
class QPushButton;

struct DiffHunk {
    QString header; // @@ ... @@ + bölüm başlığı
    int oldStart = 0, oldCount = 0, newStart = 0, newCount = 0;
    QStringList lines; // ' '/'+'/'-' önekli ("\ No newline" hariç)
    int adds() const;
    int dels() const;
};

struct FileDiff {
    QString oldPath; // a/... (yoksa /dev/null)
    QString newPath; // b/...
    QString relPath; // önekten arındırılmış
    QString indexLine;
    QList<DiffHunk> hunks;
};

// Birleşik diff çözümleyici + hunk seçiminden uygulanabilir yama üretici.
class DiffEngine {
public:
    static QList<FileDiff> parseUnified(const QString& diffText);
    // Tek dosya için seçili hunklardan `git apply` uyumlu yama
    static QString buildPatch(const FileDiff& fd, const QList<int>& pick);
    // Hunk yan yana HTML tablosu
    static QString renderSideBySide(const DiffHunk& h);
    static QString renderUnified(const DiffHunk& h);
};

class DiffDialog : public QDialog {
    Q_OBJECT
public:
    explicit DiffDialog(QWidget* parent = nullptr);
    // Repo modu: git diff çıktısından (stage/discard aktif)
    void setRepoDiff(const QString& repoRoot, const QString& diffText, bool cached);
    // Harici mod: iki dosya arası `git diff --no-index` (salt okunur)
    void setExternalDiff(const QString& diffText, const QString& title);

private slots:
    void refreshPreview();
    void stageHunk();
    void discardHunk();
    void stageAll();

private:
    QString runGit(const QStringList& args, int timeoutMs = 15000);
    void renderFileList();

    QString m_repo;
    QString m_rel;
    bool m_cached = false;
    bool m_readOnly = false;
    QList<FileDiff> m_files;
    QComboBox* m_fileBox;
    QListWidget* m_hunks;
    QTextEdit* m_preview;
    QComboBox* m_mode; // Birleşik | Yan Yana
};
