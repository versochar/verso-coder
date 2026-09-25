#pragma once
#include <QFutureWatcher>
#include <QStringList>
#include <QWidget>

class QLineEdit;
class QCheckBox;
class QTreeWidget;
class QTextEdit;
class QPushButton;
class QLabel;

struct SearchHit {
    QString file;
    int line = 0;
    int col = 0;
    QString preview;
};

// Proje genelinde arama/değiştir (regex + önizleme). Arama arka planda çalışır.
class SearchPanel : public QWidget {
    Q_OBJECT
public:
    explicit SearchPanel(QWidget* parent = nullptr);
    void setRoot(const QString& root);
    void focusSearch();

    // Test edilebilir çekirdek: sekron arama, GUI'ye dokunmaz.
    static QList<SearchHit> runSearchSync(const QString& root, const QString& query,
                                          const QString& filter, bool useRegex,
                                          bool caseSens, int maxHits = 2000,
                                          const QString& exclude = QString());
    // Dosyadan ±radius satır bağlam (önizleme bölmesi için).
    static QString contextSnippet(const QString& file, int line, int radius = 3);
    void focusExclude(); // Stage 17: hariç kutusuna odaklan

signals:
    void fileOpened(const QString& path, int line);

private slots:
    void runSearch();
    void onSearchDone();
    void replaceAll();
    void replaceFileSelected(); // Stage 17: seçili dosyada önizlemeli uygula

private:
    static QList<SearchHit> searchInFile(const QString& file, const QString& query,
                                         bool useRegex, bool caseSens);
    void applyReplace(const QString& file, const QList<SearchHit>& hits);
    void previewReplaceFor(const QString& file); // Stage 17: dosya önizlemesi

    QString m_root;
    QLineEdit* m_query;
    QLineEdit* m_replace;
    QLineEdit* m_filter;
    QLineEdit* m_exclude; // Stage 17: hariç glob
    QCheckBox* m_regex;
    QCheckBox* m_caseSens;
    QTreeWidget* m_results;
    QTextEdit* m_preview;
    QPushButton* m_btnSearch;
    QPushButton* m_btnReplaceAll;
    QPushButton* m_btnReplaceFile; // Stage 17
    QLabel* m_status;
    QList<SearchHit> m_hits;
    QFutureWatcher<QList<SearchHit>> m_watcher;
};
