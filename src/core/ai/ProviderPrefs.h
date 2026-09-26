#pragma once
#include "LlmProvider.h"
#include <QString>
#include <QStringList>

// Stage 35: sağlayıcı başına tercihler (aktif sağlayıcı, model, özel base URL).
// QSettings'te tutulur; API anahtarları BURA YAZILMAZ (bkz. SecretStore).
class ProviderPrefs {
public:
    static QString activeProvider();               // "ollama" varsayılan
    static void setActiveProvider(const QString& id);
    static QString modelFor(const QString& id, const QString& fallback = QString());
    static void setModel(const QString& id, const QString& model);
    static QString urlFor(const QString& id);      // özel base URL (boşsa varsayılan)
    static void setUrl(const QString& id, const QString& url);
    // Sağlayıcı + özel URL birleşimi
    static ProviderSpec resolve(const QString& id = QString());
    static QStringList configuredProviders();       // model/URL atanmış sağlayıcılar
    static void reset();
};
