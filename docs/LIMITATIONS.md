# Verso Coder — Bilinen Sınırlamalar (Stage 50)

Bu belge dürüsttür: nelerin çalışmadığını, nedenini ve ne zaman
düzeleceğini yazar. Pazarlama dili yok.

## AI ve kota gerçeği (UnoRouter `:free`, ölçüldü)
- Sohbet: model başına **1 istek/dk**. Gömme: **1 istek/30 dk**.
- Hız sınırı çoğunlukla 429 değil, **403/503** döner.
- Upstream doygunluğunda "All providers busy" — sigorta ve kesintili RAG
  bu yüzden var, keyfi değil.
- `openai/text-embedding-3-small` sunulmuyor (18 `:free` gömme modeli var).

## Güvenlik modeli (katmanlı, hiçbiri tek başına yeterli değil)
- Tehlikeli kalıp listesi veri sızdıran her komutu yakalayamaz → denetim
  kaydı kapatır. Kaydı kimse okumazsa katman işlevsizdir.
- Oturum izni gevşetmedir (varsayılan kapalı). Yazma/ağ asla listeye giremez.
- Eklentiye `fs.write` verildiğinde kök içi yazma serbesttir — izin verirken
  yazarı tanıyın.
- İşbirliği: anahtarlı oturumlar uçtan uca şifrelidir (ChaCha20+HMAC);
  anahtarsız oturumlar düz metindir — internette anahtarsız kullanmayın.
  Anahtar kimlik doğrulamaz (bilen herkes katılır).

## Ortam bağımlılıkları
- Salt-okunur kip yalnız `bwrap`/`unshare` kuruluysa. Yoksa onay sorulur.
- Ağ yoklaması varsayılan kapalı (gizlilik). `ai-probe doctor` ile elle.
- LSP: clangd canlı doğrulandı; pylsp/rust-analyzer/gopls kablolu ama bu
  makinede denenmedi. `pyright` yok.
- Gerçek ekran okuyucu ile uçtan uca test yapılmadı. RTL diller kapsam dışı.

## Performans (ölçüldü, kilitli)
- 100k satır açılışı 6,4 sn (çoğu vurgulamada). 2 MB üstü renksiz,
  8 MB üstü salt-okunur — bilinçli takas.
- 50 sekme açılışı tembel; ilk tıklamada kısa gecikme olur.
- Soak eşiği (64 KB/tur) tahmini; yanlış alarmda Ayarlar'dan yükseltin.

## Dağıtım
- İmza doğrulama yok. GitHub releases toplamı elle denetlenir.
- Flatpak taslak aşamasında. Temiz VM kurulum dumanı yapılmadı.

## Kapanmayan borçlar (3.x adayları)
- Soak eşiği kalibrasyonu (gerçek kullanım verisiyle).
- Onay yorgunluğu sayısal ölçümü (oturum başına modal sayısı).
- Çevrimdışı RAG (yerel gömme modeli paketlenirse).
