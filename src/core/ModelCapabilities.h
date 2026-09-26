#pragma once
#include <QJsonObject>
#include <QString>
#include <QStringList>

// Stage 33: model yetenek algılama — /api/show `capabilities` + ad sezgisi.
struct ModelCapabilities {
    bool completion = true;
    bool tools = false;
    bool vision = false;
    bool embedding = false;
    QString family;
    int contextLength = 0;

    // /api/show yanıtından (capabilities/model_info/details) çöz.
    static ModelCapabilities fromShow(const QJsonObject& show);
    // Model adından kaba sezgi (sunucu bilgisi yoksa).
    static ModelCapabilities fromName(const QString& model);
    // İkisini birleştir: show verisi varsa o, yoksa ad sezgisi.
    static ModelCapabilities detect(const QString& model, const QJsonObject& show);

    bool supportsImageInput() const { return vision; }
    QStringList badges() const; // görünen kısa etiketler
};
