#include "ModelArenaDialog.h"
#include "../core/ModelCapabilities.h"
#include "../core/ResponseScorer.h"
#include "../core/ai/AiProfiles.h"
#include "../core/ai/AiRunner.h"
#include "../core/ai/LlmProvider.h"
#include "../core/ai/ProviderHealth.h"
#include "../core/ai/ProviderPrefs.h"
#include "../core/ai/ProviderPricing.h"
#include "../core/ai/SecretStore.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QtConcurrent>
#include <algorithm>

QString ArenaTarget::label() const {
    const ProviderSpec s = ProviderRegistry::byId(providerId);
    const QString p = s.label.isEmpty() ? providerId : s.label;
    return QString("%1 · %2").arg(p, model);
}

namespace {

// Yeni gövde üreticisini (ProviderCodec) doğrudan kullanan sunucu çağrısı yok:
// tüm gövde/başlık çözümlemesi AiRunner + ProviderCodec içinde.
QString shortUsd(double usd) {
    if (usd <= 0.0) return QStringLiteral("ücretsiz");
    if (usd < 0.01) return QStringLiteral("$%1").arg(usd, 0, 'f', 5);
    return QStringLiteral("$%1").arg(usd, 0, 'f', 4);
}

} // namespace

QList<ArenaTarget> ModelArenaDialog::defaultTargets(const QStringList& localModels,
                                                    const QString& activeProvider, int maxCount) {
    QList<ArenaTarget> out;
    const QString active = activeProvider.isEmpty() ? ProviderPrefs::activeProvider()
                                                    : activeProvider;
    // 1) Yerel modeller (aktif sağlayıcı)
    for (const QString& m : localModels) {
        if (m.trimmed().isEmpty()) continue;
        if (out.size() >= maxCount) return out;
        out << ArenaTarget{active, m.trimmed()};
    }
    // 2) Yapılandırılmış diğer sağlayıcılar (anahtarı olan ya da anahtarsız yerel uyumlu)
    SecretStore store;
    for (const ProviderSpec& spec : ProviderRegistry::all()) {
        if (out.size() >= maxCount) break;
        if (spec.id == active) continue;
        if (spec.requiresKey() && store.effectiveKey(spec.id).isEmpty()) continue;
        const QString model = ProviderPrefs::modelFor(
            spec.id, ProviderRegistry::sampleModels(spec.id).value(0));
        if (model.isEmpty()) continue;
        if (ModelCapabilities::isEmbeddingModel(model)) continue; // sohbet yarışına gömme giremez
        bool dup = false;
        for (const ArenaTarget& t : out)
            if (t.providerId == spec.id && t.model == model) dup = true;
        if (!dup) out << ArenaTarget{spec.id, model};
    }
    return out;
}

ModelArenaDialog::ModelArenaDialog(const QList<ArenaTarget>& targets,
                                   const QString& systemPrompt, const QString& prompt,
                                   const QJsonObject& options, QWidget* parent)
    : QDialog(parent), m_system(systemPrompt), m_options(options), m_targets(targets) {
    setWindowTitle("Model Arena — çok sağlayıcı karşılaştırma");
    resize(1040, 660);
    auto* lay = new QVBoxLayout(this);

    auto* note = new QLabel(
        "Her hedef (sağlayıcı, model) aynı istemi koşturur; gecikme, jeton ve "
        "<b>gerçek maliyet</b> sağlayıcıya göre hesaplanır.", this);
    note->setWordWrap(true);
    note->setStyleSheet("color:#858585;");
    lay->addWidget(note);

    auto* top = new QHBoxLayout;
    top->addWidget(new QLabel("Modeller (virgülle):", this));
    m_models = new QLineEdit(this);
    QStringList labels;
    for (const ArenaTarget& t : m_targets) labels << t.label();
    m_models->setText(labels.join(", "));
    m_models->setToolTip("Virgülle ayrılmış sağlayıcı · model listesi");
    top->addWidget(m_models, 1);
    auto* bRun = new QPushButton("Koştur", this);
    top->addWidget(bRun);
    bAdopt = new QPushButton("Kazananı Sohbete Al", this);
    bAdopt->setEnabled(false);
    top->addWidget(bAdopt);
    lay->addLayout(top);

    m_prompt = new QPlainTextEdit(this);
    m_prompt->setPlainText(prompt);
    m_prompt->setMaximumHeight(110);
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
    connect(bAdopt, &QPushButton::clicked, this, &ModelArenaDialog::adopt);
    if (!prompt.isEmpty() && !targets.isEmpty()) run();
}

