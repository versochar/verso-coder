#pragma once
#include <QList>
#include <QProcess>
#include <QTextEdit>
#include <QWidget>

class QLineEdit;
class QLabel;
class QTabWidget;
class QComboBox;
class QPushButton;

// Gömülü terminal (bash wrapper) + dile göre Derle & Çalıştır.
// Stage 30: çoklu sekme (oturum başına kabuk) + kabuk profilleri.
class TerminalPanel : public QWidget {
    Q_OBJECT
public:
    explicit TerminalPanel(QWidget* parent = nullptr);
    ~TerminalPanel() override;
    void setWorkdir(const QString& dir);
    void setShell(const QString& prog);
    void runBuildFor(const QString& filePath); // aktif dosya

    // Test edilebilir eşleme: kaynak -> derleme/çalıştırma komutu
    static QString buildCommand(const QString& suffix, const QString& filePath);
    // Stage 16: dış metin (uzak komut çıktısı) — herkese açık
    void appendOut(const QString& t, const QColor& c = QColor());
    void ensureShell();
    // Stage 29: kabuğa metin gönder (eklenti katkısı)
    void sendText(const QString& text);
    // Stage 21: etkin dosya dizinini takip et (takip düğmesi açıksa cd)
    void syncToDir(const QString& dir);
    bool followDir() const;
    // Stage 28: çıkışta bul
    bool findFirst(const QString& text);
    bool findNext(bool forward = true);
    // Stage 30
    void newSession();
    void closeSession(int i);
    int sessionCount() const;
    static QStringList shellProfiles(); // kullanılabilir kabuklar

signals:
    void buildRequested();

private slots:
    void sendInput();
    void readOutput();
    void onFinished(int code);

protected:
    bool eventFilter(QObject* o, QEvent* e) override; // Stage 21: geçmişte Yukarı/Aşağı

private:
    struct Session {
        QProcess* proc = nullptr;
        QTextEdit* out = nullptr;
        QString dir;
        bool starting = false;
    };
    void ensureShell(Session& s);
    Session& cur();
    const Session& cur() const;
    void appendOutTo(Session& s, const QString& t, const QColor& c = QColor());

    void loadHistory();               // Stage 21
    void pushHistory(const QString& c); // Stage 21

    QList<Session> m_terms;
    QTabWidget* m_tabs = nullptr;
    bool m_shellStarting = false;
    QString m_dir;
    QString m_shellProg = "bash";
    QTextEdit* m_out = nullptr; // etkin oturumun görünümü (kolaylık)
    QLineEdit* m_in = nullptr;
    QLabel* m_cwd = nullptr;
    QPushButton* m_follow = nullptr; // Stage 21: dizin takibi
    QComboBox* m_shellBox = nullptr; // Stage 30: kabuk profili
    QStringList m_hist;              // Stage 21: komut geçmişi
    int m_histPos = 0;
    QString m_findText; // Stage 28: bulma metni
};
