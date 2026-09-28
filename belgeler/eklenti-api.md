# Verso Coder — Eklenti API v2 Kılavuzu

Eklentiler, eklenti klasöründeki (`Dosya → Eklentiler Klasörünü Aç`) `.js`
dosyalarıdır. Her dosya bir eklentidir; QJSEngine ile çalışır.

## Başlık manifesti

```js
// @name Selam Durumu
// @version 1.0.0
// @description Ne yaptığı (isteğe bağlı)
// @permission ui, fs.write
```

İzinler: `fs.read`, `fs.write`, `events`, `ui`, `net` (`fetch` için). İzinsiz çağrı engellenir ve
Teşhis → Eklentiler sekmesine düşer. İzinler yönetici diyaloğundan
kısıtlanabilir (`plugin/perms/<id>` geçersiz kılma).

## `verso` nesnesi

| Çağrı | İzin | Açıklama |
|---|---|---|
| `log(mesaj)` | — | Günlük + durum çubuğu |
| `registerCommand(id, başlık, fn)` | — | `plugin.<eklenti>.<id>` komutu (`callCommand(arg)` ile tetiklenir) |
| `readFile(yol)` / `writeFile(yol, metin)` | `fs.read` / `fs.write` | Dosya erişimi |
| `onEvent(ad, fn)` | `events` | `save`, `open`, `startup`, `language` (arg: yol) |
| `showStatus(metin, ms)` | `ui` | Durum çubuğu (ms sonra temizlenir) |
| `quickPick(jsonDizi, ipucu)` | `ui` | Listeden seç (dizi JSON ya da satırlı metin) |
| `inputBox(soru, varsayılan)` | `ui` | Metin sor |
| `sendTerminal(metin)` | `ui` | Terminal kabuğuna gönder |
| `fetch(url, ms)` | `net` | HTTP GET (eşzamanlı, en çok 1 MB) |
| `reportProblems(json)` | `ui` | `[{file, line, message}]` → Sorunlar paneli |
| `registerView(id, başlık, fn)` | `ui` | `fn()` HTML döndürür, paletten açılır |
| `registerTheme(ad, json)` | `ui` | Tema JSON'u içe aktarılır |
| `registerKeybinding(komutId, tuş)` | `ui` | Çakışmasızsa atanır |
| `execCommand(komutId)` | `ui` | Editör komutunu çalıştır |
| `get/setGlobalState(k, v)` | — | Eklentiye özel kalıcı alan |
| `get/setWorkspaceState(k, v)` | — | Proje başına alan |
| `get/setConfig(k, v)` | — | Yönetici diyaloğundan düzenlenebilir ayar |

## Güvenlik

- İlk yüklemede dosya mühürlenir (sha256); değişirse uyarı verilir.
- Hata yapan eklenti 3 hatada **karantinaya** alınır (yönetciden kaldırılır).
- Güvenilmez çalışma alanında eklentiler hiç yüklenmez.

## Örnekler

`resources/plugins/`: `echo.js` (v1), `selam.js` (durum + giriş + sayaç),
`notal.js` (quickPick + dosya + görünüm). İlk açılışta klasöre kopyalanırlar.

## Mağaza eklentileri (kayıtta)

| Eklenti | Ne yapar | İzinler |
|---|---|---|
| Markdown Önizleme (`md`) | `.md` → HTML görünüm | `fs.read`, `ui` |
| JSON Bakımı (`json`) | doğrula + pretty-print | `fs.read`, `fs.write`, `ui` |
| CSV Tablo (`csv`) | `.csv` → tablo görünüm | `fs.read`, `ui` |
| Kod Sayacı (`kodsay`) | satır/kod/yorum/boş + işlev | `fs.read`, `ui` |
| Lorem Üretici (`lorem`) | paragraf üretir, dosyaya ekler | `fs.read`, `fs.write`, `ui` |
| Çeviri (`ceviri`) | EN↔TR (ücretsiz API) | `net`, `ui` |
| Hava Durumu (`hava`) | 3 günlük tahmin (ücretsiz API) | `net`, `ui` |
| Git Hızlı Komut (`githizli`) | status/log/diff → terminal | `ui` |
| Çalışma Alanı Notları (`notlar`) | dosyasız notlar (proje başına) | `ui` |
| Pastel Temalar (`pastel`) | 3 tema (galeriye düşer) | `ui` |

Davranış denetimleri: `tests/test_plugins.cpp` (62 kontrol; ağ gerektiren
iki eklenti yükleme + kayıt denetimiyle sınırlı).

## Mağaza (kayıt deposu)

Resmi kayıt: `github.com/versochar/verso-coder-plugins` (git reposu).
Kökteki `index.json` listeler, `plugins/<kimlik>/plugin.js` tek dosyalık
eklentiyi verir. Uygulama içinden `Eklentiler → Mağaza`:

- Yenile → kayıt okunur (çevrimdışıysa hata verir, sessiz geçmez).
- Kur/Güncelle → izin listesi onaya sunulur, dosya iner, başlıktaki
  `@version` listedekiyle uyuşmazsa kurulum durur.
- Güncelleme rozeti: kayıt sürümü kurulu sürümden yeniyse listede görünür.
- Kayıt adresi değiştirilebilir: `plugin/registry` ayarı ya da
  `VERSO_PLUGIN_REGISTRY` ortam değişkeni (kurumsal ayna için).

Eklenti ekleme: `packaging/plugins/publish.sh` (`--check` doğrular,
varsayılan yayınlar, `--unpublish` kayıttan çıkarır). Kural: `@name` +
`@version` başlığı zorunlu, yalnız bilinen izinler, en az izin ilkesi.
