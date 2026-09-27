#include "LspClient.h"
#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>

LspClient::LspClient(QObject* parent) : QObject(parent) {
    connect(&m_proc, &QProcess::readyReadStandardOutput, this, &LspClient::onReadyRead);
    connect(&m_proc, &QProcess::readyReadStandardError, this, [this]() {
        // Sunucu logları sessizce yutulur (clangd gevezedir); kritikse status'te gösterilir.
        m_proc.readAllStandardError();
    });
    connect(&m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &LspClient::onFinished);
}

LspClient::~LspClient() { stop(); }

QString LspClient::pathToUri(const QString& path) {
    return QUrl::fromLocalFile(path).toString();
}

QString LspClient::uriToPath(const QString& uri) {
    return QUrl(uri).toLocalFile();
}

bool LspClient::start(const QString& program, const QStringList& args, const QString& workdir) {
    stop();
    m_ready = false;
    m_buf.clear();
    m_handlers.clear();
    m_proc.setWorkingDirectory(workdir);
    m_proc.start(program, args);
    if (!m_proc.waitForStarted(5000)) {
        emit serverError("LSP başlatılamadı: " + program);
        return false;
    }
    m_rootUri = pathToUri(workdir);
    QJsonObject params;
    params["processId"] = (int)QCoreApplication::applicationPid();
    params["rootUri"] = m_rootUri;
    // Stage 27: çok köklü çalışma alanı + ilişkili tanı bilgisi
    params["workspaceFolders"] = QJsonArray{QJsonObject{{"uri", m_rootUri}, {"name", "root"}}};
    QJsonObject wfold;
    wfold["supported"] = false;
    // Stage 13: geniş istemci yetenekleri (sunucu ne verirse kullansın)
    QJsonObject td;
    td["publishDiagnostics"] = QJsonObject{{"relatedInformation", true}}; // Stage 27
    td["hover"] = QJsonObject{{"contentFormat", QJsonArray{"plaintext", "markdown"}}};
    td["completion"] = QJsonObject{{"completionItem", QJsonObject{
        {"snippetSupport", false},
        {"documentationFormat", QJsonArray{"plaintext", "markdown"}}}}};
    td["signatureHelp"] = QJsonObject{{"signatureInformation", QJsonObject{
        {"documentationFormat", QJsonArray{"plaintext"}}}}};
    td["definition"] = QJsonObject{{"linkSupport", true}};
    td["references"] = QJsonObject{};
    td["documentSymbol"] = QJsonObject{{"hierarchicalDocumentSymbolSupport", true}};
    QJsonObject kindSet;
    kindSet["valueSet"] = QJsonArray{"quickfix", "refactor", "source", "source.organizeImports"};
    QJsonObject litSupport;
    litSupport["codeActionKind"] = QJsonObject{{"codeActionKind", kindSet}};
    td["codeAction"] = QJsonObject{{"codeActionLiteralSupport", litSupport}};
    td["formatting"] = QJsonObject{};
    td["inlayHint"] = QJsonObject{};
    QJsonObject semReq;
    semReq["full"] = QJsonObject{};
    QJsonObject sem;
    sem["requests"] = semReq;
    sem["tokenTypes"] = QJsonArray{"namespace", "type", "class", "enum", "interface",
        "struct", "typeParameter", "parameter", "variable", "property", "enumMember",
        "function", "method", "macro", "keyword", "comment", "string", "number", "operator"};
    sem["tokenModifiers"] = QJsonArray{"declaration", "definition", "readonly", "static",
        "deprecated", "abstract", "async", "modification", "documentation", "defaultLibrary"};
    td["semanticTokens"] = sem;
    td["callHierarchy"] = QJsonObject{{"dynamicRegistration", false}};
    // Stage 27: derinlik yetenekleri
    td["documentLink"] = QJsonObject{{"tooltipSupport", true}};
    td["colorProvider"] = QJsonObject{};
    td["codeLens"] = QJsonObject{};
    td["typeHierarchy"] = QJsonObject{{"dynamicRegistration", false}};
    QJsonObject caps;
    caps["textDocument"] = td;
    caps["workspace"] = QJsonObject{{"symbol", QJsonObject{}},
                                        {"workspaceFolders", wfold}};
    params["capabilities"] = caps;
    m_initId = m_nextId;
    sendRequest("initialize", params, [this](QJsonObject res) {
        m_caps = res["capabilities"].toObject();
        sendNotification("initialized", QJsonObject());
        m_ready = true;
        emit started();
    });
    return true;
}

