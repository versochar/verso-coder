#pragma once
#include "LlmProvider.h"
#include <QList>
#include <QString>
#include <QStringList>

// Stage 37: model havuzu — seçili model hız sınırına takılırsa (429/403/503
// "rate limit", "model busy") aynı sağlayıcıdaki başka bir modele geçilir.
// İki sert kural:
//  1) Fiyat sınıfı korunur: `:free` modelden ücretli modele **sessizce** geçilmez
//     (kullanıcı izni olmadan). "allowPaid" yalnız ayarlardan açılır.
//  2) Amaç korunur: gömme modeli sohbet yedeği olamaz, görsel isteniyorsa
//     görsel destekleyen model aranır.
class ModelPool {
public:
    enum class PriceClass { Free, Paid, Unknown };
    // need: "chat" (boş) | "vision" | "embed"
    enum class Need { Chat, Vision, Embed };

    // Katalog (fetchModels sonucu) — sağlayıcı başına bellekte tutulur
    static void setCatalog(const QString& providerId, const QStringList& models);
    static QStringList catalog(const QString& providerId);
    static bool hasCatalog(const QString& providerId);
    static void clear();
    // Diskten yükle (son oturumun kataloğu; başlangıçta anında aday bulunabilsin)
    static void loadFromDisk();
    static void saveToDisk();

    // Tercih edilen model + sıralı yedekler. İlk aday her zaman `preferred`.
    static QList<QString> candidates(const ProviderSpec& spec, const QString& preferred,
                                     int maxCount = 3, bool allowPaid = false,
                                     Need need = Need::Chat);
    // Bir sonraki yedeğe geç (denenenler listeden çıkarılır)
    static QString nextCandidate(const ProviderSpec& spec, const QString& preferred,
                                 const QStringList& alreadyTried, bool allowPaid = false,
                                 Need need = Need::Chat);

    static PriceClass priceClassOf(const ProviderSpec& spec, const QString& model);
    static bool samePriceClass(const ProviderSpec& spec, const QString& a, const QString& b);
    static bool fitsPurpose(const QString& model, Need need);
    // Sohbet modeli mi? Katalogda tür bilgisi olmadığı için ad sezgisi:
    // görsel/ses/video/gömme/moderasyon aileleri elenir. (Örn. "sd", "flux",
    // "dreamshaper", "whisper", "tts" sohbet yedeği olamaz.)
    static bool isChatCapable(const QString& model);
    // Sohbet olma olasılığı yüksek mi? (aile adı tanınan modeller önceliklenir)
    static int chatScore(const QString& model);
    static bool isKnownModel(const QString& providerId, const QString& model);
    // Katalogda olmayan ama ":free" sonekli model (nokta: katalog eski olabilir)
    static bool plausibleFreeModel(const QString& model);

private:
    static QStringList catalogFor(const QString& providerId);
};
