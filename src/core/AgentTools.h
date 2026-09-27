#pragma once
#include "AgentPolicy.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>
#include <functional>

class PatchQueue;

// Modelin istediği tek bir araç çağrısı.
#include "CommandAudit.h"
#include "PathGuard.h"
#include <QProcessEnvironment>

struct ToolCall {
    QString name;
    QJsonObject args;
    QString raw; // modele gösterilen özgün metin
};

struct ToolResult {
    bool ok = false;
    QString output;
    bool denied = false; // onay reddedildi
};

// Ajan araç kayıt defteri + güvenli yürütücü. Tüm dosya erişimi proje köküne
// hapsedilir (sandbox); yazma/komut araçları onay gerektirir.
class AgentTools {
public:
    enum WriteMode { Direct, Queue };
    using Approver = std::function<bool(const ToolCall&)>;
    using ProblemsProvider = std::function<QString()>;

    explicit AgentTools(const QString& root = QString());

    void setRoot(const QString& root) { m_root = root; }
    QString root() const { return m_root; }
    void setApprover(Approver a) { m_approver = std::move(a); }
    void setProblemsProvider(ProblemsProvider p) { m_problems = std::move(p); }
    void setWriteMode(WriteMode m) { m_writeMode = m; }
    void setPatchQueue(PatchQueue* q) { m_queue = q; }
    void setAllowCommand(bool on) { m_allowCommand = on; }
    bool allowCommand() const { return m_allowCommand; }
    // Stage 34: politika kapısı (araç izni + otonom kısıtı)
    void setPolicy(const AgentPolicy& p) { m_policy = p; }
    const AgentPolicy& policy() const { return m_policy; }
    // Stage 34: test komutu (otomatik algılanır, onay + politika gerekir)
    void setTestCommand(const QString& cmd) { m_testCommand = cmd; }
    QString testCommand() const { return m_testCommand; }
    // Git deposu mu? (git_* araçlarında kullanılır)
    void setGitRepo(bool on) { m_gitRepo = on; }
    bool gitRepo() const { return m_gitRepo; }

    // Model yanıtından araç çağrılarını ayrıştır:
    //   <tool_call>{"name":"read_file","arguments":{"path":"..."}}</tool_call>
    //   <tool name="list_dir">{}</tool>            (gövde opsiyonel)
    //   ```tool\n{"name":...}\n```                (nesne veya dizi)
    static QList<ToolCall> parseCalls(const QString& assistantText);

    // Sistem promptuna eklenecek araç tanımı/metin.
    QString systemPromptAddendum() const;
    QJsonArray toolSchemas() const;

    ToolResult execute(const ToolCall& call);

    // Tekil araçlar (doğrudan test edilebilir)
    ToolResult readFile(const QString& path, int startLine = 0, int count = -1);
    ToolResult writeFile(const QString& path, const QString& content);
    ToolResult listDir(const QString& path);
    ToolResult search(const QString& pattern, const QString& glob = QString(), int maxHits = 80);
    ToolResult runCommand(const QString& command, int timeoutMs = 20000);
    ToolResult getProblems();

    // --- Stage 34: yeni araçlar (salt-okunur ya da onay + politika kapılı) ---
    ToolResult readRange(const QString& path, int startLine, int endLine);
    ToolResult grepLines(const QString& pattern, const QString& glob = QString(), int context = 0,
                         int maxHits = 60);
    ToolResult findSymbol(const QString& name, int maxHits = 40);
    ToolResult gitStatus();
    ToolResult gitDiff(bool staged = false, int maxChars = 12000);
    ToolResult runTests(const QString& command = QString(), int timeoutMs = 120000);
    ToolResult healthScan();

    // Tehlikeli kabuk kalıpları (Stage 34 güvenlik): komut reddedilir.
    static bool isCommandBlocked(const QString& command, QString* why = nullptr);
    static QStringList blockedPatterns();
    // Test komutunu köke göre otomatik tahmin et.
    static QString guessTestCommand(const QString& root);

    // kök dışına çıkan yolları reddet
    bool isInsideRoot(const QString& absPath) const;
    // Stage 38: sembolik bağlantı çözen kanonik denetim. isInsideRoot
    // geriye uyum için korunur; dosya araçları PathGuard üzerinden geçer.
    const PathGuard& guard() const { return m_guard; }
    // Yol denetimi: başarılıysa doğrulanmış mutlak yolu yazar.
    bool safePath(const QString& input, QString& outAbs, QString& why) const;

    // Stage 38: kabuk kipi. Varsayılan "bash -c" (profil YÜKLENMEZ: kullanıcının
    // .bashrc'sindeki alias/fonksiyon komutu sessizce değiştirebilirdi).
    // "legacy" = eski davranış (bash -lc), ayarlardan seçilebilir.
    enum class ShellMode { Secure, Legacy };
    void setShellMode(ShellMode m) { m_shellMode = m; }
    void setReadOnlyWrap(bool on) { m_readOnlyWrap = on; }
    // Stage 39: kuyruk eşiği. Kuyruktaki dosya sayısı eşiği aşarsa tek "evet"
    // ile geçmemeli; gözden geçirme diyaloğunda tek tek işaretlenmeli.
    void setQueueReviewThreshold(int n) { m_queueThreshold = qMax(1, n); }
    int queueReviewThreshold() const { return m_queueThreshold; }
    bool queueNeedsReview() const;
    ShellMode shellMode() const { return m_shellMode; }
    // Ortam değişkeni temizliği: anahtar/token/AI sırlarını komuta taşıma
    static QProcessEnvironment sanitizedEnvironment();
    // Salt-okunur kip kullanılabilir mi? (bwrap/unshare)
    static QString readOnlyWrapper();
    static bool readOnlyAvailable();
    // Komut denetimi
    CommandAudit& audit() { return m_audit; }
    void setAuditEnabled(bool on) { m_auditOn = on; }
    // Onay diyaloğunda gösterilecek yol uyarısı (mutlak yol içeren komutlar)
    static QString pathWarning(const QString& command);
    QString absoluteInRoot(const QString& rel) const;

private:
    bool approve(const ToolCall& c);
    ToolResult runShell(const QString& command, int timeoutMs, const QString& label,
                        bool needsApproval);

    QString m_root;
    PathGuard m_guard; // Stage 38: yol güvenliği
    ShellMode m_shellMode = ShellMode::Secure; // Stage 38
    int m_queueThreshold = 5; // Stage 39: toplu onay eşiği
    bool m_auditOn = true;
    CommandAudit m_audit; // Stage 38: komut denetimi
    bool m_readOnlyWrap = false; // varsa salt-okunur kip kullan
    Approver m_approver;
    ProblemsProvider m_problems;
    WriteMode m_writeMode = Direct;
    PatchQueue* m_queue = nullptr;
    bool m_allowCommand = false;
    // Stage 34
    AgentPolicy m_policy;
    QString m_testCommand;
    bool m_gitRepo = false;
};