void LspClient::stop() {
    if (m_proc.state() != QProcess::NotRunning) {
        sendNotification("exit", QJsonObject());
        m_proc.terminate();
        if (!m_proc.waitForFinished(1500)) m_proc.kill();
    }
    m_ready = false;
    // Stage 42: bekleyen istekleri düşür (geç yanıtlar hayalet çağrı yapmasın)
    for (QTimer* t : m_pendingTimers) {
        t->stop();
        t->deleteLater();
    }
    m_pendingTimers.clear();
    m_pendingMethod.clear();
    m_handlers.clear();
}

void LspClient::onRequestTimeout(int id) {
    auto it = m_handlers.find(id);
    if (it == m_handlers.end()) return; // yanıt çoktan geldi
    m_handlers.erase(it);
    if (QTimer* t = m_pendingTimers.take(id)) t->deleteLater();
    const QString method = m_pendingMethod.take(id);
    logMsg("!", QString("zaman aşımı: %1 (#%2)").arg(method).arg(id));
    cancelRequest(id); // sunucuya da bildir (ölüyse no-op)
    emit requestTimedOut(method, id);
}

bool LspClient::isRunning() const {
    return m_proc.state() != QProcess::NotRunning;
}

void LspClient::sendMessage(const QJsonObject& obj) {
    QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray head = "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n";
    m_proc.write(head + body);
}

