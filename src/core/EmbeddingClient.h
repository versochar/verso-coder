#pragma once
#include <QJsonObject>
#include <QList>
#include <QNetworkAccessManager>
#include <QString>
#include <QStringList>

// Stage 33: Ollama gömme (embedding) istemcisi — /api/embed.
class EmbeddingClient {
public:
    explicit EmbeddingClient(QObject* parent = nullptr);
    void setHost(const QString& host);

    // Senkron gömme (arka plan iş parçacığında çağrılmalı). Hata doldurulur.
    QList<QList<float>> embedSync(const QString& model, const QStringList& inputs,
                                  QString& error, int timeoutMs = 60000);

    // Test edilebilir yardımcılar
    static QByteArray payload(const QString& model, const QStringList& inputs);
    static QList<QList<float>> parseResponse(const QJsonObject& o);

private:
    QNetworkAccessManager m_net;
    QString m_host = "http://localhost:11434";
};
