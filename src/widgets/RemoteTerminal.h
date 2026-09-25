#pragma once
#include "../core/ConnectionProfile.h"
#include <QProcess>
#include <QWidget>

class QPlainTextEdit;
class QLineEdit;

// Stage 16: uzak terminal — `ssh -t hedef` etkileşimli kabuğu gömülü konsol.
class RemoteTerminal : public QWidget {
    Q_OBJECT
public:
    explicit RemoteTerminal(QWidget* parent = nullptr);
    ~RemoteTerminal() override;

    bool start(const ConnectionProfile& p);
    void stop();
    bool isRunning() const;

signals:
    void statusMessage(const QString& msg);

private slots:
    void readOutput();
    void sendInput();
    void onFinished(int code);

private:
    void appendOut(const QString& t);

    QPlainTextEdit* m_out;
    QLineEdit* m_in;
    QProcess m_proc;
};
