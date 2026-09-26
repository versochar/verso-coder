#include "AboutInfo.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QtGlobal>

QString AboutInfo::version() { return "1.5.0"; }
QString AboutInfo::appName() { return "Verso Coder"; }

QString AboutInfo::buildInfo() {
    return QString("Qt %1\nDerleyici: %2\nTarih: %3 %4")
        .arg(QT_VERSION_STR)
#if defined(__clang__)
        .arg(QString("Clang %1.%2").arg(__clang_major__).arg(__clang_minor__))
#elif defined(__GNUC__)
        .arg(QString("GCC %1.%2").arg(__GNUC__).arg(__GNUC_MINOR__))
#else
        .arg("bilinmeyen")
#endif
        .arg(__DATE__)
        .arg(__TIME__);
}

QString AboutInfo::changelog() {
    return QString(
        "1.5.0 (Stage 31-34)\n"
        "  • AI derinliği: görü (vision) girdisi, gömme tabanlı hibrit RAG v2, istem galerisi\n"
        "  • Çok-modelli arena, sohbet dallanma ağacı, yanıt kalite puanı, oturum özeti\n"
        "  • Kararlılık (bütünlük mührü, çökme izi, bellek tavanı) + hız (paralel arama, artımlı RAG)\n"
        "1.4.0 (Stage 19-22)\n"
        "  • Canlı işbirliği (WebSocket) + görev zinciri (dependsOn) + eklenti API v1\n"
        "  • Yeni dosya dil seçimi (20 dil + iskelet) + ilk-çalıştırma sihirbazı\n"
        "  • Pano halkası, çoklu imleç satır sonları, sekme sabitleme, commit şablonları\n"
        "  • Erişilebilirlik + çeviri kapsama denetimleri, tanı raporu, fabrika ayarları\n"
        "1.3.0 (Stage 15-18)\n"
        "  • AI hattı (hayalet tamamlama, ajan, RAG) + uzaktan geliştirme (SSH)\n"
        "  • Editör deneyimi (snippet, outline, yerel geçmiş) + çok köklü profil\n"
        "1.0.0 (Stage 8)\n"
        "  • Komut satırı argümanları + tek örnek (single instance)\n"
        "  • Araç zinciri teşhisi (git/clangd/pylsp/ollama/...)\n"
        "  • Performans metrikleri (başlangıç/yükleme + bellek)\n"
        "  • tasks.json görev çalıştırıcı\n"
        "  • Kısayol profilleri (VS Code / JetBrains / Vim)\n"
        "  • Ayarları JSON olarak dışa/içe aktarma\n"
        "  • Proje çalışma alanı ayarları (.verso/workspace.json)\n"
        "  • Zaman damgalı yedekler + kurtarma\n"
        "  • Güncelleme manifesti kontrolü\n"
        "  • Dağıtım: install kuralları + .desktop + AppImage betiği\n");
}

QString AboutInfo::aboutHtml() {
    QString s;
    s += QString("<h2>%1</h2>").arg(appName());
    s += QString("<p><b>Sürüm:</b> %1</p>").arg(version());
    s += QString("<pre>%1</pre>").arg(buildInfo().toHtmlEscaped());
    s += "<h4>Yenilikler</h4><pre>" + changelog().toHtmlEscaped() + "</pre>";
    return s;
}

AboutInfo::Update AboutInfo::parseManifest(const QString& json) {
    Update u;
    QJsonDocument d = QJsonDocument::fromJson(json.toUtf8());
    if (!d.isObject()) return u;
    QJsonObject o = d.object();
    u.version = o.value("version").toString();
    u.url = o.value("url").toString();
    u.notes = o.value("notes").toString();
    u.valid = !u.version.isEmpty();
    return u;
}

int AboutInfo::compareVersions(const QString& a, const QString& b) {
    auto parts = [](const QString& v) {
        QStringList out;
        for (const QString& p : v.split('-').first().split('.')) out << p;
        return out;
    };
    const QStringList pa = parts(a), pb = parts(b);
    const int n = qMax(pa.size(), pb.size());
    for (int i = 0; i < n; ++i) {
        const int x = i < pa.size() ? pa[i].toInt() : 0;
        const int y = i < pb.size() ? pb[i].toInt() : 0;
        if (x != y) return x < y ? -1 : 1;
    }
    return 0;
}

bool AboutInfo::isNewer(const QString& candidate, const QString& current) {
    return compareVersions(candidate, current) > 0;
}
