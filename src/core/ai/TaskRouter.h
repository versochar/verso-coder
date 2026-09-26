#pragma once
#include "LlmProvider.h"
#include <QString>
#include <QStringList>

// Stage 36: görev sınıflandırma ve sağlayıcı/model yönlendirme.
enum class TaskClass { Trivial, Simple, Medium, Complex, Agent, Vision, Embed };

class TaskRouter {
public:
    struct Prefs {
        QString forceProvider;  // manuel kilit (boşsa yönlendirme çalışır)
        QString forceModel;
        QString quickProvider;  // basit görevler (soru, açıklama, sınıflandırma)
        QString quickModel;
        QString strongProvider; // karmaşık görevler (refactor, ajan, test)
        QString strongModel;
        QString embedProvider;  // gömme ayrı olabilir
        QString embedModel;
        bool useFreeFirst = true;  // ücretsiz/hızlı modeli tercih et
        bool enabled = true;       // yönlendirme açık mı
    };

    struct Route {
        bool ok = false;
        QString providerId;
        QString model;
        TaskClass cls = TaskClass::Simple;
        QString reason;
        bool forceLocked = false; // kullanıcı sağlayıcıyı elle kilitledi mi
    };

    // Sınıflandırma
    static TaskClass classify(const QString& task);
    static QString label(TaskClass c);
    // Karmaşıklık puanı (0 = önemsiz, 100 = çok karmaşık)
    static int complexityScore(const QString& task);
    static QStringList reasons(const QString& task); // sınıflandırma gerekçeleri

    // Yönlendirme
    static Route route(const QString& task, const Prefs& p);
    static Route route(const QString& task); // kayıtlı tercihlerle

    // Gömme yönlendirmesi (ayrı sağlayıcı olabilir)
    static QString embedProvider(const Prefs& p);
    static QString embedModel(const Prefs& p);
    static QString defaultEmbedModel(const QString& providerId);

    // Tercihler
    static Prefs prefs();
    static void setPrefs(const Prefs& p);
    static Prefs defaultPrefs();
    static void resetPrefs();
    // Ayarlar (QSettings "ai/routing")
    static void fromSettings(Prefs& p);
    static Prefs readSettings();
    static void writeSettings(const Prefs& p);
};
