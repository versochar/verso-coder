# Verso Coder — Kullanıcı Kılavuzu (özet)

Hafif, yerel ve hızlı kod editörü. Temel akışlar:

## Başlangıç
- İlk açılışta sihirbaz: dil, tema, kısayol profili, yazı boyutu.
- `Ctrl+P` hızlı aç, `Ctrl+Shift+P` komut paleti, `Ctrl+Shift+F` projede ara.
- Karşılama ekranı: Klasör Aç / Yeni Dosya (`Ctrl+N`, dil sorar) / Hızlı Aç.

## Düzenleyici
- Çoklu imleç: `Alt+tık`, `Ctrl+D` (sonraki eşleşme), `Ctrl+Shift+L` (satır sonları).
- Katlama, `Ctrl+/` satır yorumu, `F2`? (gezgin yeniden adlandırma), Zen `F11`.
- Büyük dosyalar önizleme modunda açılır; "Tamamını Yükle" ile LSP başlar.
- Yazım denetimi `F7`; sözlük koda göre otomatik seçilir.

## Sekmeler
- Orta-tık kapatır, çift-tık sabitler (`◆` sabit sekmeler kapanmaya dayanıklıdır).

## Pano
- `Ctrl+Shift+V`: pano halkasındaki önceki kopyayı yapıştırır (dönerek).

## Git
- Panel: stage/unstage, commit, push/pull, log; ileti şablonları (`feat: …`),
  `Amend` ile son commit'i düzeltme.

## Görevler
- `tasks.json` (`Build`/`Test` hazır şablon); dosya yoksa panel oluşturmayı önerir.
- `dependsOn` zinciri sırayla çalışır.

## AI (Ollama)
- Model yoksa panel kurulum adımlarını gösterir; `ollama pull <model>` + ⟳.
- Sohbet `MD` düğmesiyle markdown dışa aktarılır.

## Uzak geliştirme
- `Uzak` menüsü: bağlan, gezgin, kabuk, LSP, tünel. İlk bağlanışta host
  parmak izini "Parmak izi" düğmesiyle denetleyin (TOFU).

## Eklentiler
- `Dosya → Eklentiler Klasörünü Aç`; örnek `echo.js` ilk açılışta kopyalanır.
- JS API: `verso.log / registerCommand / readFile / writeFile`
  (`// @permission fs.read|fs.write` gerekir).

## Teşhis
- `Yardım → Sistem Teşhisi`: hakkında + araç zinciri + performans +
  erişilebilirlik + yedekler; "Tanı Raporunu Kaydet" tek dosya üretir.
- `Yardım → Klavye Kısayolları`: tüm komutlar + çakışma uyarısı Ayarlar'dadır.
- `Dosya → Fabrika Ayarlarına Dön`: önce JSON yedeği alır, sonra sıfırlar.
