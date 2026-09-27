#include "AboutInfo.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QtGlobal>

QString AboutInfo::version() { return "3.2.0"; }
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
        "3.2.0 (Stage 50)\n"
        "  • Sınırlar + retrospektif belgeleri\n"
        "  • ai-probe final + meta-test (617 kaynak)\n"
        "3.1.0 (Stage 49)\n"
        "  • Birlikte çalışma (evsahibi/katıl, eş imleçleri)\n"
        "  • Hayalet imleç düzeltmesi\n"
        "3.0.0 (Stage 48)\n"
        "  • Taşınabilir kip (--portable)\n"
        "  • Haftalık güncelleme denetimi (kapatılabilir)\n"
        "  • Çökme dökümleri sekmesi\n"
        "2.9.0 (Stage 47)\n"
        "  • Ekran okuyucu adları (çip/sekme/düzenleyici)\n"
        "  • Kontrast + çeviri + renk körü kilit testleri\n"
        "2.8.0 (Stage 46)\n"
        "  • Tembel sekme geri yükleme (hızlı açılış)\n"
        "  • Ollama otomatik başlatma ayarı\n"
        "  • Performans bütçeleri + kilit testleri\n"
        "2.7.0 (Stage 45)\n"
        "  • Dolaylı enjeksiyon koruması (tarama + bütçe)\n"
        "  • Adım adım geri alma (revertSteps)\n"
        "2.6.0 (Stage 44)\n"
        "  • sftp toplu-iş alıntı açığı düzeltildi\n"
        "  • Sahte ssh/sftp testleri (34 kontrol)\n"
        "2.5.0 (Stage 43)\n"
        "  • Git stash satırı + önde/geride sayacı\n"
        "  • Gerçek depo testleri (38 kontrol)\n"
        "2.4.0 (Stage 42)\n"
        "  • LSP istek zaman aşımı (hayalet çağrı sonu)\n"
        "  • Sahte sunucu testleri + canlı clangd hover\n"
        "2.3.0 (Stage 41)\n"
        "  • Eklenti dosya erişimi çalışma alanına kapsandı (açık kapatıldı)\n"
        "  • verso.currentFile() + kelime sayacı örnek eklentisi\n"
        "  • Bilinmeyen izinler günlüğe bildiriliyor\n"
        "2.2.0 (Stage 40)\n"
        "  • Kasa tek-yazıcı + doctor/repair tutarlılık denetimi\n"
        "  • Oturumluk güvenli komut izni (varsayılan kapalı)\n"
        "  • fetchModels zaman aşımı + tembel katalog (hızlı açılış)\n"
        "  • RAG rozeti, kota %80 uyarısı, yoğunluk sayacı\n"
        "  • 'Son Komutlar' canlı yenileniyor\n"
        "2.1.0 (Stage 39)\n"
        "  • Canlı sigorta doğrulaması: devre açılıyor, yönlendirme ayıklıyor\n"
        "  • Canlı RAG devamı: bekleme diske, ikinci koşu kaldığı yerden\n"
        "  • Onayda kabuk metakarakteri uyarısı (ikame/borulama/kök yazma)\n"
        "  • Yazma kuyruğu eşiği: toplu onayda tek tek gözden geçirme\n"
        "  • Yönlendirme rehberi gerçek ağ yoklaması yapabiliyor\n"
        "2.0.0 (Stage 38)\n"
        "  • Güvenlik: sembolik bağlantı kaçışı kapatıldı (yol denetimi)\n"
        "  • Ajan komutları: profil yüklenmiyor, anahtarlar komuttan temizleniyor\n"
        "  • Komut denetimi: her komut onay/ret/çıkış koduyla kayda giriyor\n"
        "  • İlk kez arayüz testleri: diyaloglar + MainWindow (offscreen)\n"
        "  • Sağlayıcı sigortası: yoğun sağlayıcı 120 sn yönlendirme dışı\n"
        "  • Kesintili RAG indeksleme (ilerleme kaydedilir, kaldığı yerden sürer)\n"
        "  • Ayarlar → AI Kullanımı: eylem listesi + AI önbellek boyutu/temizleme\n"
        "1.9.0 (Stage 37)\n"
        "  • Tek AI cephesi: 12 görevin tamamı aynı sağlayıcı/yönlendirme/kota yolunu kullanır\n"
        "  • Dokuz yüzey artık bulut sağlayıcılarla da çalışır (inceleme, test, commit,\n"
        "    belge, açıklama, satır içi düzeltme, hayalet tamamlama, tema, arena)\n"
        "  • Arena: sağlayıcı × model yarışı, gecikme/jeton/gerçek maliyet, kazananı al\n"
        "  • Hayalet tamamlama: yerelde FIM, bulutta önek modu (varsayılan kapalı)\n"
        "  • Konuşma özeti sağlayıcıdan arka planda istenir (arayüz bloklanmaz)\n"
        "  • Durum çubuğu sağlayıcı çipi (sağlık + kota), Ctrl+. ile AI iptali\n"
        "  • Ayarlar → AI Kullanımı: günlük token/maliyet, dağılım, kota, sağlık\n"
        "1.8.0 (Stage 36)\n"
        "  • Sağlayıcı+model bazlı fiyatlandırma; ücretsiz katmanlar 0 USD\n"
        "  • Kalıcı token geçmişi + günlük istek/token kotası (kota dolunca erken ret)\n"
        "  • Koşu maliyet onayı: tavan aşılırsa ajan başlamadan uyarır\n"
        "  • Sağlık skoru + otomatik failover (kalıcı hatada eşdeğer sağlayıcıya devir)\n"
        "  • Görev yönlendirme: basit işler hızlı/ücretsiz, karmaşık işler güçlü model\n"
        "  • RAG gömmesi tüm sağlayıcılarda (NIM/OpenAI/Gemini/UnoRouter) + önbellek\n"
        "  • Ayarlar: maliyet tavanı, kota, yönlendirme, Sağlayıcıları Karşılaştır\n"
        "1.7.0 (Stage 35)\n"
        "  • Çok sağlayıcılı AI: NVIDIA NIM, UnoRouter, OpenAI, Claude, Gemini, Groq,\n"
        "    OpenRouter, DeepSeek, Mistral, xAI, Together, Azure, LM Studio, llama.cpp, vLLM\n"
        "  • Tek arayüz: akışlı/akışsız sohbet, gömme, görsel, model kataloğu, araç çağırma\n"
        "  • API anahtarları OS anahtar deposunda ya da 0600 dosyada (düz metin ayar yok)\n"
        "  • 429/5xx için üstel geri çekilme, sağlayıcıya özel hata mesajları\n"
        "  • Ayarlar → AI Sağlayıcıları: model çekme + bağlantı testi\n"
        "1.6.0 (Stage 33-34)\n"
        "  • AI derinliği: görü (vision), gömme tabanlı hibrit RAG v2, istem galerisi, arena\n"
        "  • AI otonomisi: politika/bütçe motoru, beceri zinciri, ajan belleği, refleksiyon\n"
        "  • Koşu günlüğü + tek tıkla geri alma, proje sağlık skoru, otonom salt-okunur denetim\n"
        "  • Ajan paneli, yeni ajan araçları, tehlikeli komut engeli\n"
        "1.5.0 (Stage 31-32)\n"
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
