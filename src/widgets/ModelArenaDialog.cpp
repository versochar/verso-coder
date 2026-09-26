#include "ModelArenaDialog.h"
#include "../core/ResponseScorer.h"
#include <QDateTime>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTextBrowser>
#include <QTimer>
#include <QVBoxLayout>
#include <QtConcurrent>

namespace {

struct ArenaResult {
    QString model;
    QString text;
    QString error;
    qint64 ms = 0;
    int promptTokens = 0;
    int evalTokens = 0;
};

ArenaResult callModel(const QString& host, const QString& model, const QString& system,
                      const QString& prompt, const QJsonObject& options) {
    ArenaResult r;
    r.model = model;
    QNetworkAccessManager net;
    QNetworkRequest req(QUrl(host + "/api/chat"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject root;
    root["model"] = model;
    root["stream"] = false;
    root["options"] = options;
    root["keep_alive"] = "5m";
    QJsonArray msgs;
    if (!system.isEmpty()) msgs.append(QJsonObject{{"role", "system"}, {"content", system}});
    msgs.append(QJsonObject{{"role", "user"}, {"content", prompt}});
    root["messages"] = msgs;

    QElapsedTimer t;
    t.start();
    QNetworkReply* reply = net.post(req, QJsonDocument(root).toJson());
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(180000);
    loop.exec();
    r.ms = t.elapsed();
    if (reply->isRunning()) {
        reply->abort();
        reply->deleteLater();
        r.error = "zaman aşımı";
        return r;
    }
    const QJsonObject o = QJsonDocument::fromJson(reply->readAll()).object();
    if (reply->error() != QNetworkReply::NoError || o.contains("error"))
        r.error = o.value("error").toString(reply->errorString());
    r.text = o.value("message").toObject().value("content").toString();
    r.promptTokens = o.value("prompt_eval_count").toInt();
    r.evalTokens = o.value("eval_count").toInt();
    reply->deleteLater();
    return r;
}

} // namespace

ModelArenaDialog::ModelArenaDialog(const QString& host, const QStringList& models,
                                   const QString& systemPrompt, const QString& prompt,
                                   const QJsonObject& options, QWidget* parent)
    : QDialog(parent), m_host(host), m_system(systemPrompt), m_options(options) {
    setWindowTitle("Model Arena");
    resize(1000, 620);
    auto* lay = new QVBoxLayout(this);

    auto* top = new QHBoxLayout();
    top->addWidget(new QLabel("Modeller (virgülle):", this));
    m_models = new QLineEdit(models.join(", "), this);
    top->addWidget(m_models, 1);
    auto* bRun = new QPushButton("Koştur", this);
    top->addWidget(bRun);
    lay->addLayout(top);

    m_prompt = new QPlainTextEdit(this);
    m_prompt->setPlainText(prompt);
    m_prompt->setMaximumHeight(120);
    lay->addWidget(m_prompt);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    auto* inner = new QWidget(scroll);
    m_results = new QVBoxLayout(inner);
    scroll->setWidget(inner);
    lay->addWidget(scroll, 1);

    m_status = new QLabel("Hazır.", this);
    m_status->setStyleSheet("color:#858585;");
    lay->addWidget(m_status);

    connect(bRun, &QPushButton::clicked, this, &ModelArenaDialog::run);
    if (!prompt.isEmpty()) run();
}

void ModelArenaDialog::run() {
    QStringList models;
    for (const QString& s : m_models->text().split(',', Qt::SkipEmptyParts))
        if (!s.trimmed().isEmpty()) models << s.trimmed();
    if (models.isEmpty()) return;
    const QString prompt = m_prompt->toPlainText().trimmed();
    if (prompt.isEmpty()) return;

    while (QLayoutItem* it = m_results->takeAt(0)) {
        if (QWidget* w = it->widget()) w->deleteLater();
        delete it;
    }
    m_status->setText(QString("%1 model koşuyor...").arg(models.size()));

    const QString host = m_host, system = m_system;
    const QJsonObject options = m_options;
    const QList<ArenaResult> results = QtConcurrent::blockingMapped(
        models, [host, system, prompt, options](const QString& m) {
            return callModel(host, m, system, prompt, options);
        });

    for (const ArenaResult& r : results) {
        auto* box = new QWidget(this);
        auto* bl = new QVBoxLayout(box);
        auto* head = new QLabel(box);
        if (r.error.isEmpty()) {
            const int score = ResponseScorer::score(r.text).score;
            head->setText(QString("<b>%1</b> — %2 ms · %3+%4 token · kalite: %5")
                              .arg(r.model)
                              .arg(r.ms)
                              .arg(r.promptTokens)
                              .arg(r.evalTokens)
                              .arg(ResponseScorer::label(score)));
        } else {
            head->setText(QString("<b>%1</b> — HATA: %2").arg(r.model, r.error));
        }
        auto* body = new QTextBrowser(box);
        body->setPlainText(r.error.isEmpty() ? r.text : r.error);
        body->setMinimumHeight(160);
        bl->addWidget(head);
        bl->addWidget(body);
        m_results->addWidget(box);
    }
    m_status->setText(QString("%1 model tamamlandı.").arg(results.size()));
}