void LspClient::sendRequest(const QString& method, const QJsonObject& params,
                            std::function<void(QJsonObject)> handler) {
    int id = m_nextId++;
    m_handlers[id] = handler;
    logMsg(">", method); // Stage 27: ileti günlüğü
    sendMessage(QJsonObject{{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", params}});
}

void LspClient::sendNotification(const QString& method, const QJsonObject& params) {
    sendMessage(QJsonObject{{"jsonrpc", "2.0"}, {"method", method}, {"params", params}});
}

void LspClient::didOpen(const QString& path, const QString& languageId, const QString& text) {
    if (!isRunning()) return;
    int v = m_versions.value(path, 0) + 1;
    m_versions[path] = v;
    sendNotification("textDocument/didOpen", QJsonObject{{"textDocument", QJsonObject{
        {"uri", pathToUri(path)}, {"languageId", languageId}, {"version", v}, {"text", text}}}});
}

void LspClient::didChange(const QString& path, const QString& text) {
    if (!isRunning() || !m_versions.contains(path)) return;
    int v = m_versions[path] + 1;
    m_versions[path] = v;
    QJsonArray changes{QJsonObject{{"text", text}}};
    sendNotification("textDocument/didChange", QJsonObject{
        {"textDocument", QJsonObject{{"uri", pathToUri(path)}, {"version", v}}},
        {"contentChanges", changes}});
}

void LspClient::didClose(const QString& path) {
    if (!isRunning()) return;
    m_versions.remove(path);
    sendNotification("textDocument/didClose", QJsonObject{
        {"textDocument", QJsonObject{{"uri", pathToUri(path)}}}});
}

static QJsonObject textPos(const QString& path, int line, int col) {
    return QJsonObject{
        {"textDocument", QJsonObject{{"uri", LspClient::pathToUri(path)}}},
        {"position", QJsonObject{{"line", line}, {"character", col}}}};
}

void LspClient::requestHover(const QString& path, int line, int col,
                             std::function<void(QJsonObject)> handler) {
    if (!m_ready) return;
    sendRequest("textDocument/hover", textPos(path, line, col), handler);
}

void LspClient::requestDefinition(const QString& path, int line, int col,
                                  std::function<void(QJsonObject)> handler) {
    if (!m_ready) return;
    sendRequest("textDocument/definition", textPos(path, line, col), handler);
}

void LspClient::onReadyRead() {
    m_buf += m_proc.readAllStandardOutput();
    while (true) {
        int hi = m_buf.indexOf("\r\n\r\n");
        int sepLen = 4;
        if (hi < 0) { // bazı sunucular \n\n kullanır
            hi = m_buf.indexOf("\n\n");
            sepLen = 2;
        }
        if (hi < 0) break;
        QByteArray header = m_buf.left(hi);
        int len = -1;
        for (const QByteArray& ln : header.split('\n')) {
            QByteArray t = ln.trimmed();
            if (t.startsWith("Content-Length:"))
                len = t.mid(15).trimmed().toInt();
        }
        if (len < 0) { m_buf.clear(); break; }
        if (m_buf.size() < hi + sepLen + len) break; // tamamı gelmedi
        QByteArray body = m_buf.mid(hi + sepLen, len);
        m_buf = m_buf.mid(hi + sepLen + len);
        QJsonDocument d = QJsonDocument::fromJson(body);
        if (d.isObject()) handleMessage(d.object());
    }
}

void LspClient::handleMessage(const QJsonObject& obj) {
    if (obj.contains("method")) {
        QString m = obj["method"].toString();
        if (m == "textDocument/publishDiagnostics") {
            QJsonObject p = obj["params"].toObject();
            QString path = uriToPath(p["uri"].toString());
            QList<LspDiag> diags;
            for (const QJsonValue& v : p["diagnostics"].toArray()) {
                QJsonObject d = v.toObject();
                QJsonObject r = d["range"].toObject();
                QJsonObject s = r["start"].toObject(), e = r["end"].toObject();
                LspDiag dg;
                dg.path = path;
                dg.line = s["line"].toInt();
                dg.col = s["character"].toInt();
                dg.endLine = e["line"].toInt();
                dg.endCol = e["character"].toInt();
                dg.severity = d["severity"].toInt(1);
                dg.message = d["message"].toString();
                dg.source = d["source"].toString();
                diags << dg;
            }
            emit diagnosticsReady(path, diags);
        }
        if (m == "$/progress") {
            // Stage 27: {"token":..,"value":{"kind":"begin|report|end",...}}
            const QJsonObject v = obj["params"].toObject()["value"].toObject();
            const QString kind = v["kind"].toString();
            const QString title = v["title"].toString();
            int pct = v["percentage"].toInt(-1);
            if (kind == "end") pct = 100;
            emit progressUpdate(kind, title, pct);
            return;
        }
        if (m == "window/showMessage" || m == "window/logMessage") {
            logMsg("<", m + ": " + obj["params"].toObject()["message"].toString().left(160));
            return;
        }
        // diğer bildirimler yoksayılır
        return;
    }
    if (obj.contains("id")) {
        int id = obj["id"].toInt();
        auto it = m_handlers.find(id);
        if (it != m_handlers.end()) {
            auto h = *it;
            m_handlers.erase(it);
            // Stage 42: yanıt geldi — zamanlayıcıyı temizle
            if (QTimer* t = m_pendingTimers.take(id)) {
                t->stop();
                t->deleteLater();
            }
            m_pendingMethod.remove(id);
            // Stage 13: sonuç dizi de olabilir (references/completion/symbols)
            const QJsonValue r = obj.value("result");
            if (r.isObject()) h(r.toObject());
            else h(QJsonObject{{"result", r}});
        }
    }
}

void LspClient::onFinished(int) {
    bool wasReady = m_ready;
    m_ready = false;
    if (wasReady) emit serverError("LSP sunucusu kapandı.");
}

// ---------- Stage 13: genel erişim + kolaylık istekleri ----------

int LspClient::request(const QString& method, const QJsonObject& params,
                       std::function<void(QJsonObject)> handler) {
    if (!m_ready && method != "initialize") {
        // Hazır değilse sessizce yut (çağıran status gösterir)
        return -1;
    }
    int id = m_nextId++;
    m_handlers[id] = handler;
    sendMessage(QJsonObject{{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", params}});
    // Stage 42: zaman aşımı — yanıtsız sunucuda handler sonsuza dek kalmasın
    if (m_requestTimeoutMs > 0) {
        auto* t = new QTimer(this);
        t->setSingleShot(true);
        m_pendingMethod[id] = method;
        m_pendingTimers[id] = t;
        connect(t, &QTimer::timeout, this, [this, id]() { onRequestTimeout(id); });
        t->start(m_requestTimeoutMs);
    }
    return id;
}

void LspClient::notify(const QString& method, const QJsonObject& params) {
    sendNotification(method, params);
}

void LspClient::cancelRequest(int id) {
    if (id < 0) return;
    m_handlers.remove(id);
    if (isRunning())
        sendNotification("$/cancelRequest", QJsonObject{{"id", id}});
}

bool LspClient::hasCap(const QString& dotted) const {
    if (m_caps.isEmpty()) return true; // henüz initialize yanıtı yoksa iyimser
    QStringList parts = dotted.split('.');
    QJsonValue v = QJsonValue(m_caps);
    for (const QString& p : parts) {
        if (!v.isObject()) return false;
        v = v.toObject().value(p);
        if (v.isUndefined() || v.isNull()) return false;
    }
    if (v.isBool()) return v.toBool();
    return true; // obje/varlık = destekleniyor
}

void LspClient::requestCompletion(const QString& path, int line, int col,
                                  std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    QJsonObject p = textPos(path, line, col);
    p["context"] = QJsonObject{{"triggerKind", 1}};
    request("textDocument/completion", p, h);
}

void LspClient::requestReferences(const QString& path, int line, int col,
                                  std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    QJsonObject p = textPos(path, line, col);
    p["context"] = QJsonObject{{"includeDeclaration", true}};
    request("textDocument/references", p, h);
}

void LspClient::requestRename(const QString& path, int line, int col, const QString& newName,
                              std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    QJsonObject p = textPos(path, line, col);
    p["newName"] = newName;
    request("textDocument/rename", p, h);
}

void LspClient::requestCodeAction(const QString& path, int line, int col, int endLine, int endCol,
                                  const QStringList& only,
                                  std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    QJsonObject range = QJsonObject{
        {"start", QJsonObject{{"line", line}, {"character", col}}},
        {"end", QJsonObject{{"line", endLine}, {"character", endCol}}}};
    QJsonObject ctx = QJsonObject{{"diagnostics", QJsonArray()}};
    if (!only.isEmpty()) {
        QJsonArray arr;
        for (const QString& k : only) arr << k;
        ctx["only"] = arr;
    }
    request("textDocument/codeAction",
            QJsonObject{{"textDocument", QJsonObject{{"uri", pathToUri(path)}}},
                        {"range", range}, {"context", ctx}}, h);
}

void LspClient::requestSignature(const QString& path, int line, int col,
                                 std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    request("textDocument/signatureHelp", textPos(path, line, col), h);
}

void LspClient::requestDocSymbols(const QString& path, std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    request("textDocument/documentSymbol",
            QJsonObject{{"textDocument", QJsonObject{{"uri", pathToUri(path)}}}}, h);
}

void LspClient::requestWorkspaceSymbols(const QString& query,
                                        std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    request("workspace/symbol", QJsonObject{{"query", query}}, h);
}

void LspClient::requestFormat(const QString& path, std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    request("textDocument/formatting",
            QJsonObject{{"textDocument", QJsonObject{{"uri", pathToUri(path)}}},
                        {"options", QJsonObject{{"tabSize", 4}, {"insertSpaces", true}}}}, h);
}

// Stage 27: derinlik istekleri
void LspClient::requestRangeFormat(const QString& path, int sLine, int sCol, int eLine,
                                   int eCol, std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    QJsonObject range = QJsonObject{
        {"start", QJsonObject{{"line", sLine}, {"character", sCol}}},
        {"end", QJsonObject{{"line", eLine}, {"character", eCol}}}};
    request("textDocument/rangeFormatting",
            QJsonObject{{"textDocument", QJsonObject{{"uri", pathToUri(path)}}},
                        {"range", range},
                        {"options", QJsonObject{{"tabSize", 4}, {"insertSpaces", true}}}}, h);
}

void LspClient::requestDocumentLink(const QString& path,
                                    std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    request("textDocument/documentLink",
            QJsonObject{{"textDocument", QJsonObject{{"uri", pathToUri(path)}}}}, h);
}

void LspClient::requestDocumentColor(const QString& path,
                                     std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    request("textDocument/documentColor",
            QJsonObject{{"textDocument", QJsonObject{{"uri", pathToUri(path)}}}}, h);
}

void LspClient::requestCodeLens(const QString& path, std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    request("textDocument/codeLens",
            QJsonObject{{"textDocument", QJsonObject{{"uri", pathToUri(path)}}}}, h);
}

void LspClient::requestSelectionRange(const QString& path,
                                      const QList<QPair<int, int>>& positions,
                                      std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    QJsonArray arr;
    for (const auto& p : positions)
        arr.append(QJsonObject{{"line", p.first}, {"character", p.second}});
    request("textDocument/selectionRange",
            QJsonObject{{"textDocument", QJsonObject{{"uri", pathToUri(path)}}},
                        {"positions", arr}},
            h);
}

void LspClient::requestTypePrepare(const QString& path, int line, int col,
                                   std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    request("textDocument/prepareTypeHierarchy", textPos(path, line, col), h);
}

void LspClient::requestTypeSupertypes(const QJsonObject& item,
                                      std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    request("typeHierarchy/supertypes", QJsonObject{{"item", item}}, h);
}

void LspClient::requestTypeSubtypes(const QJsonObject& item,
                                    std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    request("typeHierarchy/subtypes", QJsonObject{{"item", item}}, h);
}

void LspClient::requestPullDiagnostics(const QString& path,
                                       std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    request("textDocument/diagnostic",
            QJsonObject{{"textDocument", QJsonObject{{"uri", pathToUri(path)}}},
                        {"previousResultId", ""}},
            h);
}

void LspClient::didChangeConfiguration(const QJsonObject& settings) {
    if (!m_ready) return;
    sendNotification("workspace/didChangeConfiguration",
                     QJsonObject{{"settings", settings}});
}

void LspClient::logMsg(const QString& dir, const QString& text) {
    m_log << dir + " " + text.left(200);
    while (m_log.size() > 200) m_log.removeFirst();
}

void LspClient::requestInlay(const QString& path, int startLine, int endLine,
                             std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    QJsonObject range = QJsonObject{
        {"start", QJsonObject{{"line", startLine}, {"character", 0}}},
        {"end", QJsonObject{{"line", endLine}, {"character", 0}}}};
    request("textDocument/inlayHint",
            QJsonObject{{"textDocument", QJsonObject{{"uri", pathToUri(path)}}},
                        {"range", range}}, h);
}

void LspClient::requestSemantic(const QString& path, std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    request("textDocument/semanticTokens/full",
            QJsonObject{{"textDocument", QJsonObject{{"uri", pathToUri(path)}}}}, h);
}

void LspClient::requestCallPrepare(const QString& path, int line, int col,
                                   std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    request("textDocument/prepareCallHierarchy", textPos(path, line, col), h);
}

void LspClient::requestCallIncoming(const QJsonObject& item,
                                    std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    request("callHierarchy/incomingCalls", QJsonObject{{"item", item}}, h);
}

void LspClient::requestCallOutgoing(const QJsonObject& item,
                                    std::function<void(QJsonObject)> h) {
    if (!m_ready) return;
    request("callHierarchy/outgoingCalls", QJsonObject{{"item", item}}, h);
}

// ---------- Stage 16: uzak LSP (ssh stdio köprüsü) ----------

bool LspClient::startRemote(const QString& remoteLsp, const QStringList& remoteArgs,
                            const QString& sshTarget, const QStringList& sshBaseArgs,
                            const QString& remoteRoot) {
    QString cmd = remoteLsp.isEmpty() ? "clangd" : remoteLsp;
    for (const QString& a : remoteArgs)
        cmd += " '" + QString(a).replace("'", "'\"'\"'") + "'";
    QString full = remoteRoot.trimmed().isEmpty()
        ? cmd
        : "cd '" + QString(remoteRoot.trimmed()).replace("'", "'\"'\"'") + "' && " + cmd;
    QStringList args = sshBaseArgs;
    args << sshTarget << full;
    // Uzak kök URI olarak kullanılamaz (yerel dosya değil); LSP yolları uzak
    // mutlak yol olarak gider — rootUri'yu uzak kökten file:// gibi kur.
    return start("ssh", args,
                 remoteRoot.trimmed().isEmpty() ? QString("/") : remoteRoot.trimmed());
}
