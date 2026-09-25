#include "MainWindow.h"
#include "core/AboutInfo.h"
#include "core/StartupArgs.h"
#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSettings>
#include <cstdio>

// Tek örnek (single instance): var olan pencereye dosya/komut iletir.
// Test için VERSO_INSTANCE_KEY ile farklı anahtar verilebilir.
static const char* kSingleKey = "verso-coder-single-instance";

// Mitsune Editor döneminden kalan ayarları yeni kimliğe taşı (bir kerelik).
static void migrateLegacySettings() {
    auto migrate = [](const char* oldOrg, const char* oldApp,
                      const char* newOrg, const char* newApp) {
        QSettings n(newOrg, newApp);
        if (!n.allKeys().isEmpty()) return; // yeni tarafta veri var
        QSettings o(oldOrg, oldApp);
        const QStringList keys = o.allKeys();
        if (keys.isEmpty()) return;
        for (const QString& k : keys) n.setValue(k, o.value(k));
    };
    migrate("Mitsune", "MitsuneEditor", "Verso", "VersoCoder");
    migrate("Mitsune", "MitsuneEditorSessions", "Verso", "VersoCoderSessions");
}

static QString instanceKey() {
    // Yeni değişken öncelikli, eski değişken geriye uyumluluk için
    const QString k = qEnvironmentVariable("VERSO_INSTANCE_KEY");
    if (!k.isEmpty()) return k;
    return qEnvironmentVariable("MITSUNE_INSTANCE_KEY", kSingleKey);
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setOrganizationName("Verso");
    app.setApplicationName("VersoCoder");
    app.setApplicationVersion(AboutInfo::version());
    app.setApplicationDisplayName("Verso Coder");
    migrateLegacySettings();

    const StartupOptions opts = StartupArgs::parse(app.arguments().mid(1));
    if (opts.help) {
        std::fputs(StartupArgs::helpText().toUtf8().constData(), stdout);
        return 0;
    }
    if (opts.version) {
        std::printf("%s %s\n", qPrintable(AboutInfo::appName()), qPrintable(AboutInfo::version()));
        return 0;
    }

    // Var olan örnek çalışıyor mu?
    const QString instanceKey = ::instanceKey();
    if (!opts.isNew) {
        QLocalSocket sock;
        sock.connectToServer(instanceKey);
        if (sock.waitForConnected(300)) {
            QStringList payload = opts.paths;
            if (opts.line > 0) payload << QString("--line=%1").arg(opts.line);
            if (!opts.command.isEmpty()) payload << ("--command=" + opts.command);
            sock.write(payload.join('\n').toUtf8());
            sock.flush();
            sock.waitForBytesWritten(500);
            sock.disconnectFromServer();
            return 0;
        }
    }

    QLocalServer::removeServer(instanceKey);
    QLocalServer server;
    server.listen(instanceKey);

    MainWindow w;
    QObject::connect(&server, &QLocalServer::newConnection, &app, [&server, &w]() {
        QLocalSocket* c = server.nextPendingConnection();
        if (!c) return;
        QObject::connect(c, &QLocalSocket::readyRead, c, [c, &w]() {
            const QStringList parts =
                QString::fromUtf8(c->readAll()).split('\n', Qt::SkipEmptyParts);
            w.applyStartupOptions(StartupArgs::parse(parts));
            w.raise();
            w.activateWindow();
            c->disconnectFromServer();
        });
    });

    w.show();
    w.applyStartupOptions(opts);
    return app.exec();
}
