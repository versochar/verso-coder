#include "SecretStore.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QUrl>

static const char* kService = "verso-ai-key";

SecretStore::SecretStore(const QString& filePath) {
    m_useKeyring = keyringAvailable();
    m_file = filePath;
    if (m_file.isEmpty()) {
        const QString dir =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        m_file = dir + "/ai-keys.json";
    }
    // Dosya kopyası HER ZAMAN okunur: anahtar deposu kilitli/erişilemez olsa
    // bile (ya da anahtar dosyaya yazılmışsa) kayıt kaybolmuş gibi görünmez.
    loadFile();
}

void SecretStore::setUseKeyring(bool on) {
    const bool was = m_useKeyring;
    m_useKeyring = on;
    if (was && !on && m_keys.isEmpty()) loadFile(); // dosya kasasına geç
}

bool SecretStore::keyringAvailable() {
    // Test kancası: VERSO_NO_KEYRING doluysa sistem anahtarlığı yok sayılır
    // (geliştirici makinesindeki gerçek anahtarlar testleri kirletmesin)
    if (!QProcessEnvironment::systemEnvironment()
             .value("VERSO_NO_KEYRING")
             .isEmpty())
        return false;
    return !QStandardPaths::findExecutable("secret-tool").isEmpty();
}

QString SecretStore::maskKey(const QString& key) {
    const QString k = key.trimmed();
    if (k.isEmpty()) return {};
    if (k.size() <= 8) return QString(k.size(), '*');
    return k.left(4) + QString(6, QChar(0x2022)) + k.right(3);
}

QString SecretStore::maskProviderId(const QString& id) { return id; }

QString SecretStore::filePathFor(const QString& providerId) const {
    return QFileInfo(m_file).absolutePath() + "/.verso-ai-" + providerId + ".key";
}

bool SecretStore::keyringLookup(const QString& providerId, QString& out) const {
    if (!m_useKeyring) return false;
    QProcess p;
    p.start("secret-tool", {"lookup", "service", kService, "provider", providerId});
    if (!p.waitForFinished(4000)) { p.kill(); return false; }
    if (p.exitCode() != 0) return false;
    const QString v = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    if (v.isEmpty()) return false;
    out = v;
    return true;
}

bool SecretStore::keyringStore(const QString& providerId, const QString& key) {
    if (!m_useKeyring) return false;
    QProcess p;
    p.start("secret-tool",
            {"store", "--label=Verso Coder AI", "service", kService, "provider", providerId});
    if (!p.waitForStarted(3000)) return false;
    p.write(key.toUtf8());
    p.closeWriteChannel();
    if (!p.waitForFinished(4000)) { p.kill(); return false; }
    return p.exitCode() == 0;
}

bool SecretStore::keyringErase(const QString& providerId) {
    if (!m_useKeyring) return false;
    QProcess p;
    p.start("secret-tool", {"clear", "service", kService, "provider", providerId});
    if (!p.waitForFinished(4000)) { p.kill(); return false; }
    return p.exitCode() == 0;
}

QString SecretStore::envKey(const QString& providerId) {
    QProcessEnvironment e = QProcessEnvironment::systemEnvironment();
    const QStringList candidates =
        {QString("VERSO_AI_KEY_%1").arg(providerId.toUpper().replace('-', '_')),
         QString("VERSO_AI_KEY_%1").arg(providerId.toUpper())};
    for (const QString& c : candidates) {
        const QString v = e.value(c);
        if (!v.isEmpty()) return v;
    }
    return {};
}

QString SecretStore::get(const QString& providerId) const {
    if (providerId.isEmpty()) return {};
    QString v;
    if (m_useKeyring && keyringLookup(providerId, v)) return v;
    if (m_keys.contains(providerId)) return m_keys.value(providerId).toString();
    if (!m_useKeyring) return {};
    // Anahtar deposu yanıt vermedi → dosya kopyasına son kez bak
    if (keyringLookup(providerId, v)) return v;
    return {};
}

bool SecretStore::has(const QString& providerId) const { return !get(providerId).isEmpty(); }

QString SecretStore::effectiveKey(const QString& providerId) const {
    const QString k = get(providerId);
    if (!k.isEmpty()) return k;
    return envKey(providerId);
}

