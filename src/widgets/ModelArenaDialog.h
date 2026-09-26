#pragma once
#include <QDialog>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QVBoxLayout;
class QLabel;

struct ArenaTarget {
    QString providerId;
    QString model;
    QString label() const;
};

// Stage 33: çok-modelli arena — aynı istemi N modele koştur, yan yana karşılaştır.
// Stage 37: hedef artık (sağlayıcı, model) çifti; yerel modellerle bulut
// sağlayıcıları yan yana koşar, gecikme/jeton/gerçek maliyet gösterilir.
class ModelArenaDialog : public QDialog {
    Q_OBJECT
public:
    ModelArenaDialog(const QList<ArenaTarget>& targets, const QString& systemPrompt,
                     const QString& prompt, const QJsonObject& options,
                     QWidget* parent = nullptr);

    // Yerel modeller + anahtarı olan yapılandırılmış sağlayıcılardan örnek havuz
    static QList<ArenaTarget> defaultTargets(const QStringList& localModels,
                                             const QString& activeProvider, int maxCount = 6);

signals:
    // Kazanan yanıtı sohbete al
    void adoptRequested(const QString& text);

private slots:
    void run();
    void adopt();

private:
    struct Result {
        QString providerId;
        QString model;
        QString text;
        QString error;
        qint64 ms = 0;
        int promptTokens = 0;
        int evalTokens = 0;
        double usd = 0.0;
        int score = 0;
    };

    // Ağ çağrısı (çalışan iş parçacığında) — AiRunner üzerinden
    static Result callTarget(const ArenaTarget& t, const QString& system, const QString& prompt);

    QString m_system;
    QJsonObject m_options;
    QList<ArenaTarget> m_targets;
    QLineEdit* m_models = nullptr;
    QPlainTextEdit* m_prompt = nullptr;
    QVBoxLayout* m_results = nullptr;
    QLabel* m_status = nullptr;
    QPushButton* bAdopt = nullptr;
    QList<Result> m_last;
};
