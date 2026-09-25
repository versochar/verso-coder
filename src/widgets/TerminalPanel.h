#pragma once
#include <QProcess>
#include <QTextEdit>
#include <QWidget>

class QLineEdit;
class QLabel;

// Gömülü terminal (bash wrapper) + dile göre Derle & Çalıştır.
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

signals:
    void buildRequested();

private slots:
    void sendInput();
    void readOutput();
    void onFinished(int code);

private:
    void ensureShell();

    QProcess m_shell;
    bool m_shellStarting = false;
    QString m_dir;
    QString m_shellProg = "bash";
    QTextEdit* m_out;
    QLineEdit* m_in;
    QLabel* m_cwd;
};
