#pragma once
#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <QProcess>
#include <functional>

struct LspDiag {
    QString path;
    int line = 0;    // 0-based
    int col = 0;
    int endLine = 0;
    int endCol = 0;
    int severity = 1; // 1=Error 2=Warning 3=Info 4=Hint
    QString message;
    QString source;
};

// LSP istemcisi (JSON-RPC, stdio): clangd / pylsp ile konuşur.
// Stage 4: initialize → didOpen/didChange → hover/definition + publishDiagnostics.
// Stage 13: genel request()/notify() + completion/references/rename/codeAction/
// signature/symbol/format/inlay/semantic/callHierarchy + sunucu yetenekleri.
class LspClient : public QObject {
    Q_OBJECT
public:
    explicit LspClient(QObject* parent = nullptr);
    ~LspClient() override;

    bool start(const QString& program, const QStringList& args, const QString& workdir);
    // Stage 16: uzak LSP — `ssh hedef -- uzak-komut` stdio köprüsü
    bool startRemote(const QString& remoteLsp, const QStringList& remoteArgs,
                     const QString& sshTarget, const QStringList& sshBaseArgs,
                     const QString& remoteRoot);
    void stop();
    bool isRunning() const;
    bool isReady() const { return m_ready; }

    void didOpen(const QString& path, const QString& languageId, const QString& text);
    void didChange(const QString& path, const QString& text);
    void didClose(const QString& path);
    // line/col 0-based. Yanıt handler'a result objesiyle gelir.
    void requestHover(const QString& path, int line, int col,
                      std::function<void(QJsonObject)> handler);
    void requestDefinition(const QString& path, int line, int col,
                           std::function<void(QJsonObject)> handler);

    // --- Stage 13: genel erişim ---
    // method + params ile istek gönder; handler result objesini alır. İstek kimliği döner.
    int request(const QString& method, const QJsonObject& params,
                std::function<void(QJsonObject)> handler);
    void notify(const QString& method, const QJsonObject& params);
    void cancelRequest(int id); // $/cancelRequest gönderir + handler'ı düşürür
    QJsonObject serverCaps() const { return m_caps; }
    // capability yokluğu: false (bilinmiyorsa true — iyimser)
    bool hasCap(const QString& dotted) const;
    // Kolaylık istekleri (tümü textPos tabanlı)
    void requestCompletion(const QString& path, int line, int col,
                           std::function<void(QJsonObject)> h);
    void requestReferences(const QString& path, int line, int col,
                           std::function<void(QJsonObject)> h);
    void requestRename(const QString& path, int line, int col, const QString& newName,
                       std::function<void(QJsonObject)> h);
    void requestCodeAction(const QString& path, int line, int col, int endLine, int endCol,
                           const QStringList& only, std::function<void(QJsonObject)> h);
    void requestSignature(const QString& path, int line, int col,
                          std::function<void(QJsonObject)> h);
    void requestDocSymbols(const QString& path, std::function<void(QJsonObject)> h);
    void requestWorkspaceSymbols(const QString& query, std::function<void(QJsonObject)> h);
    void requestFormat(const QString& path, std::function<void(QJsonObject)> h);
    // Stage 27: derinlik istekleri
    void requestRangeFormat(const QString& path, int sLine, int sCol, int eLine, int eCol,
                            std::function<void(QJsonObject)> h);
    void requestDocumentLink(const QString& path, std::function<void(QJsonObject)> h);
    void requestDocumentColor(const QString& path, std::function<void(QJsonObject)> h);
    void requestCodeLens(const QString& path, std::function<void(QJsonObject)> h);
    void requestSelectionRange(const QString& path, const QList<QPair<int, int>>& positions,
                               std::function<void(QJsonObject)> h);
    void requestTypePrepare(const QString& path, int line, int col,
                            std::function<void(QJsonObject)> h);
    void requestTypeSupertypes(const QJsonObject& item, std::function<void(QJsonObject)> h);
    void requestTypeSubtypes(const QJsonObject& item, std::function<void(QJsonObject)> h);
    void requestPullDiagnostics(const QString& path, std::function<void(QJsonObject)> h);
    void didChangeConfiguration(const QJsonObject& settings);
    // Stage 27: ileti günlüğü (teşhis için, en çok 200)
    QStringList messageLog() const { return m_log; }
    void clearLog() { m_log.clear(); }
    void requestInlay(const QString& path, int startLine, int endLine,
                      std::function<void(QJsonObject)> h);
    void requestSemantic(const QString& path, std::function<void(QJsonObject)> h);
    void requestCallPrepare(const QString& path, int line, int col,
                            std::function<void(QJsonObject)> h);
    void requestCallIncoming(const QJsonObject& item, std::function<void(QJsonObject)> h);
    void requestCallOutgoing(const QJsonObject& item, std::function<void(QJsonObject)> h);

    static QString pathToUri(const QString& path);
    static QString uriToPath(const QString& uri);

signals:
    void started();
    void diagnosticsReady(const QString& path, const QList<LspDiag>& diags);
    void serverError(const QString& msg);
    // Stage 42: istek zaman aşımı (yöntem + kimlik; handler düşürülür)
    void requestTimedOut(const QString& method, int id);
    // Stage 27: $/progress (begin/report/end)
    void progressUpdate(const QString& kind, const QString& title, int percent);

private slots:
    void onReadyRead();
    void onFinished(int code);
    void onRequestTimeout(int id); // Stage 42

private:
    void sendMessage(const QJsonObject& obj);
    void sendRequest(const QString& method, const QJsonObject& params,
                     std::function<void(QJsonObject)> handler);
    void sendNotification(const QString& method, const QJsonObject& params);
    void handleMessage(const QJsonObject& obj);

    QProcess m_proc;
    QByteArray m_buf;
    int m_nextId = 1;
    int m_initId = -1;
    bool m_ready = false;
    QString m_rootUri;
    QStringList m_log; // Stage 27: son iletiler (yön + özet)
    void logMsg(const QString& dir, const QString& text);
    QJsonObject m_caps; // Stage 13: sunucu yetenekleri (initialize result.capabilities)
    QMap<int, std::function<void(QJsonObject)>> m_handlers;
    // Stage 42: zaman aşımı takibi (id -> yöntem + zamanlayıcı)
    QMap<int, QString> m_pendingMethod;
    QMap<int, QTimer*> m_pendingTimers;
    int m_requestTimeoutMs = 30000;
    QMap<QString, int> m_versions; // path -> version
public:
    // Stage 42: istek zaman aşımı (varsayılan 30 sn; 0 = kapalı)
    void setRequestTimeoutMs(int ms) { m_requestTimeoutMs = qMax(0, ms); }
    int requestTimeoutMs() const { return m_requestTimeoutMs; }
    int pendingCount() const { return m_handlers.size(); }
private:
};