bool SecretStore::set(const QString& providerId, const QString& key) {
    if (providerId.isEmpty()) return false;
    const QString v = key.trimmed();
    if (v.isEmpty()) return remove(providerId);
    if (m_useKeyring && keyringStore(providerId, v)) {
        // Tek-yazıcı: depoya yazıldıysa dosyadaki eski kopya silinir
        if (m_keys.contains(providerId)) {
            m_keys.remove(providerId);
            saveFile();
        }
        return true;
    }
    m_useKeyring = false; // kasa dosyaya düşer
    loadFile();
    m_keys[providerId] = v;
    return saveFile();
}

bool SecretStore::remove(const QString& providerId) {
    bool any = false;
    if (m_useKeyring) any = keyringErase(providerId) || any;
    if (m_keys.contains(providerId)) {
        m_keys.remove(providerId);
        saveFile();
        any = true;
    }
    // Anahtar dosyası (yedek yol)
    const QString fp = filePathFor(providerId);
    if (QFile::exists(fp) && QFile::remove(fp)) any = true;
    return any;
}

QStringList SecretStore::providers() const {
    QStringList out = m_keys.keys();
    // Ortam değişkeniyle gelenler
    for (const QString& id : {"openai", "anthropic", "gemini", "nvidia-nim", "unorouter",
                             "groq", "openrouter", "deepseek", "mistral", "xai", "together",
                             "azure-openai"}) {
        if (!envKey(id).isEmpty() && !out.contains(id)) out << id;
    }
    return out;
}

void SecretStore::clearAll() {
    const QStringList ids = providers();
    for (const QString& id : ids) remove(id);
    m_keys = QJsonObject();
    saveFile();
}

bool SecretStore::loadFile() {
    if (m_file.isEmpty()) return false;
    QFile f(m_file);
    if (!f.open(QIODevice::ReadOnly)) return false;
    m_keys = QJsonDocument::fromJson(f.readAll()).object();
    return true;
}

bool SecretStore::saveFile() const {
    if (m_file.isEmpty()) return false;
    QDir().mkpath(QFileInfo(m_file).absolutePath());
    QFile f(m_file);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(m_keys).toJson(QJsonDocument::Indented));
    f.close();
    // 0600: yalnız sahibi okur
    QFile::setPermissions(m_file, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return true;
}

QList<SecretStore::Issue> SecretStore::doctor() const {
    QList<Issue> out;
    if (m_useKeyring && !m_keys.isEmpty()) {
        // Dosyada kayıt var ama depo etkin: hangisi güncel?
        for (auto it = m_keys.constBegin(); it != m_keys.constEnd(); ++it) {
            QString kv;
            const bool inKeyring = keyringLookup(it.key(), kv);
            if (inKeyring && kv != it.value().toString()) {
                out << Issue{it.key(), QStringLiteral("çift kayıt (depo + dosya farklı)"),
                             QStringLiteral("Depodaki ve dosyadaki anahtar farklı. "
                                            "Etkin olan depodaki kullanılır; onarım dosya "
                                            "kopyasını siler."),
                             true};
            } else if (!inKeyring) {
                out << Issue{it.key(), QStringLiteral("sahipsiz dosya kopyası"),
                             QStringLiteral("Depoda yok, dosyada var. Depo kilitliyken "
                                            "yazılmış olabilir; onarım depoya taşır."),
                             true};
            }
        }
    }
    if (!m_useKeyring && keyringAvailable()) {
        out << Issue{QString(), QStringLiteral("depo mevcut ama dosya kullanılıyor"),
                     QStringLiteral("secret-tool kurulu ama kasa dosyaya düşmüş "
                                    "(daha önce depo yazılamamış). Anahtarlar dosyada "
                                    "duruyor; isterseniz ayarladan depoyu yeniden açın."),
                     false};
    }
    return out;
}

int SecretStore::repair() {
    int fixed = 0;
    if (!m_useKeyring) return 0; // dosya kipi tek kaynak, onarılacak şey yok
    loadFile();
    for (const QString& id : m_keys.keys()) {
        const QString fileVal = m_keys.value(id).toString();
        QString kv;
        if (keyringLookup(id, kv) || keyringStore(id, fileVal)) {
            // Depo kazanır (varsa) ya da dosya depoya taşınır;
            // her iki durumda da dosya kopyası silinir.
            m_keys.remove(id);
            ++fixed;
        }
    }
    saveFile();
    return fixed;
}
