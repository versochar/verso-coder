# Verso Coder

Yerel öncelikli, Qt6 ile yazılmış hızlı kod editörü. Sekmeli editör, LSP, Git, uzak geliştirme ve **çok sağlayıcılı yapay zekâ** (yerel Ollama'dan buluta) tek uygulamada.

Sürüm 1.0.4 · MIT lisansı · Linux (AppImage/pacman) · Windows · macOS (Apple Silicon)

## Kurulum

```bash
# Arch Linux (önerilen): özel depo
# /etc/pacman.conf sonuna:
#   [verso]
#   SigLevel = Optional TrustAll
#   Server = https://github.com/versochar/verso-coder/releases/download/repo
sudo pacman -Sy verso-coder
```

- **Linux:** yukarıdaki depo ya da [releases](https://github.com/versochar/verso-coder/releases) sayfasından AppImage (çift tıkla çalışır).
- **Windows/macOS:** releases sayfasından zip/dmg.
- **Kaynaktan:** `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j && ./build/verso-coder`
- **Taşınabilir kip:** `verso-coder --portable` (ayarlar uygulama yanındaki `verso-data/` klasöründe).

## Editör

- Sekmeli düzenleme (bölünmüş görünüm, sabit sekmeler, tembel geri yükleme — 50 sekme tek dosya hızında açılır)
- Çoklu imleç, kod katlama, snippet'ler, akıllı seçim, hunk bazlı diff
- Büyük dosya kipi (2 MB üstü renksiz, 8 MB üstü salt-okunur)
- Komut paleti (`Ctrl+Shift+P`), hızlı dosya açma (`Ctrl+P`), Zen modu (F11)
- 10+ tema, tema galerisi, AI ile tema üretme, renk körü modları, yüksek kontrast teması
- Türkçe/İngilizce arayüz, ilk-çalıştırma sihirbazı

## Yapay zekâ

**20 sağlayıcı, tek cephe.** Ollama (yerel), NVIDIA NIM, UnoRouter, OpenAI, Anthropic, Gemini, Groq, DeepSeek, Mistral, xAI, Together, Azure, OpenRouter, LM Studio, llama.cpp, vLLM ve OpenAI-uyumlu her sunucu. Anahtarı bir kez girersin; sohbet, ajan, hayalet tamamlama, RAG, arena — hepsi aynı hattan geçer.

| Ne | Nasıl |
|---|---|
| Sohbet + streaming | Token token canlı yanıt, `Ctrl+.` ile durdurma |
| 12 görev profili | Açıkla, düzelt, test yaz, belge üret, commit mesajı, inceleme, özet… (`!test` gibi kısayollarla) |
| Ajan modu | Dosya okur/yazar, komut çalıştırır; her yazma ve komut **onayına** tabidir |
| Hayalet tamamlama | Yerelde FIM, bulutta önek modu (bulutta varsayılan kapalı) |
| RAG | Proje indeksleme (anlamsal + hibrit), kesintili indeksleme kaldığı yerden sürer |
| Arena | Aynı istemi birden çok modelde koştur, maliyetle karşılaştır |

**Ekonomi ve dayanıklılık** (Ayarlar → AI Kullanımı):
- Model/sağlayıcı bazında fiyatlandırma ve gerçek maliyet takibi; günlük kota tavanı
- Yoğun modelde **aynı sağlayıcıda** yedeğe geçme (ücretliye sessiz geçiş yok)
- Üst üste yoğunlukta sağlayıcı sigortası (120 sn devre dışı), otomatik failover
- "Neden çalışmıyor?" rehberi: eksik anahtar, kota, ağ sorunu tek listede

### Ücretsiz katmanda ölçülen gerçekler (UnoRouter `:free`)

| Gözlem | Sonuç |
|--------|-------|
| Katalog | 266 model, ~125'i `:free` |
| Sohbet | Model başına **1 istek/dakika** |
| Gömme | **1 istek/30 dakika** — ücretsiz katmanda büyük proje indekslenemez |
| Hız sınırı | Çoğunlukla 429 değil, **403/503** + "all providers busy" |

## Güvenlik

- **Ajan sandbox'ı:** proje kökü dışına erişim yok (sembolik bağlantı kaçışları dahil, gerekçesiyle reddedilir)
- Komutlar `bash -c` ile çalışır (profil/alias bulaşmaz); API anahtarları komut ortamından temizlenir
- Tehlikeli kalıplar engellenir (`rm -rf /`, `curl … | sh`, `sudo`…); şüpheli komutlar onayda uyarı satırıyla gelir
- **Her komut denetlenir:** Ajan Paneli → Son Komutlar (onay/ret/çıkış kodu/süre)
- Araç çıktıları LLM'e dönmeden **enjeksiyon taramasından** geçer
- Eklentiler izinli çalışır (`fs.read`, `fs.write`…), dosya erişimi çalışma alanına kapsamlıdır

## Geliştirici araçları

- **LSP:** clangd/pylsp/rust-analyzer/gopls — hover, tanıma git, bul, yeniden adlandır, hiyerarşi, hızlı düzeltme, semantik renklendirme
- **Hata ayıklama:** GDB paneli (kesme noktası, yığın, değişkenler), `launch.json`, test gezgini, kapsama görünümü
- **Git:** stage/unstage, hunk diff, stash, branch, log grafiği, blame, önde/geride sayacı
- **Uzak geliştirme:** SSH ile dosya/terminal/LSP/git/tünel; bağlantı profilleri
- **Eklentiler:** JavaScript API (`verso.*`), örnek kelime sayacı dahil
- **Birlikte çalışma:** yerel ağda oturum barındır/katıl, renkli eş imleçleri (şifresizdir — yalnız güvenilir ağda)

## Klavye (seçmeler)

`Ctrl+P` dosya aç · `Ctrl+Shift+P` komutlar · `Ctrl+F` bul · `F5` çalıştır · `F9` kesme noktası · `F12` tanıma git · `F2` yeniden adlandır · `Ctrl+.` AI durdur · `F7` yazım denetimi · `Ctrl+=/-` yakınlaştırma. Tamamı Ayarlar → Kısayollar'dan değişir (VS Code / JetBrains / Vim profilleri hazır).

## Bilinen sınırlamalar (kısa)

- Ücretsiz AI katmanları yukarıdaki kotalarla sınırlıdır.
- İşbirliği şifresizdir — internette kullanmayın.
- macOS yalnız Apple Silicon; imza doğrulama yok; RTL diller kapsam dışı.
- Tam liste: [docs/LIMITATIONS.md](docs/LIMITATIONS.md) · [pacman deposu](docs/PACMAN-REPO.md) · [retrospektif](docs/RETROSPECTIVE.md)

## Geliştirme

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
QT_QPA_PLATFORM=offscreen ctest   # 34 paket
./build/ai-probe unorouter doctor # canlı ortam raporu (anahtar gerekir)
```

Sürüm geçmişi Hakkında penceresindedir. Katkı, hata bildirimi ve fikirler için GitHub Issues.