ModelArenaDialog::Result ModelArenaDialog::callTarget(const ArenaTarget& t,
                                                      const QString& system,
                                                      const QString& prompt) {
    Result r;
    r.providerId = t.providerId;
    r.model = t.model;
    // AiRunner her çağrıyı kendi iş parçacığında yapar; ağ kodu tek yerde.
    AiRunner runner;
    runner.setRecordUsage(false); // arena maliyeti raporda gösterilir, kasa tekrar yazmasın
    AiRunner::Options o = AiRunner::optionsFor(AiTask::Arena, system);
    o.providerId = t.providerId;
    o.model = t.model;
    o.bypassRouting = true; // bu hedefi birebir kullan
    o.allowFailover = false; // adaletli karşılaştırma: herkes kendi hatasıyla eşit
    o.record = false;
    o.timeoutMs = 60000;
    const AiRunner::Result rr = runner.run(o, prompt);
    r.ms = rr.ms;
    r.text = rr.text;
    r.error = rr.error;
    r.promptTokens = rr.usage.promptTokens;
    r.evalTokens = rr.usage.evalTokens;
    r.usd = rr.usd;
    r.score = ResponseScorer::score(rr.text).score;
    return r;
}

void ModelArenaDialog::run() {
    // Metin alanındaki "sağlayıcı · model" listesini hedeflere eşle
    QList<ArenaTarget> targets;
    for (const ArenaTarget& t : m_targets) {
        bool wanted = false;
        for (const QString& raw : m_models->text().split(QLatin1Char(','), Qt::SkipEmptyParts))
            if (raw.trimmed() == t.label()) wanted = true;
        if (wanted) targets << t;
    }
    if (targets.isEmpty()) {
        m_status->setText("En az bir hedef seçin.");
        return;
    }
    const QString prompt = m_prompt->toPlainText().trimmed();
    if (prompt.isEmpty()) {
        m_status->setText("İstem boş.");
        return;
    }
    while (QLayoutItem* it = m_results->takeAt(0)) {
        if (QWidget* w = it->widget()) w->deleteLater();
        delete it;
    }
    bAdopt->setEnabled(false);
    m_status->setText(QString("%1 hedef koşuyor...").arg(targets.size()));
    QApplication::processEvents();

    const QString system = m_system;
    const QList<ModelArenaDialog::Result> results = QtConcurrent::blockingMapped(
        targets, [system, prompt](const ArenaTarget& t) { return callTarget(t, system, prompt); });

    // Sıralama: hatasız olanlar önce, sonra kalite
    QList<ModelArenaDialog::Result> sorted = results;
    std::sort(sorted.begin(), sorted.end(),
              [](const Result& a, const Result& b) {
                  if (a.error.isEmpty() != b.error.isEmpty()) return a.error.isEmpty();
                  return a.score > b.score;
              });
    for (const Result& r : sorted) {
        auto* box = new QWidget(this);
        auto* bl = new QVBoxLayout(box);
        auto* head = new QLabel(box);
        head->setTextInteractionFlags(Qt::TextSelectableByMouse);
        if (r.error.isEmpty()) {
            head->setText(QString("<b>%1</b> — %2 ms · %3+%4 token · %5 · kalite: %6")
                              .arg(ArenaTarget{r.providerId, r.model}.label())
                              .arg(r.ms)
                              .arg(r.promptTokens)
                              .arg(r.evalTokens)
                              .arg(shortUsd(r.usd))
                              .arg(ResponseScorer::label(r.score)));
        } else {
            head->setText(QString("<b>%1</b> — <span style='color:#f44747'>HATA: %2</span>")
                              .arg(ArenaTarget{r.providerId, r.model}.label(),
                                   r.error.toHtmlEscaped()));
        }
        auto* body = new QTextBrowser(box);
        body->setPlainText(r.error.isEmpty() ? r.text : r.error);
        body->setMinimumHeight(150);
        bl->addWidget(head);
        bl->addWidget(body);
        m_results->addWidget(box);
    }
    m_last = sorted;
    const int okCount = int(std::count_if(sorted.begin(), sorted.end(),
                                          [](const Result& r) { return r.error.isEmpty(); }));
    double totalUsd = 0.0;
    for (const Result& r : sorted) totalUsd += r.usd;
    m_status->setText(QString("%1/%2 başarılı · toplam %3")
                          .arg(okCount)
                          .arg(sorted.size())
                          .arg(shortUsd(totalUsd)));
    bAdopt->setEnabled(okCount > 0);
}

void ModelArenaDialog::adopt() {
    for (const Result& r : m_last) {
        if (r.error.isEmpty() && !r.text.trimmed().isEmpty()) {
            emit adoptRequested(r.text);
            close();
            return;
        }
    }
}
