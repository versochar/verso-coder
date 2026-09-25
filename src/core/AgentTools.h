#pragma once
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

    // kök dışına çıkan yolları reddet
    bool isInsideRoot(const QString& absPath) const;
    QString absoluteInRoot(const QString& rel) const;

private:
    bool approve(const ToolCall& c);

    QString m_root;
    Approver m_approver;
    ProblemsProvider m_problems;
    WriteMode m_writeMode = Direct;
    PatchQueue* m_queue = nullptr;
    bool m_allowCommand = false;
};
