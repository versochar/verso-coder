#pragma once
#include <QDialog>
#include <QJsonObject>
#include <QString>
#include <QStringList>

class QLineEdit;
class QPlainTextEdit;
class QVBoxLayout;
class QLabel;

// Stage 33: çok-modelli arena — aynı istemi N modele koştur, yan yana karşılaştır.
class ModelArenaDialog : public QDialog {
    Q_OBJECT
public:
    ModelArenaDialog(const QString& host, const QStringList& models,
                     const QString& systemPrompt, const QString& prompt,
                     const QJsonObject& options, QWidget* parent = nullptr);

private slots:
    void run();

private:
    struct Result {
        QString model;
        QString text;
        QString error;
        qint64 ms = 0;
        int promptTokens = 0;
        int evalTokens = 0;
    };

    QString m_host;
    QString m_system;
    QJsonObject m_options;
    QLineEdit* m_models = nullptr;
    QPlainTextEdit* m_prompt = nullptr;
    QVBoxLayout* m_results = nullptr;
    QLabel* m_status = nullptr;
};
