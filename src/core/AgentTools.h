#pragma once
#include "AgentPolicy.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>
#include <functional>

class PatchQueue;

// Modelin istediği tek bir araç çağrısı.
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
    QString absoluteInRoot(const QString& rel) const;

private:
    bool approve(const ToolCall& c);
    ToolResult runShell(const QString& command, int timeoutMs, const QString& label,
                        bool needsApproval);

    QString m_root;
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
