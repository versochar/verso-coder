#pragma once
#include "ai/LlmProvider.h"
#include <QString>
#include <QStringList>

// Stage 38: "neden çalışmıyor?" rehberi. Sağlık paneli artık yalnız veri
// göstermek yerine yapılacak işleri listeler: eksik anahtar, kapalı sunucu,
// kota dolmuş, ağ yok, model meşgul. Her eylem tek cümle, Türkçe, eylemilebilir.
struct SetupAction {
    enum Severity { Info, Warning, Critical };
    QString title;      // "NVIDIA NIM için anahtar gerekli"
    QString detail;
    QString action;     // "build.nvidia.com → API Keys"
    QString commandId;  // varsa ilgili komut (örn. "view.settings")
    Severity severity = Warning;
    QString providerId; // boşsa genel
};

class SetupAdvisor {
public:
    // Mevcut durumdan eylem listesi üret (ağ YOK; saf)
    static QList<SetupAction> analyze(bool networkUp = true);
    // Tek bir sağlayıcı için
    static QList<SetupAction> analyzeProvider(const QString& providerId, bool networkUp = true);
    // Tek cümlelik özet
    static QString summary(const QList<SetupAction>& actions);
    // En kritik eylem
    static SetupAction mostCritical(const QList<SetupAction>& actions);
    // Sağlayıcı için anahtar alma yeri (kullanıcıya gösterilecek metin)
    static QString keyHintFor(const QString& providerId);
    static bool hasKey(const QString& providerId);
    // --- Stage 39: canlı ağ yoklaması ---
    // generate_204 uç noktasına kısa zaman aşımlı yoklama. Ağ yoksa bile
    // 3 sn içinde döner. Gizlilik: istek gövdesi yok, yalnız durum kodu okunur.
    static bool networkUp(int timeoutMs = 3000);
    // Ağ durumunu ölçüp ona göre analiz eder (ayarlardan açılabilir)
    static QList<SetupAction> analyzeWithNetworkCheck(int timeoutMs = 3000);
};
