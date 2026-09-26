#pragma once
#include "TaskRouter.h"
#include <QString>
#include <QStringList>

// Stage 37: AI görevleri. Her yüzey (sohbet, hayalet tamamlama, arena, tema üretimi,
// belge yorumu, test, commit mesajı, özet, görsel) bu listeden bir görev seçer ve
// aynı ölçülere göre çalışır.
enum class AiTask {
    Chat,      // serbest sohbet
    Explain,   // satır içi düzeltme / açıklama
    Review,    // kod inceleme
    Test,      // test üretimi
    Doc,       // belge yorumu
    Commit,    // commit mesajı
    Summarize, // konuşma özeti
    Fim,       // hayalet tamamlama (FIM/önek)
    Vision,    // görsel analiz
    Theme,     // tema üretimi
    Arena,     // çoklu karşılaştırma
    Extract,   // yapılandırılmış çıktı
};

namespace AiProfiles {

struct Profile {
    double temperature = -1.0; // <0 → kullanıcı ayarı
    int maxTokens = 0;         // 0 → gönderme
    int timeoutMs = 120000;
    TaskClass hint = TaskClass::Simple;
    bool streaming = true;
    bool nativeTools = false;
    QString systemSuffix; // sistem isteminin sonuna eklenir
};

// Saf tablo — ağ yok, test edilebilir
inline Profile profile(AiTask t) {
    Profile p;
    switch (t) {
    case AiTask::Chat:
        p.hint = TaskClass::Simple;
        p.systemSuffix = QString();
        break;
    case AiTask::Explain:
        p.temperature = 0.2;
        p.maxTokens = 2048;
        p.hint = TaskClass::Simple;
        p.systemSuffix = "Kısa ve somut ol; gereksiz tekrar yapma.";
        break;
    case AiTask::Review:
        p.temperature = 0.2;
        p.maxTokens = 3072;
        p.hint = TaskClass::Medium;
        p.systemSuffix = "Maddeli yaz; her bulgu için dosya:satır ver.";
        break;
    case AiTask::Test:
        p.temperature = 0.3;
        p.maxTokens = 3072;
        p.hint = TaskClass::Medium;
        p.systemSuffix = "Çalıştırılabilir test üret; kütüphane uydurma.";
        break;
    case AiTask::Doc:
        p.temperature = 0.2;
        p.maxTokens = 1024;
        p.hint = TaskClass::Simple;
        p.systemSuffix = "Belge yorumunu tanımın hemen üstüne yaz.";
        break;
    case AiTask::Commit:
        p.temperature = 0.4;
        p.maxTokens = 512;
        p.hint = TaskClass::Trivial;
        p.systemSuffix = "Conventional Commits biçiminde tek satır özet + gövde.";
        break;
    case AiTask::Summarize:
        p.temperature = 0.2;
        p.maxTokens = 1024;
        p.timeoutMs = 60000;
        p.hint = TaskClass::Medium;
        p.streaming = false;
        p.systemSuffix = "Karar ve olguları koru; tekrarı at, madde kullan.";
        break;
    case AiTask::Fim:
        p.temperature = 0.15;
        p.maxTokens = 96;
        p.timeoutMs = 4000; // hayalet yanıtı çok hızlı olmalı
        p.hint = TaskClass::Trivial;
        p.streaming = false;
        p.systemSuffix = "Yalnız eksik kodu yaz; açıklama ve işaret yok.";
        break;
    case AiTask::Vision:
        p.temperature = 0.3;
        p.maxTokens = 2048;
        p.hint = TaskClass::Vision;
        p.systemSuffix = "Görselde gördüğünü somutla; tahmin yürütme.";
        break;
    case AiTask::Theme:
        p.temperature = 0.7;
        p.maxTokens = 3072;
        p.timeoutMs = 90000;
        p.hint = TaskClass::Simple;
        p.streaming = false;
        p.systemSuffix = "Yalnız geçerli JSON tema nesnesi döndür.";
        break;
    case AiTask::Arena:
        p.temperature = 0.4;
        p.maxTokens = 2048;
        p.timeoutMs = 90000;
        p.hint = TaskClass::Simple;
        p.streaming = false;
        p.systemSuffix = QString();
        break;
    case AiTask::Extract:
        p.temperature = 0.0;
        p.maxTokens = 1024;
        p.hint = TaskClass::Trivial;
        p.streaming = false;
        p.systemSuffix = "Yalnız istenen biçimde çıktı ver.";
        break;
    }
    return p;
}

inline QString label(AiTask t) {
    switch (t) {
    case AiTask::Chat: return QStringLiteral("sohbet");
    case AiTask::Explain: return QStringLiteral("açıklama");
    case AiTask::Review: return QStringLiteral("inceleme");
    case AiTask::Test: return QStringLiteral("test");
    case AiTask::Doc: return QStringLiteral("belge");
    case AiTask::Commit: return QStringLiteral("commit");
    case AiTask::Summarize: return QStringLiteral("özet");
    case AiTask::Fim: return QStringLiteral("tamamlama");
    case AiTask::Vision: return QStringLiteral("görsel");
    case AiTask::Theme: return QStringLiteral("tema");
    case AiTask::Arena: return QStringLiteral("arena");
    case AiTask::Extract: return QStringLiteral("çıkarım");
    }
    return QStringLiteral("bilinmeyen");
}

// Sistem istemine görev tonunu ekler (user-provided system boşsa default kullanılır)
inline QString withSuffix(const QString& system, AiTask t) {
    const QString suffix = profile(t).systemSuffix;
    if (suffix.isEmpty()) return system;
    if (system.trimmed().isEmpty()) return suffix;
    return system.trimmed() + QStringLiteral("\n\n") + suffix;
}

inline QStringList allLabels() {
    QStringList out;
    for (int i = 0; i <= int(AiTask::Extract); ++i) out << label(AiTask(i));
    return out;
}

} // namespace AiProfiles
