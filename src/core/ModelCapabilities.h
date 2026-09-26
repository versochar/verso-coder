#pragma once
#include "ai/LlmProvider.h"
#include <QJsonObject>
#include <QString>
#include <QStringList>

// Stage 33: model yetenek algılama — /api/show `capabilities` + ad sezgisi.
// Stage 36: ProviderSpec ile birleşir; `reasoning` ve `freeTier` eklendi.
struct ModelCapabilities {
    bool completion = true;
    bool tools = false;
    bool vision = false;
    bool embedding = false;
    bool reasoning = false; // düşünme/analiz kanalı var (thinking, reasoning_content)
    bool freeTier = false;  // :free / yerel / geliştirme kredisi
    QString family;
    int contextLength = 0;
    int contextLimit = 0; // sağlayıcı tavanı (0 = bilinmiyor)

    // /api/show yanıtından (capabilities/model_info/details) çöz.
    static ModelCapabilities fromShow(const QJsonObject& show);
    // Model adından kaba sezgi (sunucu bilgisi yoksa).
    static ModelCapabilities fromName(const QString& model);
    // İkisini birleştir: show verisi varsa o, yoksa ad sezgisi.
    static ModelCapabilities detect(const QString& model, const QJsonObject& show);
    // Stage 36: sağlayıcı yetenekleri + ad sezgisi (ağ YOK, tamamen saf).
    // Sağlayıcı düzeyi kazanır (görü/araç/gömme); model sezgisi üstüne yazılmaz.
    static ModelCapabilities fromSpec(const ProviderSpec& spec, const QString& model);
    // İki kaynağı birleştir: a (daha zayıf) + b (daha güçlü) → b kazanır
    static ModelCapabilities merge(const ModelCapabilities& a, const ModelCapabilities& b);

    bool supportsImageInput() const { return vision; }
    // Görü/araç/gömme/araç çağırma yeteneklerinden herhangi biri var mı?
    bool anyUse() const { return completion || embedding; }
    QStringList badges() const; // görünen kısa etiketler
};
