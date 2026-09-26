#pragma once
#include <QDialog>

class QLineEdit;
class QTextEdit;

// Stage 28: hex görüntüleyici (salt-okunur + adrese git).
class HexViewDialog : public QDialog {
    Q_OBJECT
public:
    explicit HexViewDialog(const QString& path, QWidget* parent = nullptr);

private slots:
    void goToAddress();

private:
    void render(qint64 from, qint64 lines = 256);
    QByteArray m_data;
    QTextEdit* m_view = nullptr;
    QLineEdit* m_addr = nullptr;
};
