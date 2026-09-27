#pragma once
#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

// Stage 38: ajan komut denetimi — onaylanan/deneyen her kabuk komutu
// kalıcı bir günlüğe yazılır. "Kota/kural denetimi" listeye dayalı olduğu için
// veri sızdıran ama tehlikeli görünmeyen komutları yakalayamaz; denetim
// kaydı bu boşluğu kapatır (kullanıcı sonra görebilir).
struct CommandAuditEntry {
    QDateTime at;
    QString tool;      // run_command | run_tests | …
    QString command;
    bool approved = false; // kullanıcı onayı alındı mı
    bool denied = false;   // reddedildi mi (kural / kullanıcı)
    int exitCode = 0;
    qint64 durationMs = 0;
    QString note;      // engelleme nedeni

    QJsonObject toJson() const;
    static CommandAuditEntry fromJson(const QJsonObject& o);
    // "12:03:14  ✓ run_command  cmake --build build  (2,1 sn)"
    QString line() const;
};

class CommandAudit {
public:
    explicit CommandAudit(const QString& file = QString());

    QString filePath() const { return m_file; }
    bool load();
    bool save() const;
    void clear();

    void record(const CommandAuditEntry& e);
    void record(const QString& tool, const QString& command, bool approved, bool denied,
                int exitCode = 0, qint64 durationMs = 0, const QString& note = QString());

    QList<CommandAuditEntry> entries() const { return m_entries; }
    int count() const { return int(m_entries.size()); }
    int maxEntries() const { return m_max; }
    void setMaxEntries(int n);
    // Son N kayıt (yeni en başta)
    QList<CommandAuditEntry> last(int n) const;
    // Bir araca/komuta göre arama
    QList<CommandAuditEntry> search(const QString& needle) const;
    // Özet: toplam/deneyen/onaylanan süre
    struct Summary {
        int total = 0;
        int denied = 0;
        qint64 totalMs = 0;
    };
    Summary summary() const;
    // Denetim günlüğü dosyası tek başına okunabilir metin olarak dışa aktarılır
    static QString toText(const QList<CommandAuditEntry>& list);

private:
    QString m_file;
    int m_max = 200;
    QList<CommandAuditEntry> m_entries;
};
